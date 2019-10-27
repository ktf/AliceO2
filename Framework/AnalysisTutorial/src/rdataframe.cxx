// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"
#include "Framework/AnalysisDataModel.h"
#include <ROOT/RDataframe.hxx>
#include <ROOT/RArrowDS.hxx>

#include <cmath>

using namespace o2;
using namespace o2::framework;

// This is a very simple example showing how to create an histogram
// FIXME: this should really inherit from AnalysisTask but
//        we need GCC 7.4+ for that
struct ATask {
  void process(aod::Tracks const& tracks)
  {
    auto source = std::make_unique<ROOT::RDF::RArrowDS>(tracks.asArrowTable(), std::vector<std::string>{});
    ROOT::RDataFrame rdf(std::move(source));
    LOG(ERROR) << *(rdf.Count());
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  return WorkflowSpec{
    adaptAnalysisTask<ATask>("rdataframe"),
  };
}
