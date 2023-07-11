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

#include "Framework/ResourcePolicyHelpers.h"
#include "Framework/DeviceSpec.h"
#include "ResourcesMonitoringHelper.h"
#include "Framework/Signpost.h"

#include <string>
#include <regex>

#define O2_FORCE_LOGGER_SIGNPOST
O2_DECLARE_DYNAMIC_LOG(resource_policies);

namespace o2::framework
{

/// A trivial task is a task which will execute regardless of
/// the resources available.
ResourcePolicy ResourcePolicyHelpers::trivialTask(char const* s)
{
  return ResourcePolicy{
    "trivial",
    [matcher = std::regex(s)](DeviceSpec const& spec) -> bool {
      return std::regex_match(spec.name, matcher);
    },
    [](ComputingQuotaOffer const&, ComputingQuotaOffer const&) -> OfferScore { return OfferScore::Enough; }};
}

ResourcePolicy ResourcePolicyHelpers::cpuBoundTask(char const* s, int requestedCPUs)
{
  return ResourcePolicy{
    "cpu-bound",
    [matcher = std::regex(s)](DeviceSpec const& spec) -> bool {
      return std::regex_match(spec.name, matcher);
    },
    [requestedCPUs](ComputingQuotaOffer const& offer, ComputingQuotaOffer const& accumulated) -> OfferScore { return accumulated.cpu >= requestedCPUs ? OfferScore::Enough : OfferScore::More; }};
}

ResourcePolicy ResourcePolicyHelpers::sharedMemoryBoundTask(char const* s, int requestedSharedMemory)
{
  return ResourcePolicy{
    "shm-bound",
    [matcher = std::regex(s)](DeviceSpec const& spec) -> bool {
      bool matches = std::regex_match(spec.name, matcher);
      if (matches) {
        O2_LOG_ENABLE_DYNAMIC(resource_policies);
      }
      return matches;
    },
    [requestedSharedMemory](ComputingQuotaOffer const& offer, ComputingQuotaOffer const& accumulated) -> OfferScore { 
	    O2_LOG_ENABLE_DYNAMIC(resource_policies);
	    O2_SIGNPOST_ID_GENERATE(rid, resource_policies);
	    O2_SIGNPOST_START(resource_policies, rid, "shm-bound-selector", "Starting selection process");
      if (offer.sharedMemory == 0) {
	      O2_SIGNPOST_EVENT_EMIT(resource_policies, rid, "shm-bound-selector", "No shared memory in offer.");
	    O2_SIGNPOST_END(resource_policies, rid, "shm-bound-selector", "Ending selection process");
        return OfferScore::Unneeded;
      }
      bool enough = accumulated.sharedMemory >= requestedSharedMemory;
      if (enough) {
      O2_SIGNPOST_EVENT_EMIT(resource_policies, rid, "shm-bound-selector", "Enough Requested shared memory: required %" PRIu64 " / provided %" PRIu64, requestedSharedMemory, accumulated.sharedMemory);
	    O2_SIGNPOST_END(resource_policies, rid, "shm-bound-selector", "Ending selection process");
	      return OfferScore::Enough;
      }
      O2_SIGNPOST_EVENT_EMIT(resource_policies, rid, "shm-bound-selector", "Not enough shared memory: required %" PRIu64 " / provided %" PRIu64, requestedSharedMemory, accumulated.sharedMemory);
	    O2_SIGNPOST_END(resource_policies, rid, "shm-bound-selector", "Ending selection process");
	      return OfferScore::More; }};
}

} // namespace o2::framework
