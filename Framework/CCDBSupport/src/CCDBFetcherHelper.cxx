// Copyright 2019-2025 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#include "CCDBFetcherHelper.h"

namespace o2::framework
{

o2::ccdb::CcdbApi& CCDBFetcherHelper::getAPI(const std::string& path)
{
  // find the first = sign in the string. If present drop everything after it
  // and between it and the previous /.
  auto pos = path.find('=');
  if (pos == std::string::npos) {
    auto entry = remappings.find(path);
    return apis[entry == remappings.end() ? "" : entry->second];
  }
  auto pos2 = path.rfind('/', pos);
  if (pos2 == std::string::npos || pos2 == pos - 1 || pos2 == 0) {
    throw runtime_error_f("Malformed path %s", path.c_str());
  }
  auto entry = remappings.find(path.substr(0, pos2));
  return apis[entry == remappings.end() ? "" : entry->second];
}
} // o2::framework
