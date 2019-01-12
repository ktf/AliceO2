// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#include <benchmark/benchmark.h>

#include "Framework/TableBuilder.h"
#include "Framework/RootTableBuilderHelpers.h"

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RArrowDS.hxx>
#include <TTree.h>
#include <TRandom.h>
#include <arrow/table.h>
#include <arrow/ipc/writer.h>
#include <arrow/io/memory.h>
#include <arrow/ipc/writer.h>
#include <arrow/ipc/reader.h>

#include <random>
#include <memory>

using namespace o2::framework;

template class std::shared_ptr<arrow::Schema>;
template class std::shared_ptr<arrow::Column>;
template class std::vector<std::shared_ptr<arrow::Column>>;
template class std::shared_ptr<arrow::Array>;
template class std::vector<std::shared_ptr<arrow::Field>>;
template class std::shared_ptr<arrow::ChunkedArray>;
template class std::shared_ptr<arrow::Table>;
template class std::shared_ptr<arrow::Field>;

// Helper to fill a tree with random data.
void fillTTree(TTree &t1, size_t entries) {
  Float_t xyz[3];
  Int_t ij[2];
  Float_t px, py, pz;
  Double_t random;
  Int_t ev;
  t1.Branch("px", &px, "px/F");
  t1.Branch("py", &py, "py/F");
  t1.Branch("pz", &pz, "pz/F");
  t1.Branch("random", &random, "random/D");
  t1.Branch("ev", &ev, "ev/I");
  t1.Branch("xyz", xyz, "xyz[3]/F");
  t1.Branch("ij", ij, "ij[2]/I");
  //fill the tree
  for (Int_t i = 0; i < entries; i++) {
    gRandom->Rannor(xyz[0], xyz[1]);
    gRandom->Rannor(px, py);
    pz = px * px + py * py;
    xyz[2] = i + 1;
    ij[0] = i;
    ij[1] = i + 1;
    random = gRandom->Rndm();
    ev = i + 1;
    t1.Fill();
  }
}

static void BM_TTreeWriting(benchmark::State& state)
{
  using namespace o2::framework;

  for (auto _ : state) {
    /// Create a simple TTree
    {
    TTree t1("t1", "a simple Tree with simple variables");
    fillTTree(t1, state.range(0));
    }
  }
  state.SetBytesProcessed(state.iterations() * 44* state.range(0));
}



static void BM_TableBuilding(benchmark::State& state)
{
  using namespace o2::framework;
  /// Create a simple TTree
  TTree t1("t1", "a simple Tree with simple variables");
  Float_t xyz[3];
  Int_t ij[2];
  Float_t px, py, pz;
  Double_t random;
  Int_t ev;
  t1.Branch("px", &px, "px/F");
  t1.Branch("py", &py, "py/F");
  t1.Branch("pz", &pz, "pz/F");
  t1.Branch("random", &random, "random/D");
  t1.Branch("ev", &ev, "ev/I");
  t1.Branch("xyz", xyz, "xyz[3]/F");
  t1.Branch("ij", ij, "ij[2]/I");
  //fill the tree
  for (Int_t i = 0; i < state.range(0); i++) {
    gRandom->Rannor(xyz[0], xyz[1]);
    gRandom->Rannor(px, py);
    pz = px * px + py * py;
    xyz[2] = i + 1;
    ij[0] = i;
    ij[1] = i + 1;
    random = gRandom->Rndm();
    ev = i + 1;
    t1.Fill();
  }

  // Create an arrow table from this.
  TTreeReader reader(&t1);
  TTreeReaderArray<float> xyzReader(reader, "xyz");
  TTreeReaderArray<int> ijkReader(reader, "ij");
  TTreeReaderValue<float> pxReader(reader, "px");
  TTreeReaderValue<float> pyReader(reader, "py");
  TTreeReaderValue<float> pzReader(reader, "pz");
  TTreeReaderValue<double> randomReader(reader, "random");
  TTreeReaderValue<int> evReader(reader, "ev");

  for (auto _ : state) {
    TableBuilder builder;
    RootTableBuilderHelpers::convertTTree(builder, reader, xyzReader, ijkReader, pxReader, pyReader, pzReader, randomReader, evReader);
    auto table = builder.finalize();
  }
  state.SetBytesProcessed(state.iterations() * 44* state.range(0));
}

static void BM_VectorSumBaseline(benchmark::State& state) {
  using namespace o2::framework;
  std::vector<float> px(state.range(0));
  std::vector<float> py(state.range(0));
  std::vector<float> ssum(state.range(0));
  std::random_device rd;  //Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<> dis(1.0, 2.0);

  for (size_t i = 0; i < state.range(0); ++i) {
    px[i] = dis(gen);
    py[i] = dis(gen);
    ssum[i] = 0;
  }
  for (auto _ : state) {
    for (size_t i = 0; i < state.range(0); ++i) {
       benchmark::DoNotOptimize(ssum[i] = px[i]*px[i] + py[i]*py[i]);
    }
  }
  state.SetBytesProcessed(state.iterations() * 8 * state.range(0));
}

static void BM_DataFrameSimpleSum(benchmark::State& state) {
  using namespace o2::framework;
  /// Create a simple TTree
  TTree t1("t1", "a simple Tree with simple variables");
  Float_t xyz[3];
  Int_t ij[2];
  Float_t px, py, pz;
  Double_t random;
  Int_t ev;
  t1.Branch("px", &px, "px/F");
  t1.Branch("py", &py, "py/F");
  t1.Branch("pz", &pz, "pz/F");
  t1.Branch("random", &random, "random/D");
  t1.Branch("ev", &ev, "ev/I");
  t1.Branch("xyz", xyz, "xyz[3]/F");
  t1.Branch("ij", ij, "ij[2]/I");
  //fill the tree
  for (Int_t i = 0; i < state.range(0); i++) {
    gRandom->Rannor(xyz[0], xyz[1]);
    gRandom->Rannor(px, py);
    pz = px * px + py * py;
    xyz[2] = i + 1;
    ij[0] = i;
    ij[1] = i + 1;
    random = gRandom->Rndm();
    ev = i + 1;
    t1.Fill();
  }

  size_t i = 0;
  char buffer[16];
  ROOT::RDataFrame df(t1);
  for (auto _ : state) {
    snprintf(buffer, 16, "ssum%zu", ++i);
    *df.Define(buffer, [](float px, float py) { return px*px + py+py; }, {"px", "py"}).Count();
  }
  state.SetBytesProcessed(state.iterations() * 8 * state.range(0));
}

static void BM_DataFrameArrowSimpleSum(benchmark::State& state) {
  using namespace o2::framework;
  /// Create a simple TTree
  TTree t1("t1", "a simple Tree with simple variables");
  Float_t xyz[3];
  Int_t ij[2];
  Float_t px, py, pz;
  Double_t random;
  Int_t ev;
  t1.Branch("px", &px, "px/F");
  t1.Branch("py", &py, "py/F");
  t1.Branch("pz", &pz, "pz/F");
  t1.Branch("random", &random, "random/D");
  t1.Branch("ev", &ev, "ev/I");
  t1.Branch("xyz", xyz, "xyz[3]/F");
  t1.Branch("ij", ij, "ij[2]/I");
  //fill the tree
  for (Int_t i = 0; i < state.range(0); i++) {
    gRandom->Rannor(xyz[0], xyz[1]);
    gRandom->Rannor(px, py);
    pz = px * px + py * py;
    xyz[2] = i + 1;
    ij[0] = i;
    ij[1] = i + 1;
    random = gRandom->Rndm();
    ev = i + 1;
    t1.Fill();
  }

  // Create an arrow table from this.
  TTreeReader reader(&t1);
  TTreeReaderArray<float> xyzReader(reader, "xyz");
  TTreeReaderArray<int> ijkReader(reader, "ij");
  TTreeReaderValue<float> pxReader(reader, "px");
  TTreeReaderValue<float> pyReader(reader, "py");
  TTreeReaderValue<float> pzReader(reader, "pz");
  TTreeReaderValue<double> randomReader(reader, "random");
  TTreeReaderValue<int> evReader(reader, "ev");
  TableBuilder builder;
  RootTableBuilderHelpers::convertTTree(builder, reader, xyzReader, ijkReader, pxReader, pyReader, pzReader, randomReader, evReader);
  auto table = builder.finalize();
  auto source  = std::make_unique<ROOT::RDF::RArrowDS>(table, std::vector<std::string>{"px", "py"});

  ROOT::RDataFrame df(std::move(source));
  size_t i = 0;
  char buffer[16];
  for (auto _ : state) {
    snprintf(buffer, 16, "ssum%zu", ++i);
    *df.Define(buffer, [](float px, float py) { return px*px + py+py; }, {"px", "py"}).Count();
  }
  state.SetBytesProcessed(state.iterations() * 8 * state.range(0));
}

BENCHMARK(BM_VectorSumBaseline)->RangeMultiplier(2)->Range(1 << 20, 1 << 24);
BENCHMARK(BM_DataFrameArrowSimpleSum)->RangeMultiplier(2)->Range(1 << 20, 1 << 24);
BENCHMARK(BM_DataFrameSimpleSum)->RangeMultiplier(2)->Range(1 << 20, 1 << 24);
BENCHMARK(BM_TTreeWriting)->RangeMultiplier(2)->Range(1 << 20, 1 << 24);
BENCHMARK(BM_TableBuilding)->RangeMultiplier(2)->Range(1 << 20, 1 << 24);

BENCHMARK_MAIN()
