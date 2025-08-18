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
#ifndef O2_FRAMEWORK_CCDBHELPERS_H_
#define O2_FRAMEWORK_CCDBHELPERS_H_

#include "Framework/AlgorithmSpec.h"
#include "Framework/OutputRoute.h"
#include "Framework/DataAllocator.h"
#include "CCDB/CcdbApi.h"
#include <unordered_map>
#include <string>

namespace o2::framework
{

struct CCDBFetcherHelper {
  struct CCDBCacheInfo {
    std::string etag;
    size_t cacheValidUntil = 0;
    size_t cachePopulatedAt = 0;
    size_t cacheMiss = 0;
    size_t cacheHit = 0;
    size_t minSize = -1ULL;
    size_t maxSize = 0;
    int lastCheckedTF = 0;
  };

  struct RemapMatcher {
    std::string path;
  };

  struct RemapTarget {
    std::string url;
  };

  std::unordered_map<std::string, CCDBCacheInfo> mapURL2UUID;
  std::unordered_map<std::string, DataAllocator::CacheId> mapURL2DPLCache;
  std::string createdNotBefore = "0";
  std::string createdNotAfter = "3385078236000";
  std::unordered_map<std::string, o2::ccdb::CcdbApi> apis;
  std::vector<OutputRoute> routes;
  std::unordered_map<std::string, std::string> remappings;
  uint32_t lastCheckedTFCounterOrbReset = 0; // last checkecked TFcounter for bulk check
  int queryPeriodGlo = 1;
  int queryPeriodFactor = 1;
  int64_t timeToleranceMS = 5000;

  o2::ccdb::CcdbApi& getAPI(const std::string& path);
};

struct CCDBHelpers {
  struct ParserResult {
    std::unordered_map<std::string, std::string> remappings;
    std::string error;
  };
  static AlgorithmSpec fetchFromCCDB();
  static ParserResult parseRemappings(char const*);
};

} // namespace o2::framework

#endif // O2_FRAMEWORK_CCDBHELPERS_H_
