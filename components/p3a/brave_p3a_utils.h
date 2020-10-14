/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_UTILS_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_UTILS_H_

class PrefRegistrySimple;
class PrefService;

namespace brave_shields {

// Note: these are APPEND-ONLY enumerations! Never remove any existing values,
// as these enums are used to bucket a UMA histogram, and removing values
// breaks that.
enum ShieldsIconUsage {
  kNeverClicked,
  kClicked,
  kShutOffShields,
  kChangedPerSiteShields,
  kSize,
};

// With the "Maybe" methods, we save latest value to local state and compare
// new values with it. The idea is to write to a histogram only the highest
// value (e.g. we are not interested in |kClicked| event if the user already
// turned off shields. Since P3A sends only latest written values, these are
// enough for our current goals.
void MaybeRecordShieldsUsageP3A(ShieldsIconUsage usage,
                                PrefService* local_state);
}  // namespace brave_shields

namespace brave {

enum NTPCustomizeUsage {
  kNeverOpened,
  kOpened,
  kOpenedAndEdited,
  kCustomizeUsageMax
};

void MaybeRecordNTPCustomizeUsageP3A(NTPCustomizeUsage usage,
                                     PrefService* local_state);

void RegisterP3AUtilsPrefs(PrefRegistrySimple* local_state);

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_UTILS_H_