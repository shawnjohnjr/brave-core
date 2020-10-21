/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_day_frequency_cap.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

PerDayFrequencyCap::PerDayFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

PerDayFrequencyCap::~PerDayFrequencyCap() = default;

bool PerDayFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad,
    const AdEventList& ad_events) {
  const AdEventList filtered_ad_events = FilterAdEvents(ad_events, ad);

  if (!DoesRespectCap(filtered_ad_events, ad)) {
    last_message_ = base::StringPrintf("creativeSetId %s has exceeded the "
        "frequency capping for perDay", ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string PerDayFrequencyCap::get_last_message() const {
  return last_message_;
}

bool PerDayFrequencyCap::DoesRespectCap(
    const AdEventList& ad_events,
    const CreativeAdInfo& ad) {
  const std::deque<uint64_t> history =
      GetTimestampHistoryForAdEvents(ad_events);

  const uint64_t time_constraint =
      base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

  return DoesHistoryRespectCapForRollingTimeConstraint(
      history, time_constraint, ad.per_day);
}

AdEventList PerDayFrequencyCap::FilterAdEvents(
    const AdEventList& ad_events,
    const CreativeAdInfo& ad) const {
  AdEventList filtered_ad_events = ad_events;

  const auto iter = std::remove_if(filtered_ad_events.begin(),
      filtered_ad_events.end(), [&ad](const AdEventInfo& ad_event) {
    return ad_event.creative_set_id != ad.creative_set_id ||
        ad_event.confirmation_type != ConfirmationType::kViewed;
  });
  filtered_ad_events.erase(iter, filtered_ad_events.end());

  return filtered_ad_events;
}

}  // namespace ads
