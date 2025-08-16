// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
///
/// \brief FullTracks is a join of Tracks, TracksCov, and TracksExtra.
/// \author
/// \since

#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"
#include "Framework/AnalysisDataModel.h"
#include "DataFormatsTOF/CalibLHCphaseTOF.h"
#include <iostream>

#include <TH2F.h>

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;

namespace o2::aod
{
namespace tofcalib
{
DECLARE_SOA_CCDB_COLUMN(LHCphase, lhcPhase, o2::dataformats::CalibLHCphaseTOF, "TOF/LHCphase"); //!
} // namespace tofcalib

DECLARE_SOA_TIMESTAMPED_TABLE(TOFCalibrationObjects, aod::Timestamps, o2::aod::timestamp::Timestamp, 1, "TOFCALIB", //!
                              tofcalib::LHCphase);
} // namespace o2::aod

struct DummyTimestampsTable {
  Produces<aod::Timestamps> timestamps; /// Table with SOR timestamps produced by the task
  Service<o2::framework::ControlService> control;

  void process(Enumeration<0, 1>& e)
  {
    timestamps(1755262994000); // 14-08-2025
    timestamps(1755255600011); // 15-08-2025
    control->readyToQuit(QuitRequest::Me);
    std::cout << "Executed " << std::endl;
  }
};

struct SimpleCCDBConsumer {
  void process(o2::aod::TOFCalibrationObjects const& ccdbObjectsForAllTimestamps)
  {
    LOGP(info, "Looking at all the LHCphases associated to the timestamps");
    for (auto& object : ccdbObjectsForAllTimestamps) {
      std::cout << object.lhcPhase().getStartValidity() << " " << object.lhcPhase().getEndValidity() << std::endl;
    }
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{
    adaptAnalysisTask<DummyTimestampsTable>(cfgc),
    adaptAnalysisTask<SimpleCCDBConsumer>(cfgc, TaskName{"simple-ccdb-cunsumer"}),
  };
}
