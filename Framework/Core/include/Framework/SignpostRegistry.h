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

#include <string>

using o2_log_t = struct _o2_log_t*;

namespace o2::framework
{
enum struct SignpostKind : int {
  Normal = 0,
  Dynamic,
  Stacktrace,
};

enum struct SignpostId : int {
  Test = 0,
};

struct SignpostSpec {
  enum SignpostKind kind = SignpostKind::Normal;
  std::string name = "unnamed";
};

class SignpostRegistry
{
 public:
  void registerSignpost(SignpostSpec const& signpost);

  o2_log_t& get(enum SignpostId id)
  {
    return mSignposts[(int)id];
  }

 private:
  std::vector<SignpostSpec> mSignpostSpecs;
  std::array<o2_log_t, 100> mSignposts;
};

} // namespace o2::framework
