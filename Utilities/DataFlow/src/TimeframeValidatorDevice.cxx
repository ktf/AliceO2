/// @file   TimeframeValidatorDevice.cxx
/// @author Giulio Eulisse, Matthias Richter, Sandro Wenzel
/// @since  2017-02-07
/// @brief  Validator device for a full time frame

#include <thread> // this_thread::sleep_for
#include <chrono>

#include "DataFlow/TimeframeValidatorDevice.h"
#include "FairMQProgOptions.h"


AliceO2::DataFlow::TimeframeValidatorDevice::TimeframeValidatorDevice()
  : O2Device()
  , mInChannelName()
{
}

void AliceO2::DataFlow::TimeframeValidatorDevice::InitTask()
{
  mInChannelName = fConfig->GetValue<std::string>(OptionKeyInputChannelName);
}



void AliceO2::DataFlow::TimeframeValidatorDevice::Run()
{
  while (CheckCurrentState(RUNNING)) {
    FairMQParts timeframeParts;
    if (Receive(timeframeParts, mInChannelName, 0, 100) <= 0)
      continue;

    assert(timeframeParts.Size() >= 2);

    // TODO: fill this with checks on time frame
    LOG(INFO) << "This time frame has " << timeframeParts.Size() << " parts \n";
  }
}
