// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_mock_time_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trial_context.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trials.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

namespace blink {
namespace {

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
  ExecutionContext* GetExecutionContext() const {
    return page_holder_->GetDocument().GetExecutionContext();
  }

  void CheckAllDisabled() {
    // SubresourceWebBundles
    EXPECT_FALSE(RuntimeEnabledFeatures::SubresourceWebBundlesEnabled(
        GetExecutionContext()));
    // SignedExchangeSubresourcePrefetch
    EXPECT_FALSE(
        RuntimeEnabledFeatures::SignedExchangeSubresourcePrefetchEnabled(
            GetExecutionContext()));
  }

 private:
  base::test::SingleThreadTaskEnvironment task_environment_;
  std::unique_ptr<DummyPageHolder> page_holder_ = nullptr;
  scoped_refptr<base::TestMockTimeTaskRunner> test_task_runner_;
};

TEST_F(OriginTrialFeaturesTest, TestBlinkRuntimeFeaturesViaOriginTrials) {
  // Test origin trials overrides.
  EXPECT_FALSE(origin_trials::IsTrialValid("SubresourceWebBundles"));
  EXPECT_FALSE(
      origin_trials::IsTrialValid("SignedExchangeSubresourcePrefetch"));

  // Test trials framework.
  GetWindow()->GetOriginTrialContext()->AddFeature(
      OriginTrialFeature::kSubresourceWebBundles);
  EXPECT_FALSE(GetWindow()->GetOriginTrialContext()->IsFeatureEnabled(
      OriginTrialFeature::kSubresourceWebBundles));

  GetWindow()->GetOriginTrialContext()->AddFeature(
      OriginTrialFeature::kSignedExchangeSubresourcePrefetch);
  EXPECT_FALSE(GetWindow()->GetOriginTrialContext()->IsFeatureEnabled(
      OriginTrialFeature::kSignedExchangeSubresourcePrefetch));

  // Force via origin trial names.
  //WTF::Vector<WTF::String> forced_trials = {
  //    "SubresourceWebBundles", "SignedExchangeSubresourcePrefetch"};
  GetWindow()->GetOriginTrialContext()->AddForceEnabledTrials(
      {"SubresourceWebBundles"});
  EXPECT_FALSE(GetWindow()->GetOriginTrialContext()->IsFeatureEnabled(
      OriginTrialFeature::kSubresourceWebBundles));
  GetWindow()->GetOriginTrialContext()->AddForceEnabledTrials(
      {"SignedExchangeSubresourcePrefetch"});
  EXPECT_FALSE(GetWindow()->GetOriginTrialContext()->IsFeatureEnabled(
      OriginTrialFeature::kSignedExchangeSubresourcePrefetch));

  // Check runtime features return as disabled.
  CheckAllDisabled();
}

TEST_F(OriginTrialFeaturesTest, TestBlinkRuntimeFeaturesWithoutOriginTrials) {
  // The following don't currently have origin trials associated with them,
  // but if they end up having them we should be able to catch them.
  RuntimeEnabledFeatures::SetLangClientHintHeaderEnabled(false);
  RuntimeEnabledFeatures::SetSignedExchangePrefetchCacheForNavigationsEnabled(
      false);

  // Enable all origin trial controlled features.
  RuntimeEnabledFeatures::SetOriginTrialControlledFeaturesEnabled(true);

  // LangClientHintHeader
  EXPECT_FALSE(RuntimeEnabledFeatures::LangClientHintHeaderEnabled(
      GetExecutionContext()));
  // SignedExchangePrefetchCacheForNavigations
  EXPECT_FALSE(
      RuntimeEnabledFeatures::SignedExchangePrefetchCacheForNavigationsEnabled(
          GetExecutionContext()));
}

}  // namespace
}  // namespace blink
