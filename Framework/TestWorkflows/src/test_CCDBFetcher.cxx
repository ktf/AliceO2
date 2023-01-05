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
#include "Framework/runDataProcessing.h"
#include "Framework/ServiceRegistry.h"
#include "Framework/ControlService.h"
#include "Framework/CCDBParamSpec.h"
#include "Framework/DataRefUtils.h"
#include "Framework/Task.h"
#include "DataFormatsTOF/CalibLHCphaseTOF.h"

#include <chrono>
#include <thread>

using namespace o2::framework;

struct CCDBTestTask : public o2::framework::Task {
  void run(ProcessingContext& ctx) final
  {
    auto& inputs = ctx.inputs();
    auto ref = inputs.get("somecondition");
    auto payloadSize = DataRefUtils::getPayloadSize(ref);
    if (payloadSize != 2048) {
      LOGP(error, "Wrong size for condition payload (expected {}, found {}", 2048, payloadSize);
    }
    auto condition = inputs.get<o2::dataformats::CalibLHCphaseTOF*>("somecondition");
    LOG(error) << "Condition size" << condition->size();
    for (int pi = 0; pi < condition->size(); pi++) {
      LOGP(info, "Phase at {} for timestamp {} is {}", pi, condition->timestamp(pi), condition->LHCphase(pi));
    }
  }

  void finaliseCCDB(ConcreteDataMatcher&, void*) final
  {
    LOG(error) << "Deserialization callback invoked";
  }
};

struct ProduceDummyData : public o2::framework::Task {
  void run(ProcessingContext& ctx) final
  {
    LOG(error) << "Producing dummy data";
    ctx.outputs().make<int>(OutputRef{"a", 0}, 1);
  }

};

// This is how you can define your processing in a declarative way
WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  return WorkflowSpec{
    {
      .name = "CCDBTestTask",
      .inputs = {InputSpec{"somecondition", "TOF", "LHCphase", 0, Lifetime::Condition, ccdbParamSpec("TOF/LHCphase")},
                 InputSpec{"sometimer", "TST", "A", 0, Lifetime::Timeframe}},
      .outputs = {OutputSpec{"TST", "B", 0, Lifetime::Timeframe}},
      .algorithm = adaptFromTask<CCDBTestTask>(),
    },
    {
      .name = "A",
      .inputs = {InputSpec{"sometimer", "TST", "BAR", 0, Lifetime::Timer, {startTimeParamSpec(1638548475371)}}},
      .outputs = {OutputSpec{{"a"}, "TST", "A", 0, Lifetime::Timeframe}},
      .algorithm = adaptFromTask<ProduceDummyData>(),
    }
  };
}
