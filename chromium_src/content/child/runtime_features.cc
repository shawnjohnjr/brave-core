/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define SetRuntimeFeaturesDefaultsAndUpdateFromArgs SetRuntimeFeaturesDefaultsAndUpdateFromArgs_ChromiumImpl
#include "../../../../content/child/runtime_features.cc"
#undef SetRuntimeFeaturesDefaultsAndUpdateFromArgs

namespace content {

void SetRuntimeFeaturesDefaultsAndUpdateFromArgs(
    const base::CommandLine& command_line) {
  SetRuntimeFeaturesDefaultsAndUpdateFromArgs_ChromiumImpl(command_line);

  // Disable runtime features that we don't want to be affected by Chromium
  // features enablement, experiments, etc.
  blink::WebRuntimeFeatures::EnableSubresourceWebBundles(false);
  blink::WebRuntimeFeatures::EnableSignedExchangeSubresourcePrefetch(false);
  blink::WebRuntimeFeatures::EnableSignedExchangePrefetchCacheForNavigations(
      false);

  // This feature doesn't have a dedicated WebRuntimeFeatures wrapper.
  blink::WebRuntimeFeatures::EnableFeatureFromString("LangClientHintHeader",
                                                     false);
}

} // namespace content
