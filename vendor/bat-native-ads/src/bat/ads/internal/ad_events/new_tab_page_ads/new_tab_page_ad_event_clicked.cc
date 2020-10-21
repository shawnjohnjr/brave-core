/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad_event_clicked.h"

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/logging.h"

namespace ads {

namespace {
const ConfirmationType kConfirmationType = ConfirmationType::kClicked;
}  // namespace

NewTabPageAdEventClicked::NewTabPageAdEventClicked(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

NewTabPageAdEventClicked::~NewTabPageAdEventClicked() = default;

void NewTabPageAdEventClicked::Trigger(
    const NewTabPageAdInfo& ad) {
  BLOG(3, "Clicked new tab page ad with uuid " << ad.uuid
      << " and creative instance id " << ad.creative_instance_id);

  ads_->set_last_clicked_ad(ad);

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
      BLOG(1, "Failed to log new tab page ad clicked event");
    }
  });

  ads_->AppendNewTabPageAdToHistory(ad, kConfirmationType);

  ads_->get_confirmations()->ConfirmAd(ad.creative_instance_id,
      kConfirmationType);
}

}  // namespace ads
