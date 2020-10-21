/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_AD_EXCLUSION_RULES_NEW_TAB_PAGE_AD_WALLPAPER_CAP_FREQUENCY_CAP_H_  // NOLINT
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_AD_EXCLUSION_RULES_NEW_TAB_PAGE_AD_WALLPAPER_CAP_FREQUENCY_CAP_H_  // NOLINT

#include <string>

#include "bat/ads/internal/frequency_capping/ad_exclusion_rules/ad_exclusion_rule.h"

namespace ads {

class AdsImpl;

class ViewedNewTabPageAdFrequencyCap : public AdExclusionRule {
 public:
  ViewedNewTabPageAdFrequencyCap(
      const AdsImpl* const ads);

  ~ViewedNewTabPageAdFrequencyCap() override;

  ViewedNewTabPageAdFrequencyCap(
      const ViewedNewTabPageAdFrequencyCap&) = delete;
  ViewedNewTabPageAdFrequencyCap& operator=(
      const ViewedNewTabPageAdFrequencyCap&) = delete;

  bool ShouldExclude(
      const AdInfo& ad,
      const AdEventList& ad_events) override;

  std::string get_last_message() const override;

 private:
  const AdsImpl* const ads_;  // NOT OWNED

  std::string last_message_;

  bool DoesRespectCap(
      const AdEventList& ad_events,
      const AdInfo& ad);

  AdEventList FilterAdEvents(
      const AdEventList& ad_events,
      const AdInfo& ad) const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_AD_EXCLUSION_RULES_NEW_TAB_PAGE_AD_WALLPAPER_CAP_FREQUENCY_CAP_H_  // NOLINT
