/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <random>

#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"

#define BRAVE_WORKERNAVIGATOR_USERAGENT                                \
  if (blink::WebContentSettingsClient* settings =                      \
          brave::GetContentSettingsClientFor(GetExecutionContext())) { \
    if (!settings->AllowFingerprinting(true)) {                        \
      return brave::BraveSessionCache::From(*(GetExecutionContext()))  \
          .FarbledUserAgent(user_agent_);                              \
    }                                                                  \
  }

#include "../../../../../../../third_party/blink/renderer/core/workers/worker_navigator.cc"

#undef BRAVE_WORKERNAVIGATOR_USERAGENT
