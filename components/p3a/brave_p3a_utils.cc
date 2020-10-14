/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_utils.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

void MaybeRecordShieldsUsageP3A(ShieldsIconUsage usage,
                                PrefService* local_state) {
  // May be null in tests.
  if (!local_state) {
    return;
  }
  int last_value = local_state->GetInteger(brave::kShieldUsageStatus);
  if (last_value < usage) {
    UMA_HISTOGRAM_ENUMERATION("Brave.Shields.UsageStatus", usage, kSize);
    local_state->SetInteger(brave::kShieldUsageStatus, usage);
  }
}

}  // namespace brave_shields

namespace brave {

void MaybeRecordNTPCustomizeUsageP3A(NTPCustomizeUsage usage,
                                     PrefService* local_state) {
  // May be null in tests.
  if (!local_state) {
    return;
  }
  int last_value = local_state->GetInteger(kNTPCustomizeUsageStatus);
  if (last_value < usage) {
    UMA_HISTOGRAM_ENUMERATION("Brave.NTP.CustomizeUsageStatus", usage,
        kCustomizeUsageMax);
    local_state->SetInteger(kNTPCustomizeUsageStatus, usage);
  }
}

void RegisterP3AUtilsPrefs(PrefRegistrySimple* local_state) {
  local_state->RegisterIntegerPref(kShieldUsageStatus, -1);
  local_state->RegisterIntegerPref(kNTPCustomizeUsageStatus, -1);
}

}  // namespace brave
