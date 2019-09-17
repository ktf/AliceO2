// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "Framework/ASoA.h"
#include "Framework/TableBuilder.h"
#include "Framework/AnalysisDataModel.h"
#include <TH1F.h>
#include <benchmark/benchmark.h>
#include <random>
#include <vector>

using namespace o2::framework;
using namespace arrow;
using namespace o2::soa;

static void BM_RootSimpleFill(benchmark::State& state)
{
  struct XYZ {
    double x;
    double y;
    double z;
  };
  std::vector<XYZ> foo;
  foo.resize(state.range(0));

  // Seed with a real random value, if available
  std::default_random_engine e1(1234567891);
  std::uniform_real_distribution<float> uniform_dist(0, 1);

  for (size_t i = 0; i < state.range(0); ++i) {
    foo[i] = XYZ{uniform_dist(e1), uniform_dist(e1), uniform_dist(e1)};
  }

  for (auto _ : state) {
    TH1F histo{"Foo", "Bar", 100,  0, 1};
    for (auto& xyz : foo) {
      histo.Fill(xyz.x);
    }
    benchmark::DoNotOptimize(histo);
  }
}

BENCHMARK(BM_RootSimpleFill)->Range(8, 8 << 17);

static void BM_RootSimpleFillN(benchmark::State& state)
{
  struct XYZ {
    double x;
    double y;
    double z;
  };
  std::vector<XYZ> foo;
  foo.resize(state.range(0));

  // Seed with a real random value, if available
  std::default_random_engine e1(1234567891);
  std::uniform_real_distribution<float> uniform_dist(0, 1);

  for (size_t i = 0; i < state.range(0); ++i) {
    foo[i] = XYZ{uniform_dist(e1), uniform_dist(e1), uniform_dist(e1)};
  }

  for (auto _ : state) {
    TH1F histo{"Foo", "Bar", 100,  0, 1};
    histo.FillN(foo.size(), (const double*)foo.data(), nullptr, 3);
    benchmark::DoNotOptimize(histo);
  }
}

BENCHMARK(BM_RootSimpleFillN)->Range(8, 8 << 17);
BENCHMARK_MAIN()
