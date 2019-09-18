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
#include <TList.h>
#include <benchmark/benchmark.h>
#include <random>
#include <vector>

using namespace o2::framework;
using namespace arrow;
using namespace o2::soa;

static const int nChunks = 4;

struct XYZ {
  double x;
  double y;
  double z;
};

inline bool operator<(const XYZ& lhs, const XYZ& rhs)
{
  return lhs.x < rhs.x;
}

static void BM_RootSimpleFill(benchmark::State& state)
{
  std::vector<XYZ> foo;
  foo.resize(nChunks * state.range(0));

  // Seed with a real random value, if available
  std::default_random_engine e1(1234567891);
  std::uniform_real_distribution<float> uniform_dist(0, 10);

  for (size_t i = 0; i < nChunks * state.range(0); ++i) {
    foo[i] = XYZ{uniform_dist(e1), uniform_dist(e1), uniform_dist(e1)};
  }

  TH1F histo{"Foo", "Bar", 100, 0, 10};
  for (auto _ : state) {
    histo.Reset();
    for (auto& xyz : foo) {
      histo.Fill(xyz.x);
    }
    benchmark::DoNotOptimize(histo);
  }
  state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(double) * nChunks);
}

static void BM_RootSimpleFillN(benchmark::State& state)
{
  std::vector<XYZ> foo;
  foo.resize(nChunks * state.range(0));

  // Seed with a real random value, if available
  std::default_random_engine e1(1234567891);
  std::uniform_real_distribution<float> uniform_dist(0, 10);

  for (size_t i = 0; i < nChunks * state.range(0); ++i) {
    foo[i] = XYZ{uniform_dist(e1), uniform_dist(e1), uniform_dist(e1)};
  }

  TH1F histo{"Foo", "Bar", 100, 0, 10};
  for (auto _ : state) {
    histo.FillN(foo.size(), (const double*)foo.data(), nullptr, 1);
    benchmark::DoNotOptimize(histo);
  }
  state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(double) * nChunks);
}

static void BM_RootPresortedFillN(benchmark::State& state)
{
  std::vector<XYZ> foo;
  foo.resize(nChunks * state.range(0));

  // Seed with a real random value, if available
  std::default_random_engine e1(1234567891);
  std::uniform_real_distribution<float> uniform_dist(0, 10);

  for (size_t i = 0; i < nChunks * state.range(0); ++i) {
    foo[i] = XYZ{uniform_dist(e1), uniform_dist(e1), uniform_dist(e1)};
  }

  std::sort(foo.begin(), foo.end());

  TH1F histo{"Foo", "Bar", 100, 0, 10};
  for (auto _ : state) {
    histo.FillN(foo.size(), (const double*)foo.data(), nullptr, 1);
    benchmark::DoNotOptimize(histo);
  }
  state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(double) * nChunks);
}

static void BM_RootChunkedFillN(benchmark::State& state)
{
  using array_t = std::vector<XYZ>;
  array_t foo[8];
  for (auto i = 0u; i < nChunks; ++i) {
    (foo[i]).resize(state.range(0));
  }

  // Seed with a real random value, if available
  for (auto j = 0u; j < nChunks; ++j) {
    std::default_random_engine e1(1234567891);
    std::uniform_real_distribution<float> uniform_dist(0, 10);

    for (size_t i = 0; i < state.range(0); ++i) {
      (foo[j])[i] = XYZ{uniform_dist(e1), uniform_dist(e1), uniform_dist(e1)};
    }
  }

  TH1F histo1{"Foo1", "Bar1", 100, 0, 10};
  TH1F histo2{"Foo2", "Bar2", 100, 0, 10};
  TH1F histo3{"Foo3", "Bar3", 100, 0, 10};
  TH1F histo4{"Foo4", "Bar4", 100, 0, 10};
  TH1F histo5{"Foo5", "Bar5", 100, 0, 10};
  TH1F histo6{"Foo6", "Bar6", 100, 0, 10};
  TH1F histo7{"Foo7", "Bar7", 100, 0, 10};
  TH1F histo8{"Foo8", "Bar8", 100, 0, 10};

  std::vector<TH1F*> histos{&histo1, &histo2, &histo3, &histo4, &histo5, &histo6, &histo7, &histo8};
  for (auto _ : state) {
    for (auto i = 0u; i < nChunks; ++i) {
      histos[i]->FillN((foo[i]).size(), (const double*)(foo[i]).data(), nullptr, 1);
    }

    TH1F histom{"M", "M", 100, 0, 10};
    TList l{};
    for (auto i = 0u; i < nChunks; ++i) {
      l.Add(histos[i]);
    }
    histom.Merge(&l);

    benchmark::DoNotOptimize(histom);
  }
  state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(double) * nChunks);
}

BENCHMARK(BM_RootSimpleFill)->Range(8, 8 << 17);
BENCHMARK(BM_RootSimpleFill)->Range(8, 8 << 17);
BENCHMARK(BM_RootPresortedFillN)->Range(8, 8 << 17);
BENCHMARK(BM_RootSimpleFillN)->Range(8, 8 << 17);
BENCHMARK(BM_RootChunkedFillN)->Range(8, 8 << 17);

BENCHMARK_MAIN()
