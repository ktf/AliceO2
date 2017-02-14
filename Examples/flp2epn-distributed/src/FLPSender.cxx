/**
 * FLPSender.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko, M. Al-Turany, C. Kouzinopoulos
 */

#include <cstdint> // UINT64_MAX
#include <cassert>

#include "FairMQLogger.h"
#include "FairMQMessage.h"
#include "FairMQTransportFactory.h"
#include "FairMQProgOptions.h"

#include "FLP2EPNex_distributed/FLPSender.h"
#include "Headers/DataHeader.h"
#include "DataFlow/SubframeMetadata.h"

using namespace std;
using namespace std::chrono;
using namespace AliceO2::Devices;
using SubframeMetadata = AliceO2::DataFlow::SubframeMetadata;

FLPSender::FLPSender()
  : fSTFBuffer()
  , fArrivalTime()
  , fNumEPNs(0)
  , fIndex(0)
  , fSendOffset(0)
  , fSendDelay(8)
  , fEventSize(10000)
  , fTestMode(0)
  , fTimeFrameId(0)
  , fInChannelName()
  , fOutChannelName()
{
}

FLPSender::~FLPSender()
{
}

void FLPSender::InitTask()
{
  fIndex = fConfig->GetValue<int>("flp-index");
  fEventSize = fConfig->GetValue<int>("event-size");
  fNumEPNs = fConfig->GetValue<int>("num-epns");
  fTestMode = fConfig->GetValue<int>("test-mode");
  fSendOffset = fConfig->GetValue<int>("send-offset");
  fSendDelay = fConfig->GetValue<int>("send-delay");
  fInChannelName = fConfig->GetValue<string>("in-chan-name");
  fOutChannelName = fConfig->GetValue<string>("out-chan-name");
}


void FLPSender::Run()
{
  // base buffer, to be copied from for every timeframe body (zero-copy)
  FairMQMessagePtr baseMsg(NewMessage(fEventSize));

  // store the channel reference to avoid traversing the map on every loop iteration
  //FairMQChannel& dataInChannel = fChannels.at(fInChannelName).at(0);

  while (CheckCurrentState(RUNNING)) {
    // - Get the SubtimeframeMetadata
    // - Add the current FLP id to the SubtimeframeMetadata
    // - Forward to the EPN the whole subtimeframe
    FairMQParts subtimeframeParts;
    if (Receive(subtimeframeParts, fInChannelName, 0, 100) <= 0)
      continue;

    assert(subtimeframeParts.Size() != 0);
    assert(subtimeframeParts.Size() >= 2);
    Header::DataHeader* dh = reinterpret_cast<Header::DataHeader*>(subtimeframeParts.At(0)->GetData());
    assert(strncmp(dh->dataDescription.str, "SUBTIMEFRAMEMETA", 16) == 0);

    SubframeMetadata* sfm = reinterpret_cast<SubframeMetadata*>(subtimeframeParts.At(1)->GetData());
    sfm->flpIndex = fIndex;

    fArrivalTime.push(steady_clock::now());
    fSTFBuffer.push(move(subtimeframeParts));

    // if offset is 0 - send data out without staggering.
    assert(fSTFBuffer.size() > 0);

    if (fSendOffset == 0 && fSTFBuffer.size() > 0) {
      sendFrontData();
    } else if (fSTFBuffer.size() > 0) {
      if (duration_cast<milliseconds>(steady_clock::now() - fArrivalTime.front()).count() >= (fSendDelay * fSendOffset)) {
        sendFrontData();
      } else {
        // LOG(INFO) << "buffering...";
      }
    }
  }
}

inline void FLPSender::sendFrontData()
{
  SubframeMetadata *sfm = static_cast<SubframeMetadata*>(fSTFBuffer.front().At(1)->GetData());
  uint16_t currentTimeframeId = AliceO2::DataFlow::timeframeIdFromTimestamp(sfm->startTime);

  // for which EPN is the message?
  int direction = currentTimeframeId % fNumEPNs;
  // LOG(INFO) << "Sending event " << currentTimeframeId << " to EPN#" << direction << "...";

  if (Send(fSTFBuffer.front(), fOutChannelName, direction, 0) < 0) {
    LOG(ERROR) << "Failed to queue sub-timeframe #" << currentTimeframeId << " to EPN[" << direction << "]";
  }
  fSTFBuffer.pop();
  fArrivalTime.pop();
}
