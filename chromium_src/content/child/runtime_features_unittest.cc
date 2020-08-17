// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "content/public/common/content_features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

class RuntimeEnabledFeaturesTest : public testing::Test {
 public:
  RuntimeEnabledFeaturesTest() : testing::Test() {
    // Enable features on start via Chromium features.
    EnableChromiumFeatures();
  }

  void EnableChromiumFeatures() {
    feature_list_.InitWithFeatures(
        { // features::kLangClientHintHeader,
         features::kSubresourceWebBundles,
         features::kSignedExchangePrefetchCacheForNavigations,
         features::kSignedExchangeSubresourcePrefetch},
        {});
  }

  void CheckChromiumFeaturesEnabled() {
    // Confirm that features are enabled.
    //EXPECT_TRUE(base::FeatureList::IsEnabled(features::kLangClientHintHeader));
    EXPECT_TRUE(base::FeatureList::IsEnabled(features::kSubresourceWebBundles));
    EXPECT_TRUE(base::FeatureList::IsEnabled(
        features::kSignedExchangeSubresourcePrefetch));
    EXPECT_TRUE(base::FeatureList::IsEnabled(
        features::kSignedExchangePrefetchCacheForNavigations));
  }

  void CheckAllDisabled() {
    using blink::RuntimeEnabledFeatures;
    // LangClientHintHeader
    EXPECT_FALSE(RuntimeEnabledFeatures::LangClientHintHeaderEnabled(nullptr));
    // SubresourceWebBundles
    EXPECT_FALSE(RuntimeEnabledFeatures::SubresourceWebBundlesEnabled(nullptr));
    // SignedExchangePrefetchCacheForNavigations
    EXPECT_FALSE(RuntimeEnabledFeatures::
                     SignedExchangePrefetchCacheForNavigationsEnabled(nullptr));
    // SignedExchangeSubresourcePrefetch
    EXPECT_FALSE(
        RuntimeEnabledFeatures::SignedExchangeSubresourcePrefetchEnabled(
            nullptr));
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(RuntimeEnabledFeaturesTest, TestBlinkRuntimeFeaturesDisabled) {
  CheckChromiumFeaturesEnabled();
  CheckAllDisabled();
}
