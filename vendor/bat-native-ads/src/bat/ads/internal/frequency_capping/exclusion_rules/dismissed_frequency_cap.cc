/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/dismissed_frequency_cap.h"

#include <stdint.h>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/sorts/ads_history/ads_history_sort_factory.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

DismissedFrequencyCap::DismissedFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

DismissedFrequencyCap::~DismissedFrequencyCap() = default;

bool DismissedFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad,
    const AdEventList& ad_events) {
  const AdEventList filtered_ad_events = FilterAdEvents(ad_events, ad);

  if (!DoesRespectCap(filtered_ad_events, ad)) {
    last_message_ = base::StringPrintf("campaignId %s has exceeded the "
        "frequency capping for dismissed", ad.campaign_id.c_str());
    return true;
  }

  return false;
}

std::string DismissedFrequencyCap::get_last_message() const {
  return last_message_;
}

bool DismissedFrequencyCap::DoesRespectCap(
    const AdEventList& ad_events,
    const CreativeAdInfo& ad) {
  int count = 0;

  for (const auto& ad_event : ad_events) {
    if (ad_event.confirmation_type == ConfirmationType::kClicked) {
      count = 0;
    } else if (ad_event.confirmation_type == ConfirmationType::kDismissed) {
      count++;
    }
  }

  if (count >= 2) {
    // An ad was dismissed two or more times in a row without being clicked, so
    // do not show another ad from the same campaign for 48 hours
    return false;
  }

  return true;
}

AdEventList DismissedFrequencyCap::FilterAdEvents(
    const AdEventList& ad_events,
    const CreativeAdInfo& ad) const {
  const uint64_t time_constraint =
      2 * base::Time::kSecondsPerHour * base::Time::kHoursPerDay;

  const uint64_t now = base::Time::Now().ToDoubleT();

  AdEventList filtered_ad_events = ad_events;

  const auto iter = std::remove_if(filtered_ad_events.begin(),
      filtered_ad_events.end(), [&](const AdEventInfo& ad_event) {
    return ad_event.campaign_id != ad.campaign_id ||
        now - ad_event.timestamp >= time_constraint;
  });
  filtered_ad_events.erase(iter, filtered_ad_events.end());

  return filtered_ad_events;
}

}  // namespace ads
