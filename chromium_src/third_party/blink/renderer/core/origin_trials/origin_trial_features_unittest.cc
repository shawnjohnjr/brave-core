/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

#include <array>
#include <string>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_mock_time_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trial_context.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trials.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {
namespace {

struct BraveDisabledTrial {
  const char* trial_name;
  OriginTrialFeature trial_feature;
};

// This list should be in sync with the ones in origin_trials.cc and
// origin_trial_context.cc overrides.
const std::array<BraveDisabledTrial, 3> kBraveDisabledTrials = {{
    // [Not released yet]
    //    {"DigitalGoods", OriginTrialFeature::kDigitalGoods},
    {"NativeFileSystem2", OriginTrialFeature::kNativeFileSystem},
    {"SignedExchangeSubresourcePrefetch",
     OriginTrialFeature::kSignedExchangeSubresourcePrefetch},
    {"SubresourceWebBundles", OriginTrialFeature::kSubresourceWebBundles},
}};

class OriginTrialFeaturesTest : public testing::Test {
 public:
  OriginTrialFeaturesTest() : testing::Test() {}

  void SetUp() override {
    test_task_runner_ = base::MakeRefCounted<base::TestMockTimeTaskRunner>();
    page_holder_ = std::make_unique<DummyPageHolder>(IntSize(800, 600));
    page_holder_->GetDocument().SetURL(KURL("https://example.com"));
  }

  LocalFrame* GetFrame() const { return &(page_holder_->GetFrame()); }
  LocalDOMWindow* GetWindow() const { return GetFrame()->DomWindow(); }

 private:
  base::test::SingleThreadTaskEnvironment task_environment_;
  std::unique_ptr<DummyPageHolder> page_holder_ = nullptr;
  scoped_refptr<base::TestMockTimeTaskRunner> test_task_runner_;
};

TEST_F(OriginTrialFeaturesTest, TestOriginTrialsNames) {
  // Check if our disabled trials are still valid trials in Chromium.
  // If any name fails, check if the trial was removed and then it can be
  // removed from origin_trials.cc and origin_trial_context.cc overrides.
  for (const auto& trial : kBraveDisabledTrials) {
    EXPECT_TRUE(origin_trials::IsTrialValid_ForTests(trial.trial_name))
        << std::string("Failing trial: ") + trial.trial_name;
  }
}

TEST_F(OriginTrialFeaturesTest, TestBlinkRuntimeFeaturesViaOriginTrials) {
  // Test origin trials overrides.
  for (const auto& trial : kBraveDisabledTrials) {
    // IsTrialValid override.
    EXPECT_FALSE(origin_trials::IsTrialValid(trial.trial_name))
        << std::string("Failing trial: ") + trial.trial_name;

    // Trials framework AddFeature override.
    GetWindow()->GetOriginTrialContext()->AddFeature(trial.trial_feature);
    EXPECT_FALSE(GetWindow()->GetOriginTrialContext()->IsFeatureEnabled(
        trial.trial_feature))
        << std::string("Failing trial: ") + trial.trial_name;

    // Trials framework force via origin trial names.
    WTF::Vector<WTF::String> forced_trials = {trial.trial_name};
    GetWindow()->GetOriginTrialContext()->AddForceEnabledTrials(forced_trials);
    EXPECT_FALSE(GetWindow()->GetOriginTrialContext()->IsFeatureEnabled(
        trial.trial_feature))
        << std::string("Failing trial: ") + trial.trial_name;
  }
}

TEST_F(OriginTrialFeaturesTest, TestBlinkRuntimeFeaturesWithoutOriginTrials) {
  // The following don't currently have origin trials associated with them,
  // but if they end up having them we should be able to catch here.
  RuntimeEnabledFeatures::SetDigitalGoodsEnabled(false);
  // [Available in Cr87] RuntimeEnabledFeatures::SetDirectSocketsEnabled(false);
  RuntimeEnabledFeatures::SetLangClientHintHeaderEnabled(false);
  RuntimeEnabledFeatures::SetSignedExchangePrefetchCacheForNavigationsEnabled(
      false);

  // Enable all origin trial controlled features.
  RuntimeEnabledFeatures::SetOriginTrialControlledFeaturesEnabled(true);

  // Check if features in question became enabled.
  EXPECT_FALSE(RuntimeEnabledFeatures::DigitalGoodsEnabled());
  // [Available in Cr87]
  // EXPECT_FALSE(RuntimeEnabledFeatures::DirectSocketsEnabled());
  EXPECT_FALSE(RuntimeEnabledFeatures::LangClientHintHeaderEnabled());
  EXPECT_FALSE(RuntimeEnabledFeatures::
                   SignedExchangePrefetchCacheForNavigationsEnabled());
}

}  // namespace
}  // namespace blink
