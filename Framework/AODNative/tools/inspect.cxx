// Copyright 2019-2026 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

// o2-aod-native-inspect <file> [--json]
//
// Prints the file header, every top-level key and, per DF and tree, the
// branch layout and basket locations, using only the ROOT-free reader.

#include "AODNative/Container.h"
#include "AODNative/Errors.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

using namespace o2::aod::native;

namespace
{
std::string quote(std::string const& s)
{
  std::string out = "\"";
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') {
      out += '\\';
      out += (char)c;
    } else if (c < 0x20) {
      char buf[8];
      std::snprintf(buf, sizeof(buf), "\\u%04x", c);
      out += buf;
    } else {
      out += (char)c;
    }
  }
  return out + "\"";
}

void printJSON(RootFile& file)
{
  auto& h = file.header();
  auto& out = std::cout;
  out << "{\n  \"header\": {"
      << "\"version\": " << h.version << ", \"begin\": " << h.begin << ", \"end\": " << h.end
      << ", \"seekFree\": " << h.seekFree << ", \"nbytesFree\": " << h.nbytesFree << ", \"nfree\": " << h.nfree
      << ", \"nbytesName\": " << h.nbytesName << ", \"units\": " << (int)h.units << ", \"compress\": " << h.compress
      << ", \"seekInfo\": " << h.seekInfo << ", \"nbytesInfo\": " << h.nbytesInfo << "},\n";
  out << "  \"keys\": [";
  bool first = true;
  for (auto& k : file.topKeys()) {
    out << (first ? "\n" : ",\n") << "    {\"name\": " << quote(k.name) << ", \"class\": " << quote(k.className)
        << ", \"cycle\": " << k.cycle << ", \"seek\": " << k.seekKey << ", \"nbytes\": " << k.nbytes << ", \"objlen\": " << k.objlen << "}";
    first = false;
  }
  out << "\n  ],\n  \"dataFrames\": [";
  bool firstDF = true;
  for (auto& df : file.dataFrames()) {
    out << (firstDF ? "\n" : ",\n") << "    {\"name\": " << quote(df) << ", \"trees\": [";
    firstDF = false;
    bool firstTree = true;
    for (auto& key : file.keysOf(df)) {
      if (key.className != "TTree") {
        continue;
      }
      auto tree = file.readTree(key);
      out << (firstTree ? "\n" : ",\n") << "      {\"name\": " << quote(tree.name) << ", \"entries\": " << tree.entries
          << ", \"totBytes\": " << tree.totBytes << ", \"zipBytes\": " << tree.zipBytes << ", \"branches\": [";
      firstTree = false;
      bool firstBranch = true;
      for (auto& b : tree.branches) {
        out << (firstBranch ? "\n" : ",\n") << "        {\"name\": " << quote(b.name) << ", \"leaf\": \"TLeaf" << b.leafClass
            << "\", \"len\": " << b.fixedLen << ", \"isUnsigned\": " << (b.isUnsigned ? "true" : "false")
            << ", \"countBranch\": " << quote(b.countBranch) << ", \"entries\": " << b.entries
            << ", \"totBytes\": " << b.totBytes << ", \"zipBytes\": " << b.zipBytes << ", \"baskets\": [";
        firstBranch = false;
        bool firstBasket = true;
        for (auto& basket : b.baskets) {
          out << (firstBasket ? "" : ", ") << "[" << basket.seek << ", " << basket.nbytes << ", " << basket.firstEntry
              << ", " << basket.nEntries << ", " << (basket.embedded ? "true" : "false") << "]";
          firstBasket = false;
        }
        out << "]}";
      }
      out << "\n      ]}";
    }
    out << "\n    ]}";
  }
  out << "\n  ]\n}\n";
}

void printText(RootFile& file)
{
  auto& h = file.header();
  std::printf("ROOT file version %d, begin %d, end %llu, compression %d, %s seeks\n", h.version, h.begin,
              (unsigned long long)h.end, h.compress, h.version >= 1000000 ? "64-bit" : "32-bit");
  std::printf("\n%-32s %-16s %5s %14s %10s %10s\n", "key", "class", "cycle", "seek", "nbytes", "objlen");
  for (auto& k : file.topKeys()) {
    std::printf("%-32s %-16s %5d %14llu %10u %10u\n", k.name.c_str(), k.className.c_str(), k.cycle,
                (unsigned long long)k.seekKey, k.nbytes, k.objlen);
  }
  for (auto& df : file.dataFrames()) {
    std::printf("\n%s\n", df.c_str());
    for (auto& key : file.keysOf(df)) {
      if (key.className != "TTree") {
        std::printf("  %s (%s, skipped)\n", key.name.c_str(), key.className.c_str());
        continue;
      }
      auto tree = file.readTree(key);
      std::printf("  %s: %lld entries, %lld bytes, %lld compressed\n", tree.name.c_str(), (long long)tree.entries,
                  (long long)tree.totBytes, (long long)tree.zipBytes);
      for (auto& b : tree.branches) {
        std::printf("    %-28s TLeaf%c%s len %d%s%s, %zu baskets:", b.name.c_str(), b.leafClass, b.isUnsigned ? " unsigned" : "",
                    b.fixedLen, b.countBranch.empty() ? "" : ", count ", b.countBranch.c_str(), b.baskets.size());
        for (auto& basket : b.baskets) {
          if (basket.embedded) {
            std::printf(" [embedded %lld+%d]", (long long)basket.firstEntry, basket.nEntries);
          } else {
            std::printf(" [%llu:%u %lld+%d]", (unsigned long long)basket.seek, basket.nbytes, (long long)basket.firstEntry, basket.nEntries);
          }
        }
        std::printf("\n");
      }
    }
  }
}
} // namespace

int main(int argc, char** argv)
{
  std::string path;
  bool json = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--json") == 0) {
      json = true;
    } else if (path.empty()) {
      path = argv[i];
    } else {
      path.clear();
      break;
    }
  }
  if (path.empty()) {
    std::fprintf(stderr, "usage: %s <file> [--json]\n", argv[0]);
    return 2;
  }
  try {
    RootFile file{ByteSource::open(path, Options{})};
    if (json) {
      printJSON(file);
    } else {
      printText(file);
    }
  } catch (FormatError const& e) {
    std::cout.flush();
    std::fprintf(stderr, "%s: %s\n", argv[0], e.what());
    return 1;
  }
  return 0;
}
