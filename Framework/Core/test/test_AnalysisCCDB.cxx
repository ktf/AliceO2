// Copyright 2019-2022 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"

#include <cmath>

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;

DECLARE_SOA_STORE();
namespace test
{
DECLARE_SOA_COLUMN_FULL(X, x, int32_t, "x");
DECLARE_SOA_COLUMN_FULL(Y, y, int32_t, "y");
DECLARE_SOA_COLUMN_FULL(Z, z, int32_t, "z");
DECLARE_SOA_DYNAMIC_COLUMN(Sum, sum, [](int32_t x, int32_t y) { return x + y; });
DECLARE_SOA_EXPRESSION_COLUMN(ESum, esum, int32_t, test ::x + test::y);
} // namespace test
  //
DECLARE_SOA_TABLE(Points, "TST", "POINTS", test::X, test::Y);


#define ASSERT_ERROR(condition)                                   \
  if ((condition) == false) {                                     \
    LOG(fatal) << R"(Test condition ")" #condition R"(" failed)"; \
  }

struct CCDBTestFakeDataSource {
  Service<o2::framework::ControlService> control;
  Produces<Points> points;

  void process(Enumeration<0, 1>& e)
  {
    points(0, 0);
    points(1, 1);
    points(2, 2);
    // Hardcoded from DataFormats/simulation/include/SimulationDataFormat/O2DatabasePDG.h
  }
};

struct CCDBTestConsumer {
  Condition<TOF/LHCphase> obj = {"TOF/LHCphase"};

  void process(aod::TestTable const& table)
  {
    control->readyToQuit(QuitRequest::Me);
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{
    adaptAnalysisTask<CCDBTestFakeDataSource>(cfgc),
    adaptAnalysisTask<CCDBTestConsumer>(cfgc)};
}
