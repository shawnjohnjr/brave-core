/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/browser/native_file_system/file_system_chooser_test_helpers.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/blink/public/common/features.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "ui/shell_dialogs/select_file_dialog_factory.h"
#include "url/gurl.h"

class NativeFileSystemAPIBrowserTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<bool> {
 public:
  NativeFileSystemAPIBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_.ServeFilesFromDirectory(test_data_dir);
  }

  ~NativeFileSystemAPIBrowserTest() override = default;

  bool IsNativeFileSystemAPIEnabled() { return GetParam(); }

  void SetUp() override {
    if (IsNativeFileSystemAPIEnabled()) {
      scoped_feature_list_.InitAndEnableFeature(
          blink::features::kNativeFileSystemAPI);
    }
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    EXPECT_TRUE(https_server_.Start());
    // Map all hosts to localhost.
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* main_frame() {
    return web_contents()->GetMainFrame();
  }

 protected:
  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(NativeFileSystemAPIBrowserTest, FilePicker) {
  EXPECT_EQ(
      IsNativeFileSystemAPIEnabled(),
      base::FeatureList::IsEnabled(blink::features::kNativeFileSystemAPI));

  ui::SelectFileDialog::SetFactory(
      new content::CancellingSelectFileDialogFactory);
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  auto result = content::EvalJs(main_frame(), "self.showOpenFilePicker()");
  if (IsNativeFileSystemAPIEnabled()) {
    EXPECT_TRUE(result.error.find("aborted") != std::string::npos)
        << result.error;
  } else {
    EXPECT_TRUE(
        result.error.find("self.showOpenFilePicker is not a function") !=
        std::string::npos)
        << result.error;
  }
}

INSTANTIATE_TEST_SUITE_P(NativeFileSystemAPIBrowserTest,
                         NativeFileSystemAPIBrowserTest,
                         ::testing::Bool());
