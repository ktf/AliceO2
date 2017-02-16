/// @file   TimeframeValidatorDevice.cxx
/// @author Giulio Eulisse, Matthias Richter, Sandro Wenzel
/// @since  2017-02-07
/// @brief  Validator device for a full time frame

#include <thread> // this_thread::sleep_for
#include <chrono>

#include "DataFlow/TimeframeValidatorDevice.h"
#include "FairMQProgOptions.h"

// FIXME: this should really be in a central place
typedef int PartPosition;
typedef std::pair<AliceO2::Header::DataHeader, PartPosition> IndexElement;

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

    auto indexHeader = reinterpret_cast<Header::DataHeader*>(timeframeParts.At(timeframeParts.Size() - 2)->GetData());
    auto index = reinterpret_cast<void*>(timeframeParts.At(timeframeParts.Size() - 1)->GetData());

    // TODO: fill this with checks on time frame
    LOG(INFO) << "This time frame has " << timeframeParts.Size() << " parts.\n";
    auto indexEntries = indexHeader->payloadSize / sizeof(IndexElement);
    LOG(INFO) << "This time frame has " << indexEntries << "entries in the index.\n";
    if ((indexEntries * 2 + 2) != (timeframeParts.Size()))
      LOG(ERROR) << "Mismatched index and received parts\n";
  }
}
