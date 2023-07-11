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

#include "Framework/ComputingQuotaEvaluator.h"
#include "Framework/DataProcessingStats.h"
#include "Framework/ServiceRegistryRef.h"
#include "Framework/DeviceState.h"
#include "Framework/DriverClient.h"
#include "Framework/Monitoring.h"
#include "Framework/Logger.h"
#include "Framework/DeviceSpec.h"
#include <Monitoring/Monitoring.h>
#define O2_FORCE_LOGGER_SIGNPOST
#include "Framework/Signpost.h"

#include <vector>
#include <uv.h>
#include <cassert>

O2_DECLARE_DYNAMIC_LOG(quota_evaluator);

namespace o2::framework
{

ComputingQuotaEvaluator::ComputingQuotaEvaluator(ServiceRegistryRef ref)
  : mRef(ref)
{
  auto& spec = mRef.get<DeviceSpec const>();

  if (spec.name == "internal-dpl-aod-reader") {
    O2_LOG_ENABLE_DYNAMIC(quota_evaluator);
  }
  auto& state = mRef.get<DeviceState>();
  // The first offer is valid, but does not contain any resource
  // so this will only work with some device which does not require
  // any CPU. Notice this will have troubles if a given DPL process
  // runs for more than a year.
  mOffers[0] = {
    0,
    0,
    0,
    -1,
    -1,
    OfferScore::Unneeded,
    true};
  mInfos[0] = {
    uv_now(state.loop),
    0,
    0};

  // Creating a timer to check for expired offers
  mTimer = (uv_timer_t*)malloc(sizeof(uv_timer_t));
  uv_timer_init(state.loop, mTimer);
}

struct QuotaEvaluatorStats {
  std::vector<int> invalidOffers;
  std::vector<int> otherUser;
  std::vector<int> unexpiring;
  std::vector<int> selectedOffers;
  std::vector<int> expired;
};

// Ugly hack. I should probably think of something better.
std::string asString(std::vector<int> const& v)
{
  return fmt::format("{}", fmt::join(v, ", "));
}

bool ComputingQuotaEvaluator::selectOffer(int task, ComputingQuotaRequest const& selector, uint64_t now)
{
  O2_SIGNPOST_ID_GENERATE(oid, quota_evaluator);
  O2_SIGNPOST_START(quota_evaluator, oid, "selectOffer", "starting selection process for task %d at %" PRIu64, task, now);

  auto selectOffer = [&offers = this->mOffers, &infos = this->mInfos, task](int ref, uint64_t now) {
    auto& selected = offers[ref];
    auto& info = infos[ref];
    selected.user = task;
    if (info.firstUsed == 0) {
      info.firstUsed = now;
    }
    info.lastUsed = now;
  };

  ComputingQuotaOffer accumulated;
  static QuotaEvaluatorStats stats;

  stats.invalidOffers.clear();
  stats.otherUser.clear();
  stats.unexpiring.clear();
  stats.selectedOffers.clear();
  stats.expired.clear();

  auto summarizeWhatHappended = [ref = mRef, oid](bool enough, std::vector<int> const& result, ComputingQuotaOffer const& totalOffer, QuotaEvaluatorStats& stats) -> bool {
    auto& dpStats = ref.get<DataProcessingStats>();
    if (result.size() == 1 && result[0] == 0) {
      O2_SIGNPOST_END(quota_evaluator, oid, "selectOffer", "No resource requested. Scheduling.");
      return enough;
    }
    if (enough) {
      O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "selection_summary",
                             "%" PRIu64 " offers were selected for a total of: cpu %d, memory %" PRIu64 ", shared memory %" PRIu64,
                             (uint64_t)result.size(), totalOffer.cpu, totalOffer.memory, totalOffer.sharedMemory);
      O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "selection_summary", "The following offers were selected for computation: %s", asString(result).data());
      dpStats.updateStats({static_cast<short>(ProcessingStatsId::RESOURCES_SATISFACTORY), DataProcessingStats::Op::Add, 1});
    } else {
      O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "selection_summary",
                             "Not enough resoruces provided.");
      dpStats.updateStats({static_cast<short>(ProcessingStatsId::RESOURCES_MISSING), DataProcessingStats::Op::Add, 1});
      if (result.size()) {
        O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "selection_summary", "Insufficient resources provided by %d offers.",
                               (int)result.size());
        dpStats.updateStats({static_cast<short>(ProcessingStatsId::RESOURCES_INSUFFICIENT), DataProcessingStats::Op::Add, 1});
      }
    }
    if (stats.invalidOffers.size()) {
      O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "selection_summary", "The following offers were invalid: %s.", asString(stats.invalidOffers).data());
    }
    if (stats.otherUser.size()) {
      O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "selection_summary", "The following offers were owned by other users: %s.", asString(stats.otherUser).data());
    }
    if (stats.expired.size()) {
      O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "selection_summary", "The following offers were expired: %s.", asString(stats.expired).data());
    }
    if (stats.unexpiring.size() > 1) {
      O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "selection_summary", "The following offers will never expire: %s.", asString(stats.unexpiring).data());
    }

    O2_SIGNPOST_END(quota_evaluator, oid, "selectOffer", "Scheduling result %d.", enough);
    return enough;
  };

  bool enough = false;
  int64_t minValidity = 0;

  for (int i = 0; i != mOffers.size(); ++i) {
    auto& offer = mOffers[i];
    auto& info = mInfos[i];
    if (enough) {
      break;
    }
    // Ignore:
    // - Invalid offers
    // - Offers which belong to another task
    // - Expired offers
    if (offer.valid == false) {
      stats.invalidOffers.push_back(i);
      continue;
    }
    if (offer.user != -1 && offer.user != task) {
      stats.otherUser.push_back(i);
      continue;
    }
    if (offer.runtime < 0) {
      stats.unexpiring.push_back(i);
    } else if (offer.runtime + info.received < now) {
      O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "offer_selection",
                             "Offer %i expired since %" PRIu64 "ms and holds %d MB",
                             i, offer.runtime + info.received - now, offer.sharedMemory / 1000000);
      mExpiredOffers.push_back(ComputingQuotaOfferRef{i});
      stats.expired.push_back(i);
      continue;
    } else {
      O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "offer_selection",
                             "Offer %i still valid for %" PRIu64 "ms and providing %d MB",
                             i, offer.runtime + info.received - now, offer.sharedMemory / 1000000);
      if (minValidity == 0) {
        minValidity = offer.runtime + info.received - now;
      }
      minValidity = std::min(minValidity,(int64_t)(offer.runtime + info.received - now));
    }
    /// We then check if the offer is suitable
    assert(offer.sharedMemory >= 0);
    auto tmp = accumulated;
    tmp.cpu += offer.cpu;
    tmp.memory += offer.memory;
    tmp.sharedMemory += offer.sharedMemory;
    offer.score = selector(offer, tmp);
    switch (offer.score) {
      case OfferScore::Unneeded:
        continue;
      case OfferScore::Unsuitable:
        continue;
      case OfferScore::More:
        selectOffer(i, now);
        accumulated = tmp;
        stats.selectedOffers.push_back(i);
        continue;
      case OfferScore::Enough:
        selectOffer(i, now);
        accumulated = tmp;
        stats.selectedOffers.push_back(i);
        enough = true;
        break;
    };
  }

  if (minValidity != 0) {
    O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "offer_selection",
                           "Next offer will expire in %" PRIu64 "ms",
                           minValidity);
    mTimer->data = (void*)minValidity;
    uv_timer_start(mTimer, [](uv_timer_t* handle) {
      int64_t minValidity = (int64_t)handle->data;
      O2_SIGNPOST_ID_GENERATE(oid2, quota_evaluator);
      O2_SIGNPOST_START(quota_evaluator, oid2, "offer_selection", "Timer expired after %" PRIi64, minValidity);
      O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid2, "offer_selection",
                             "Offer should be checked again");
      O2_SIGNPOST_END(quota_evaluator, oid2, "offer_selection", "Done Timer expired");
    },
                   minValidity + 100, 0);
  }
  // If we get here it means we never got enough offers, so we return false.
  return summarizeWhatHappended(enough, stats.selectedOffers, accumulated, stats);
}

void ComputingQuotaEvaluator::consume(int id, ComputingQuotaConsumer& consumer, std::function<void(ComputingQuotaOffer const& accumulatedConsumed, ComputingQuotaStats& reportConsumedOffer)>& reportConsumedOffer)
{
  // This will report how much of the offers has to be considered consumed.
  // Notice that actual memory usage might be larger, because we can over
  // allocate.
  consumer(id, mOffers, mStats, reportConsumedOffer);
}

void ComputingQuotaEvaluator::dispose(int taskId)
{
  for (int oi = 0; oi < mOffers.size(); ++oi) {
    auto& offer = mOffers[oi];
    if (offer.user != taskId) {
      continue;
    }
    offer.user = -1;
    // Disposing the offer so that the resource can be recyled.
    /// An offer with index 0 is always there.
    /// All the others are reset.
    if (oi == 0) {
      return;
    }
    if (offer.valid == false) {
      continue;
    }
    if (offer.sharedMemory <= 0) {
      offer.valid = false;
      offer.score = OfferScore::Unneeded;
    }
  }
}

/// Move offers from the pending list to the actual available offers
void ComputingQuotaEvaluator::updateOffers(std::vector<ComputingQuotaOffer>& pending, uint64_t now)
{
  for (size_t oi = 0; oi < mOffers.size(); oi++) {
    auto& storeOffer = mOffers[oi];
    auto& info = mInfos[oi];
    if (pending.empty()) {
      return;
    }
    if (storeOffer.valid == true) {
      continue;
    }
    info.received = now;
    auto& offer = pending.back();
    storeOffer = offer;
    storeOffer.valid = true;
    pending.pop_back();
  }
}

void ComputingQuotaEvaluator::handleExpired(std::function<void(ComputingQuotaOffer const&, ComputingQuotaStats const& stats)> expirator)
{
  O2_SIGNPOST_ID_GENERATE(oid, quota_evaluator);
  O2_SIGNPOST_START(quota_evaluator, oid, "handleExpired", "ComputingQuotaEvaluator::handleExpired");
  if (mExpiredOffers.size()) {
    O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "handleExpired", "Handling %" PRIu64 " expired offers", (uint64_t)mExpiredOffers.size());
  } else {
    O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "handleExpired", "No expired offers");
  }
  /// Whenever an offer is expired, we give back the resources
  /// to the driver.
  for (auto& ref : mExpiredOffers) {
    auto& offer = mOffers[ref.index];
    if (offer.sharedMemory < 0) {
      O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "handleExpired", "Offer %d does not have any more memory", ref.index);
      offer.valid = false;
      offer.score = OfferScore::Unneeded;
      continue;
    }
    // FIXME: offers should go through the driver client, not the monitoring
    // api.
    O2_SIGNPOST_EVENT_EMIT(quota_evaluator, oid, "handleExpired", "Offer %d expired. Giving back " O2_ENG_TYPE("memory-in-bytes", PRIu64) " and %d cores", ref.index, offer.sharedMemory / 1000000, offer.cpu);
    assert(offer.sharedMemory >= 0);
    mStats.totalExpiredBytes += offer.sharedMemory;
    mStats.totalExpiredOffers++;
    expirator(offer, mStats);
    offer.sharedMemory = -1;
    offer.valid = false;
    offer.score = OfferScore::Unneeded;
  }
  mExpiredOffers.clear();
  O2_SIGNPOST_END(quota_evaluator, oid, "handleExpired", "Done ComputingQuotaEvaluator::handleExpired");
}

} // namespace o2::framework
