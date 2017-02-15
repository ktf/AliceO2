/// @file   SubframeBuilderDevice.cxx
/// @author Giulio Eulisse, Matthias Richter, Sandro Wenzel
/// @since  2017-02-07
/// @brief  Demonstrator device for a subframe builder

#include <thread> // this_thread::sleep_for
#include <chrono>

#include "DataFlow/SubframeBuilderDevice.h"
#include "DataFlow/SubframeMetadata.h"
#include "FairMQProgOptions.h"


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

bool AliceO2::DataFlow::SubframeBuilderDevice::BuildAndSendFrame()
{
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

  if (mDataType.compare("TPC")) {
    // LOG(INFO) << "SENDING TPC PAYLOAD\n";
    auto clusterPayload = new std::vector<TPCTestCluster>;
    clusterPayload->resize(1000); // sending a thousand clusters
    for (int i = 0; i < 1000; ++i) {
      // put some random toy time stamp to each cluster
      clusterPayload->operator[](i).timeStamp = md.startTime + i;
    }

    // For the moment, add the data as another part to this message
    AliceO2::Header::DataHeader payloadheader;
    payloadheader.dataDescription = AliceO2::Header::DataDescription("TPCCLUSTER");
    payloadheader.dataOrigin = AliceO2::Header::DataOrigin("TPC");
    payloadheader.subSpecification = 0;
    payloadheader.payloadSize = clusterPayload->size() * sizeof(TPCTestCluster);

    AddMessage(
      outgoing, payloadheader,
      NewMessage(clusterPayload->data(), payloadheader.payloadSize,
                 [](void* data, void* hint) { delete static_cast<decltype(clusterPayload)>(hint); }, clusterPayload));
  } else if (mDataType.compare("ITS")) {
    // LOG(INFO) << "SENDING ITS PAYLOAD\n";
    auto ITSPayload = new std::vector<ITSRawData>;
    ITSPayload->resize(500);
    for (size_t i = 0; i < ITSPayload->size(); ++i) {
      // put some toy time stamp to each raw data entry
      ITSPayload->operator[](i).timeStamp = md.startTime + i;
    }

    AliceO2::Header::DataHeader payloadheader;
    payloadheader.dataDescription = AliceO2::Header::DataDescription("ITSRAW");
    payloadheader.dataOrigin = AliceO2::Header::DataOrigin("ITS");
    payloadheader.subSpecification = 0;
    payloadheader.payloadSize = ITSPayload->size() * sizeof(ITSRawData);

    AddMessage(outgoing, payloadheader,
               NewMessage(ITSPayload->data(), payloadheader.payloadSize,
                          [](void* data, void* hint) { delete static_cast<decltype(ITSPayload)>(hint); }, ITSPayload));
  }
  else {
    LOG(INFO) << "not sending detector payload\n";
  }
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
