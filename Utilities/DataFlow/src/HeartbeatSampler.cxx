// @file   HeartbeatSampler.h
// @author Matthias Richter
// @since  2017-02-03
// @brief  Heartbeat sampler device

#include <thread> // this_thread::sleep_for
#include <chrono>

#include "DataFlow/HeartbeatSampler.h"
#include "Headers/HeartbeatFrame.h"
#include "FairMQProgOptions.h"

AliceO2::DataFlow::HeartbeatSampler::HeartbeatSampler()
  : O2Device()
  , mPeriod(89100)
  , mOutputChannelName("output")
{
}

AliceO2::DataFlow::HeartbeatSampler::~HeartbeatSampler()
{
}

void AliceO2::DataFlow::HeartbeatSampler::InitTask()
{
  mPeriod = fConfig->GetValue<uint32_t>(OptionKeyPeriod);
  mOutputChannelName = fConfig->GetValue<std::string>(OptionKeyOutputChannelName);
}

bool AliceO2::DataFlow::HeartbeatSampler::ConditionalRun()
{
  std::this_thread::sleep_for(std::chrono::nanoseconds(mPeriod));

  AliceO2::Header::HeartbeatStatistics hbfPayload;

  AliceO2::Header::DataHeader dh;
  dh.dataDescription = AliceO2::Header::gDataDescriptionHeartbeatFrame;
  dh.dataOrigin = AliceO2::Header::DataOrigin("TEST");
  dh.subSpecification = 0;
  dh.payloadSize = sizeof(hbfPayload);

  AliceO2::Header::HeartbeatFrameEnvelope specificHeader;
  //specificHeader.header = 0;
  //specificHeader.trailer = 0;

  O2Message outgoing;

  // build multipart message from header and payload
  AddMessage(outgoing, {dh, specificHeader}, NewSimpleMessage(hbfPayload));

  // send message
  Send(outgoing, mOutputChannelName.c_str());
  outgoing.fParts.clear();

  return true;
}
