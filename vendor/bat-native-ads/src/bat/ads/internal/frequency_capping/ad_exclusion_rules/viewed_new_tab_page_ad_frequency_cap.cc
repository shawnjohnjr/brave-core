/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/ad_exclusion_rules/viewed_new_tab_page_ad_frequency_cap.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/logging.h"

namespace ads {

namespace {
const int kViewedNewTabPageAdFrequencyCap = 1;
}  // namespace

ViewedNewTabPageAdFrequencyCap::ViewedNewTabPageAdFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

ViewedNewTabPageAdFrequencyCap::~ViewedNewTabPageAdFrequencyCap() = default;

bool ViewedNewTabPageAdFrequencyCap::ShouldExclude(
    const AdInfo& ad,
    const AdEventList& ad_events) {
  const AdEventList filtered_ad_events = FilterAdEvents(ad_events, ad);

  if (!DoesRespectCap(filtered_ad_events, ad)) {
    last_message_ = base::StringPrintf("uuid %s has exceeded the "
        "frequency capping for new tab page ad", ad.uuid.c_str());
    return true;
  }

  return false;
}

std::string ViewedNewTabPageAdFrequencyCap::get_last_message() const {
  return last_message_;
}

bool ViewedNewTabPageAdFrequencyCap::DoesRespectCap(
    const AdEventList& ad_events,
    const AdInfo& ad) {
  if (ad_events.size() >= kViewedNewTabPageAdFrequencyCap) {
    return false;
  }

  return true;
}

AdEventList ViewedNewTabPageAdFrequencyCap::FilterAdEvents(
    const AdEventList& ad_events,
    const AdInfo& ad) const {
  AdEventList filtered_ad_events = ad_events;

  const auto iter = std::remove_if(filtered_ad_events.begin(),
      filtered_ad_events.end(), [&ad](const AdEventInfo& ad_event) {
    return ad_event.uuid != ad.uuid ||
        ad_event.confirmation_type != ConfirmationType::kViewed ||
        ad_event.ad_type != AdType::kNewTabPageAd;
  });
  filtered_ad_events.erase(iter, filtered_ad_events.end());

  return filtered_ad_events;
}

}  // namespace ads
