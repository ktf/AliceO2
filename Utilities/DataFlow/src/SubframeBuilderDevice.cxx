/// @file   SubframeBuilderDevice.cxx
/// @author Giulio Eulisse, Matthias Richter, Sandro Wenzel
/// @since  2017-02-07
/// @brief  Demonstrator device for a subframe builder

#include <thread> // this_thread::sleep_for
#include <chrono>
#include <functional>

#include "DataFlow/SubframeBuilderDevice.h"
#include "DataFlow/SubframeMetadata.h"
#include "Headers/HeartbeatFrame.h"
#include "FairMQProgOptions.h"

using HeartbeatHeader = AliceO2::Header::HeartbeatHeader;
using HeartbeatTrailer = AliceO2::Header::HeartbeatTrailer;

struct TestPayload {
 // std::vector<TestSerializedCluster> clusters;
 std::vector<double> clusters;
};


AliceO2::DataFlow::SubframeBuilderDevice::SubframeBuilderDevice()
  : O2Device()
  , mFrameNumber(0)
  , mDuration(DefaultDuration)
  , mInputChannelName()
  , mOutputChannelName()
  , mIsSelfTriggered(false)
{
}

AliceO2::DataFlow::SubframeBuilderDevice::~SubframeBuilderDevice()
{
}

void AliceO2::DataFlow::SubframeBuilderDevice::InitTask()
{
//  mDuration = fConfig->GetValue<uint32_t>(OptionKeyDuration);
  mIsSelfTriggered = fConfig->GetValue<bool>(OptionKeySelfTriggered);
  mInputChannelName = fConfig->GetValue<std::string>(OptionKeyInputChannelName);
  mOutputChannelName = fConfig->GetValue<std::string>(OptionKeyOutputChannelName);
  mInitDataFileName = fConfig->GetValue<std::string>(OptionKeyInDataFile);
  mDataType = fConfig->GetValue<std::string>(OptionKeyDetector);

  if (!mIsSelfTriggered) {
    // depending on whether the device is self-triggered or expects input,
    // the handler function needs to be registered or not.
    // ConditionalRun is not called anymore from the base class if the
    // callback is registered
    OnData(mInputChannelName.c_str(), &AliceO2::DataFlow::SubframeBuilderDevice::HandleData);
  }
}

bool AliceO2::DataFlow::SubframeBuilderDevice::ConditionalRun()
{
  // TODO: make the time constant configurable
  std::this_thread::sleep_for(std::chrono::nanoseconds(mDuration));

  BuildAndSendFrame();
  mFrameNumber++;

  return true;
}


// FIXME: how do we actually find out the payload size???
size_t extractDetectorPayload(char **payload, char *buffer, size_t bufferSize) {
  *payload = buffer + sizeof(HeartbeatHeader);
  return bufferSize - sizeof(HeartbeatHeader) - sizeof(HeartbeatTrailer);
}

static std::set<std::string> gValidDetectorTypes = {"TPC", "ITS"};

bool AliceO2::DataFlow::SubframeBuilderDevice::BuildAndSendFrame()
{
  if (gValidDetectorTypes.count(mDataType) == 0) {
    LOG(INFO) << "not sending detector payload\n";
    return true;
  }

  // top level subframe header, the DataHeader is going to be used with
  // description "SUBTIMEFRAMEMETA"
  // this should be defined in a common place, and also the origin
  // the origin can probably name a detector identifier, but not sure if
  // all CRUs of a FLP in all cases serve a single detector
  AliceO2::Header::DataHeader dh;
  dh.dataDescription = AliceO2::Header::DataDescription("SUBTIMEFRAMEMETA");
  dh.dataOrigin = AliceO2::Header::DataOrigin("TEST");
  dh.subSpecification = 0;
  dh.payloadSize = sizeof(SubframeMetadata);

  // subframe meta information as payload
  SubframeMetadata md;
  md.startTime = mFrameNumber * mDuration + mHeartbeatStart;
  md.duration = mDuration;
  LOG(INFO) << "Start time for subframe " << timeframeIdFromTimestamp(md.startTime, mDuration) << " " << md.startTime<< "\n";

  // send an empty subframe (no detector payload), only the data header
  // and the subframe meta data are added to the sub timeframe
  // TODO: this is going to be changed as soon as the device implements
  // handling of the input data
  O2Message outgoing;

  // build multipart message from header and payload
  AddMessage(outgoing, dh, NewSimpleMessage(md));

  char *incomingBuffer = nullptr;
  AliceO2::Header::DataHeader payloadheader;

  size_t bufferSize = 0;

  if (mDataType.compare("TPC")) {
    std::function<void(TPCTestCluster&, int)> f = [md](TPCTestCluster &cluster, int idx) {cluster.timeStamp = md.startTime + idx;};
    bufferSize = fakeHBHPayloadHBT<TPCTestCluster>(&incomingBuffer, f, 1000);
    // For the moment, add the data as another part to this message
    payloadheader.dataDescription = AliceO2::Header::DataDescription("TPCCLUSTER");
    payloadheader.dataOrigin = AliceO2::Header::DataOrigin("TPC");
  } else if (mDataType.compare("ITS")) {
    bufferSize = fakeHBHPayloadHBT<ITSRawData>(&incomingBuffer, [md](ITSRawData &cluster, int idx) {cluster.timeStamp = md.startTime + idx;}, 500);
    payloadheader.dataDescription = AliceO2::Header::DataDescription("ITSRAW");
    payloadheader.dataOrigin = AliceO2::Header::DataOrigin("ITS");
  }

  char *payload = nullptr;
  auto payloadSize = extractDetectorPayload((char **)payload, (char *)incomingBuffer, bufferSize);
  payloadheader.subSpecification = 0;
  payloadheader.payloadSize = payloadSize;

  AddMessage(outgoing, payloadheader,
             NewMessage(payload, payloadheader.payloadSize,
                        [](void* data, void* hint) { delete[] reinterpret_cast<char *>(hint); }, incomingBuffer));
  // send message
  Send(outgoing, mOutputChannelName.c_str());
  outgoing.fParts.clear();

  return true;
}

bool AliceO2::DataFlow::SubframeBuilderDevice::HandleData(FairMQParts& msgParts, int /*index*/)
{
  // loop over header payload pairs in the incoming multimessage
  // for each pair
  // - check timestamp
  // - create new subtimeframe if none existing where the timestamp of the data fits
  // - add pair to the corresponding subtimeframe

  // check for completed subtimeframes and send all completed frames
  // the builder does not implement the routing to the EPN, this is done in the
  // specific FLP-EPN setup
  // to fit into the simple emulation of event/frame ids in the flpSender the order of
  // subtimeframes needs to be preserved
  return true;
}
