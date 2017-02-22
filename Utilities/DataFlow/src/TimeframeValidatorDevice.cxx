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

    if (timeframeParts.Size() < 2)
      LOG(ERROR) << "Expecting at least 2 parts\n";

    auto indexHeader = reinterpret_cast<Header::DataHeader*>(timeframeParts.At(timeframeParts.Size() - 2)->GetData());
    // FIXME: Provide iterator pair API for the index 
    //        Index should really be something which provides an
    //        iterator pair API so that we can sort / find / lower_bound
    //        easily. Right now we simply use it a C-style array.
    auto index = reinterpret_cast<IndexElement*>(timeframeParts.At(timeframeParts.Size() - 1)->GetData());

    // TODO: fill this with checks on time frame
    LOG(INFO) << "This time frame has " << timeframeParts.Size() << " parts.\n";
    auto indexEntries = indexHeader->payloadSize / sizeof(IndexElement);
    if (strncmp(indexHeader->dataDescription.str, "TIMEFRAMEINDEX", 14) != 0)
      LOG(ERROR) << "Could not find a valid index header\n";
    LOG(INFO) << indexHeader->dataDescription.str << "\n";
    LOG(INFO) << "This time frame has " << indexEntries << "entries in the index.\n";
    if ((indexEntries * 2 + 2) != (timeframeParts.Size()))
      LOG(ERROR) << "Mismatched index and received parts\n";

    // - Use the index to find out if we have TPC data
    // - Get the part with the TPC data
    // - Validate TPCCluster dummy data
    // - Validate ITSRaw dummy data
    int tpcIndex = -1;
    int itsIndex = -1;

    for (int ii = 0; ii < indexEntries; ++ii) {
      IndexElement &ie = index[ii];
      if (ie.first.dataDescription == "TPC")
        tpcIndex = ie.second;
      if (ie.first.dataDescription == "ITS")
        itsIndex = ie.second;
    }

    if (tpcIndex < 0)
      LOG(ERROR) << "Could not find expected TPC payload\n";
    if (itsIndex < 0)
      LOG(ERROR) << "Could not find expected ITS payload\n";
    LOG(INFO) << "Everything is fine with received timeframe\n";
  }
}
