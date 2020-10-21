/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad_event_viewed.h"

#include <utility>

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/frequency_capping/ad_exclusion_rules/viewed_new_tab_page_ad_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/frequency_capping/permission_rules/new_tab_page_ads_per_day_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/new_tab_page_ads_per_hour_frequency_cap.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

namespace {
const ConfirmationType kConfirmationType = ConfirmationType::kViewed;
}  // namespace

NewTabPageAdEventViewed::NewTabPageAdEventViewed(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

NewTabPageAdEventViewed::~NewTabPageAdEventViewed() = default;

void NewTabPageAdEventViewed::Trigger(
    const NewTabPageAdInfo& ad) {
  const auto permission_rules = CreatePermissionRules();
  if (!ads_->IsAdAllowed(permission_rules)) {
    BLOG(1, "New tab page ad: Not allowed based on history");
    return;
  }

  database::table::AdEvents ad_events_database_table(ads_);
  ad_events_database_table.GetAll([=](
      const Result result,
      const AdEventList ad_events) {
    if (result != Result::SUCCESS) {
      BLOG(1, "New tab page ad: Failed to get ad events");
      return;
    }

    ViewedNewTabPageAdFrequencyCap frequency_cap(ads_);
    if (frequency_cap.ShouldExclude(ad, ad_events)) {
      if (!frequency_cap.get_last_message().empty()) {
        BLOG(2, frequency_cap.get_last_message());
      }

      BLOG(1, "New tab page ad: Not allowed based on history");
      return;
    }

    BLOG(3, "Viewed new tab page ad with uuid " << ad.uuid
        << " and creative instance id " << ad.creative_instance_id);

    AdEventInfo ad_event;
    ad_event.uuid = ad.uuid;
    ad_event.creative_instance_id = ad.creative_instance_id;
    ad_event.creative_set_id = ad.creative_set_id;
    ad_event.campaign_id = ad.campaign_id;
    ad_event.timestamp = static_cast<int64_t>(base::Time::Now().ToDoubleT());
    ad_event.confirmation_type = kConfirmationType;
    ad_event.ad_type = ad.type;
    database::table::AdEvents ad_events_database_table(ads_);
    ad_events_database_table.LogEvent(ad_event, [](
        const Result result) {
      if (result != Result::SUCCESS) {
        BLOG(1, "Failed to log new tab page ad viewed event");
      }
    });

    ads_->get_confirmations()->ConfirmAd(ad.creative_instance_id,
        kConfirmationType);
  });
}

std::vector<std::unique_ptr<PermissionRule>>
NewTabPageAdEventViewed::CreatePermissionRules() const {
  std::vector<std::unique_ptr<PermissionRule>> permission_rules;

  std::unique_ptr<PermissionRule> ads_per_hour_frequency_cap =
      std::make_unique<NewTabPageAdsPerHourFrequencyCap>(ads_);
  permission_rules.push_back(std::move(ads_per_hour_frequency_cap));

  std::unique_ptr<PermissionRule> ads_per_day_frequency_cap =
      std::make_unique<NewTabPageAdsPerDayFrequencyCap>(ads_);
  permission_rules.push_back(std::move(ads_per_day_frequency_cap));

  return permission_rules;
}

}  // namespace ads
