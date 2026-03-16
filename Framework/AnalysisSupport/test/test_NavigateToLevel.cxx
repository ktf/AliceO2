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

#include <catch_amalgamated.hpp>

#include "../src/DataInputDirector.h"

using namespace o2::framework;

// Tests for DataInputDirectorContext::levelForOrigin

TEST_CASE("levelForOrigin empty mapping")
{
  DataInputDirectorContext ctx;
  CHECK(ctx.levelForOrigin("AOD") == -1);
  CHECK(ctx.levelForOrigin("DYN") == -1);
}

TEST_CASE("levelForOrigin single entry")
{
  DataInputDirectorContext ctx;
  ctx.parentLevelToOrigin = {{"DYN", 1}};
  CHECK(ctx.levelForOrigin("DYN") == 1);
  CHECK(ctx.levelForOrigin("AOD") == -1);
}

TEST_CASE("levelForOrigin multiple entries")
{
  DataInputDirectorContext ctx;
  ctx.parentLevelToOrigin = {{"DYN", 1}, {"EMB", 2}, {"EXT", 1}};
  CHECK(ctx.levelForOrigin("DYN") == 1);
  CHECK(ctx.levelForOrigin("EMB") == 2);
  CHECK(ctx.levelForOrigin("EXT") == 1);
  CHECK(ctx.levelForOrigin("AOD") == -1);
  CHECK(ctx.levelForOrigin("") == -1);
}

// Tests for DataInputDescriptor::navigateToLevel

TEST_CASE("navigateToLevel returns null with no input files")
{
  // With no input files, setFile will fail immediately and navigateToLevel
  // must return {nullptr, -1} without crashing.
  DataInputDirectorContext ctx;
  ctx.allowedParentLevel = 2;
  DataInputDescriptor desc(false, 0, ctx);

  auto [parentFile, parentNumTF] = desc.navigateToLevel(0, 0, 1, "DYN");
  CHECK(parentFile == nullptr);
  CHECK(parentNumTF == -1);
}
