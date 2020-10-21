/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/dismissed_frequency_cap.h"

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/platform/platform_helper_mock.h"
#include "bat/ads/internal/time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;
using ::testing::Return;

namespace ads {

namespace {

const char kCreativeInstanceId[] = "9aea9a47-c6a0-4718-a0fa-706338bb2156";

const std::vector<std::string> kCampaignIds = {
  "60267cee-d5bb-4a0d-baaf-91cd7f18e07e",
  "90762cee-d5bb-4a0d-baaf-61cd7f18e07e"
};

}  // namespace

class BatAdsDismissedFrequencyCapTest : public ::testing::Test {
 protected:
  BatAdsDismissedFrequencyCapTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        locale_helper_mock_(std::make_unique<
            NiceMock<brave_l10n::LocaleHelperMock>>()),
        platform_helper_mock_(std::make_unique<
            NiceMock<PlatformHelperMock>>()),
        frequency_cap_(std::make_unique<DismissedFrequencyCap>(ads_.get())) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());

    PlatformHelper::GetInstance()->set_for_testing(platform_helper_mock_.get());
  }

  ~BatAdsDismissedFrequencyCapTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    const base::FilePath path = temp_dir_.GetPath();

    SetBuildChannel(false, "test");

    ON_CALL(*locale_helper_mock_, GetLocale())
        .WillByDefault(Return("en-US"));

    MockPlatformHelper(platform_helper_mock_, PlatformType::kMacOS);

    ads_->OnWalletUpdated("c387c2d8-a26d-4451-83e4-5c0c6fd942be",
        "5BEKM1Y7xcRSg/1q8in/+Lki2weFZQB+UMYZlRw8ql8=");

    MockLoad(ads_client_mock_);
    MockLoadUserModelForId(ads_client_mock_);
    MockLoadResourceForId(ads_client_mock_);
    MockSave(ads_client_mock_);

    MockPrefs(ads_client_mock_);

    database_ = std::make_unique<Database>(path.AppendASCII("database.sqlite"));
    MockRunDBTransaction(ads_client_mock_, database_);

    Initialize(ads_);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  Client* get_client() {
    return ads_->get_client();
  }

  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
  std::unique_ptr<PlatformHelperMock> platform_helper_mock_;
  std::unique_ptr<DismissedFrequencyCap> frequency_cap_;
  std::unique_ptr<Database> database_;
};

TEST_F(BatAdsDismissedFrequencyCapTest,
    AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;
  ad.campaign_id = kCampaignIds.at(0);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedFrequencyCapTest,
    AdAllowedForAdWithSameCampaignIdWithin48HoursIfDismissed) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;
  ad.campaign_id = kCampaignIds.at(0);

  const std::vector<ConfirmationType> confirmation_types = {
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed,
  };

  for (const auto& confirmation_type : confirmation_types) {
    AdHistory ad_history = GenerateAdHistory(
        AdType::kAdNotification, ad, confirmation_type);
    get_client()->AppendAdHistoryToAdsHistory(ad_history);

    task_environment_.FastForwardBy(base::TimeDelta::FromMinutes(5));
  }

  task_environment_.FastForwardBy(base::TimeDelta::FromHours(47));

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedFrequencyCapTest,
    AdAllowedForAdWithSameCampaignIdWithin48HoursIfDismissedThenClicked) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;
  ad.campaign_id = kCampaignIds.at(0);

  const std::vector<ConfirmationType> confirmation_types = {
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed,
    ConfirmationType::kViewed,
    ConfirmationType::kClicked
  };

  for (const auto& confirmation_type : confirmation_types) {
    AdHistory ad_history = GenerateAdHistory(
        AdType::kAdNotification, ad, confirmation_type);
    get_client()->AppendAdHistoryToAdsHistory(ad_history);

    task_environment_.FastForwardBy(base::TimeDelta::FromMinutes(5));
  }

  task_environment_.FastForwardBy(base::TimeDelta::FromHours(47));

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedFrequencyCapTest,
    AdAllowedForAdWithSameCampaignIdAfter48HoursIfDismissedThenClicked) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;
  ad.campaign_id = kCampaignIds.at(0);

  const std::vector<ConfirmationType> confirmation_types = {
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed,
    ConfirmationType::kViewed,
    ConfirmationType::kClicked
  };

  for (const auto& confirmation_type : confirmation_types) {
    AdHistory ad_history = GenerateAdHistory(
        AdType::kAdNotification, ad, confirmation_type);
    get_client()->AppendAdHistoryToAdsHistory(ad_history);

    task_environment_.FastForwardBy(base::TimeDelta::FromMinutes(5));
  }

  task_environment_.FastForwardBy(base::TimeDelta::FromHours(48));

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedFrequencyCapTest,
    AdAllowedForAdWithSameCampaignIdWithin48HoursIfClickedThenDismissed) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;
  ad.campaign_id = kCampaignIds.at(0);

  const std::vector<ConfirmationType> confirmation_types = {
    ConfirmationType::kViewed,
    ConfirmationType::kClicked,
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed
  };

  for (const auto& confirmation_type : confirmation_types) {
    AdHistory ad_history = GenerateAdHistory(
        AdType::kAdNotification, ad, confirmation_type);
    get_client()->AppendAdHistoryToAdsHistory(ad_history);

    task_environment_.FastForwardBy(base::TimeDelta::FromMinutes(5));
  }

  task_environment_.FastForwardBy(base::TimeDelta::FromHours(47));

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedFrequencyCapTest,
    AdAllowedForAdWithSameCampaignIdAfter48HoursIfClickedThenDismissed) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;
  ad.campaign_id = kCampaignIds.at(0);

  const std::vector<ConfirmationType> confirmation_types = {
    ConfirmationType::kViewed,
    ConfirmationType::kClicked,
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed
  };

  for (const auto& confirmation_type : confirmation_types) {
    AdHistory ad_history = GenerateAdHistory(
        AdType::kAdNotification, ad, confirmation_type);
    get_client()->AppendAdHistoryToAdsHistory(ad_history);

    task_environment_.FastForwardBy(base::TimeDelta::FromMinutes(5));
  }

  task_environment_.FastForwardBy(base::TimeDelta::FromHours(48));

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedFrequencyCapTest,
    AdAllowedForAdWithSameCampaignIdAfter48HoursIfClickedThenDismissedTwice) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;
  ad.campaign_id = kCampaignIds.at(0);

  const std::vector<ConfirmationType> confirmation_types = {
    ConfirmationType::kViewed,
    ConfirmationType::kClicked,
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed,
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed
  };

  for (const auto& confirmation_type : confirmation_types) {
    AdHistory ad_history = GenerateAdHistory(
        AdType::kAdNotification, ad, confirmation_type);
    get_client()->AppendAdHistoryToAdsHistory(ad_history);

    task_environment_.FastForwardBy(base::TimeDelta::FromMinutes(5));
  }

  task_environment_.FastForwardBy(base::TimeDelta::FromHours(48));

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedFrequencyCapTest,
    AdNotAllowedForAdWithSameCampaignIdWithin48HoursIfClickedThenDismissedTwice) {  // NOLINT
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;
  ad.campaign_id = kCampaignIds.at(0);

  const std::vector<ConfirmationType> confirmation_types = {
    ConfirmationType::kViewed,
    ConfirmationType::kClicked,
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed,
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed
  };

  for (const auto& confirmation_type : confirmation_types) {
    AdHistory ad_history = GenerateAdHistory(
        AdType::kAdNotification, ad, confirmation_type);
    get_client()->AppendAdHistoryToAdsHistory(ad_history);

    task_environment_.FastForwardBy(base::TimeDelta::FromMinutes(5));
  }

  task_environment_.FastForwardBy(base::TimeDelta::FromHours(47));

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsDismissedFrequencyCapTest,
    AdAllowedForAdWithDifferentCampaignIdWithin48Hours) {
  // Arrange
  CreativeAdInfo ad_1;
  ad_1.creative_instance_id = kCreativeInstanceId;
  ad_1.campaign_id = kCampaignIds.at(0);

  CreativeAdInfo ad_2;
  ad_2.creative_instance_id = kCreativeInstanceId;
  ad_2.campaign_id = kCampaignIds.at(1);

  const std::vector<ConfirmationType> confirmation_types = {
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed,
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed
  };

  for (const auto& confirmation_type : confirmation_types) {
    AdHistory ad_history = GenerateAdHistory(
        AdType::kAdNotification, ad_2, confirmation_type);
    get_client()->AppendAdHistoryToAdsHistory(ad_history);

    task_environment_.FastForwardBy(base::TimeDelta::FromMinutes(5));
  }

  task_environment_.FastForwardBy(base::TimeDelta::FromHours(47));

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedFrequencyCapTest,
    AdAllowedForAdWithDifferentCampaignIdAfter48Hours) {
  // Arrange
  CreativeAdInfo ad_1;
  ad_1.creative_instance_id = kCreativeInstanceId;
  ad_1.campaign_id = kCampaignIds.at(0);

  CreativeAdInfo ad_2;
  ad_2.creative_instance_id = kCreativeInstanceId;
  ad_2.campaign_id = kCampaignIds.at(1);

  const std::vector<ConfirmationType> confirmation_types = {
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed,
    ConfirmationType::kViewed,
    ConfirmationType::kDismissed
  };

  for (const auto& confirmation_type : confirmation_types) {
    AdHistory ad_history = GenerateAdHistory(
        AdType::kAdNotification, ad_2, confirmation_type);
    get_client()->AppendAdHistoryToAdsHistory(ad_history);

    task_environment_.FastForwardBy(base::TimeDelta::FromMinutes(5));
  }

  task_environment_.FastForwardBy(base::TimeDelta::FromHours(48));

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

}  // namespace ads
