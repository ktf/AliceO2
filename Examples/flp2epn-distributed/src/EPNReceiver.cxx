/**
 * EPNReceiver.cxx
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko, M. Al-Turany, C. Kouzinopoulos
 */

#include <cstddef> // size_t
#include <fstream> // writing to file (DEBUG)

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"

#include "FLP2EPNex_distributed/EPNReceiver.h"
#include "Headers/DataHeader.h"
#include "DataFlow/SubframeMetadata.h"

using namespace std;
using namespace std::chrono;
using namespace AliceO2::Devices;
using SubframeMetadata = AliceO2::DataFlow::SubframeMetadata;
using TPCTestPayload = AliceO2::DataFlow::TPCTestPayload;
using TPCTestCluster = AliceO2::DataFlow::TPCTestCluster;

EPNReceiver::EPNReceiver()
  : fTimeframeBuffer()
  , fDiscardedSet()
  , fNumFLPs(0)
  , fBufferTimeoutInMs(5000)
  , fTestMode(0)
  , fInChannelName()
  , fOutChannelName()
  , fAckChannelName()
{
}

EPNReceiver::~EPNReceiver()
{
}

void EPNReceiver::InitTask()
{
  fNumFLPs = fConfig->GetValue<int>("num-flps");
  fBufferTimeoutInMs = fConfig->GetValue<int>("buffer-timeout");
  fTestMode = fConfig->GetValue<int>("test-mode");
  fInChannelName = fConfig->GetValue<string>("in-chan-name");
  fOutChannelName = fConfig->GetValue<string>("out-chan-name");
  fAckChannelName = fConfig->GetValue<string>("ack-chan-name");
}

void EPNReceiver::PrintBuffer(const unordered_map<uint16_t, TFBuffer>& buffer) const
{
  string header = "===== ";

  for (int i = 1; i <= fNumFLPs; ++i) {
    stringstream out;
    out << i % 10;
    header += out.str();
    //i > 9 ? header += " " : header += "  ";
  }
  LOG(INFO) << header;

  for (auto& it : buffer) {
    string stars = "";
    for (unsigned int j = 1; j <= (it.second).parts.Size(); ++j) {
      stars += "*";
    }
    LOG(INFO) << setw(4) << it.first << ": " << stars;
  }
}

void EPNReceiver::DiscardIncompleteTimeframes()
{
  auto it = fTimeframeBuffer.begin();

  while (it != fTimeframeBuffer.end()) {
    if (duration_cast<milliseconds>(steady_clock::now() - (it->second).start).count() > fBufferTimeoutInMs) {
      LOG(WARN) << "Timeframe #" << it->first << " incomplete after " << fBufferTimeoutInMs << " milliseconds, discarding";
      fDiscardedSet.insert(it->first);
      fTimeframeBuffer.erase(it++);
      LOG(WARN) << "Number of discarded timeframes: " << fDiscardedSet.size();
    } else {
      // LOG(INFO) << "Timeframe #" << it->first << " within timeout, buffering...";
      ++it;
    }
  }
}

void EPNReceiver::Run()
{
  // DEBUG: store receive intervals per FLP
  // vector<vector<int>> rcvIntervals(fNumFLPs, vector<int>());
  // vector<std::chrono::steady_clock::time_point> rcvTimestamp(fNumFLPs);
  // end DEBUG

  // f2eHeader* header; // holds the header of the currently arrived message.
  uint16_t id = 0; // holds the timeframe id of the currently arrived sub-timeframe.

  FairMQChannel& ackOutChannel = fChannels.at(fAckChannelName).at(0);

  while (CheckCurrentState(RUNNING)) {
    FairMQParts subtimeframeParts;
    if (Receive(subtimeframeParts, fInChannelName, 0, 100) <= 0)
      continue;

    assert(subtimeframeParts.Size() >= 2);

    Header::DataHeader* dh = reinterpret_cast<Header::DataHeader*>(subtimeframeParts.At(0)->GetData());
    assert(strncmp(dh->dataDescription.str, "SUBTIMEFRAMEMETA", 16) == 0);
    SubframeMetadata* sfm = reinterpret_cast<SubframeMetadata*>(subtimeframeParts.At(1)->GetData());
    id = AliceO2::DataFlow::timeframeIdFromTimestamp(sfm->startTime);

    // in this case the subtime frame did send some data
    if(subtimeframeParts.Size() > 2) {
      int part = 2;
      // check if we got something from TPC
      auto *header = reinterpret_cast<Header::DataHeader*>(subtimeframeParts.At(part)->GetData());
      if (strncmp(header->dataDescription.str, "TPCCLUSTER", 16) == 0) {
         assert( header->payloadSize == subtimeframeParts.At(part+1)->GetSize() );
         TPCTestCluster *cl = reinterpret_cast<TPCTestCluster*>(subtimeframeParts.At(part+1)->GetData());
         auto numberofClusters = header->payloadSize / sizeof(TPCTestCluster);
         assert( header->payloadSize % sizeof(TPCTestCluster) == 0 );
      }
    }

    if (fDiscardedSet.find(id) == fDiscardedSet.end())
    {
      if (fTimeframeBuffer.find(id) == fTimeframeBuffer.end())
      {
        // if this is the first part with this ID, save the receive time.
        fTimeframeBuffer[id].start = steady_clock::now();
      }
      // if the received ID has not previously been discarded,
      // store the data part in the buffer
      fTimeframeBuffer[id].parts.AddPart(move(subtimeframeParts.At(1)));
      PrintBuffer(fTimeframeBuffer);
    }
    else
    {
      // if received ID has been previously discarded.
      LOG(WARN) << "Received part from an already discarded timeframe with id " << id;
    }

    if (fTimeframeBuffer[id].parts.Size() == fNumFLPs) {
      // LOG(INFO) << "Collected all parts for timeframe #" << id;
      // when all parts are collected send then to the output channel
      Send(fTimeframeBuffer[id].parts, fOutChannelName);

      if (fTestMode > 0) {
        // Send an acknowledgement back to the sampler to measure the round trip time
        unique_ptr<FairMQMessage> ack(NewMessage(sizeof(uint16_t)));
        memcpy(ack->GetData(), &id, sizeof(uint16_t));

        if (ackOutChannel.Send(ack, 0) <= 0) {
          LOG(ERROR) << "Could not send acknowledgement without blocking";
        }
      }

      // fTimeframeBuffer[id].end = steady_clock::now();

      fTimeframeBuffer.erase(id);
    }

    // LOG(WARN) << "Buffer size: " << fTimeframeBuffer.size();

    // Check if any incomplete timeframes in the buffer are older than
    // timeout period, and discard them if they are
    DiscardIncompleteTimeframes();
  }

  // DEBUG: save
  // if (fTestMode > 0) {
  //   std::time_t t = system_clock::to_time_t(system_clock::now());
  //   tm utc = *gmtime(&t);
  //   std::stringstream s;
  //   s << utc.tm_year + 1900 << "-" << utc.tm_mon + 1 << "-" << utc.tm_mday << "-" << utc.tm_hour << "-" << utc.tm_min << "-" << utc.tm_sec;
  //   string name = s.str();
  //   for (int x = 0; x < fNumFLPs; ++x) {
  //     ofstream flpRcvTimes(fId + "-" + name + "-flp-" + to_string(x) + ".log");
  //     for (auto it = rcvIntervals.at(x).begin() ; it != rcvIntervals.at(x).end(); ++it) {
  //       flpRcvTimes << *it << endl;
  //     }
  //     flpRcvTimes.close();
  //   }
  // }
  // end DEBUG
}
