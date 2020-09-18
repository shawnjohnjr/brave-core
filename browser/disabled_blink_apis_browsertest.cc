/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/browser/native_file_system/file_system_chooser_test_helpers.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "ui/shell_dialogs/select_file_dialog_factory.h"
#include "url/gurl.h"

class DisabledBlinkAPIsBrowserTest : public InProcessBrowserTest {
 public:
  DisabledBlinkAPIsBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    EXPECT_TRUE(https_server_.Start());
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

 protected:
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(DisabledBlinkAPIsBrowserTest, NativeFileSystem) {
  ui::SelectFileDialog::SetFactory(
      new content::CancellingSelectFileDialogFactory);
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::RenderFrameHost* rfh = contents->GetMainFrame();

  auto result = content::EvalJs(rfh, "self.showOpenFilePicker()");
  EXPECT_TRUE(result.error.find("self.showOpenFilePicker is not a function") !=
              std::string::npos)
      << result.error;

  auto result1 = content::EvalJs(rfh, "navigator.storage.getDirectory()");
  EXPECT_TRUE(
      result1.error.find("navigator.storage.getDirectory is not a function") !=
      std::string::npos)
      << result1.error;
}

IN_PROC_BROWSER_TEST_F(DisabledBlinkAPIsBrowserTest, DirectSockets) {
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::RenderFrameHost* rfh = contents->GetMainFrame();

  auto result = content::EvalJs(
      rfh,
      "navigator.openTCPSocket({remoteAddress: '127.0.0.1', remotePort: 0})");
  EXPECT_TRUE(result.error.find("navigator.openTCPSocket is not a function") !=
              std::string::npos)
      << result.error;
}

IN_PROC_BROWSER_TEST_F(DisabledBlinkAPIsBrowserTest, DigitalGoods) {
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::RenderFrameHost* rfh = contents->GetMainFrame();

  auto result = content::EvalJs(rfh, "Window.getDigitalGoodsService()");
  EXPECT_TRUE(
      result.error.find("Window.getDigitalGoodsService is not a function") !=
      std::string::npos)
      << result.error;
}
