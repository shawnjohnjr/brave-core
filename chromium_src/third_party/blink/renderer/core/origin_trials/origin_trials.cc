/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/origin_trials/origin_trials.h"

namespace blink {
namespace origin_trials {
bool IsTrialValid_ChromiumImpl(const StringView& trial_name);
}  // namespace origin_trials
}  // namespace blink

#define IsTrialValid IsTrialValid_ChromiumImpl
#include "../gen/third_party/blink/renderer/core/origin_trials/origin_trials.cc"
#undef IsTrialValid

namespace blink {

bool origin_trials::IsTrialValid(const StringView& trial_name) {
  static const char* const kBraveDisabledTrialNames[] = {
      "SubresourceWebBundles",
      "SignedExchangeSubresourcePrefetch"
  };
  if (base::Contains(kBraveDisabledTrialNames, trial_name))
    return false;
  return origin_trials::IsTrialValid_ChromiumImpl(trial_name);
}

}  // namespace blink
