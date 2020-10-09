/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "base/path_service.h"
#include "base/test/bind_test_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/web_package/test_support/web_bundle_builder.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "third_party/blink/public/common/features.h"
#include "url/gurl.h"

namespace {
const char kPageHtml[] = R"(
        <title>Loaded</title>
        <body>
        <script>
        (() => {
          const wbn_url =
              new URL('./web_bundle.wbn', location.href).toString();
          const pass_js_url = new URL('./pass.js', location.href).toString();
          const link = document.createElement('link');
          link.rel = 'webbundle';
          link.href = wbn_url;
          link.resources = pass_js_url;
          document.body.appendChild(link);
        })();
        </script>
        </body>
      )";
}  // namespace

class SubresourceWebBundlesBrowserTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<bool> {
 public:
  SubresourceWebBundlesBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_.ServeFilesFromDirectory(test_data_dir);
  }

  ~SubresourceWebBundlesBrowserTest() override = default;

  bool IsSubresourceWebBundlesEnabled() { return GetParam(); }

  void SetUp() override {
    if (IsSubresourceWebBundlesEnabled()) {
      scoped_feature_list_.InitAndEnableFeature(
          features::kSubresourceWebBundles);
    }
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    RegisterWebBundleRequestHandler("/web_bundle.wbn", &web_bundle_);
    RegisterRequestHandler("/test.html", "text/html", kPageHtml);

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

 protected:
  bool TryLoadScript(const char* script_src) {
    bool success = false;
    std::string script = base::StringPrintf(R"(
      (() => {
        const script = document.createElement('script');
        script.addEventListener('load', () => {
          window.domAutomationController.send(true);
        });
        script.addEventListener('error', () => {
          window.domAutomationController.send(false);
        });
        script.src = '%s';
        document.body.appendChild(script);
      })();
                                          )",
                                            script_src);
    EXPECT_TRUE(ExecuteScriptAndExtractBool(main_frame(), script, &success));
    return success;
  }

  // Registers a request handler for static content.
  void RegisterRequestHandler(const std::string& relative_url,
                              const std::string& content_type,
                              const std::string& content) {
    https_server_.RegisterRequestHandler(base::BindLambdaForTesting(
        [relative_url, content_type,
         content](const net::test_server::HttpRequest& request)
            -> std::unique_ptr<net::test_server::HttpResponse> {
          if (request.relative_url == relative_url) {
            auto response =
                std::make_unique<net::test_server::BasicHttpResponse>();
            response->set_code(net::HTTP_OK);
            response->set_content_type(content_type);
            response->set_content(content);
            return std::move(response);
          }
          return nullptr;
        }));
  }

  // Registers a request handler for web bundle. This method takes a pointer of
  // the content of the web bundle, because we can't create the content of the
  // web bundle before starting the server since we need to know the port number
  // of the test server due to the same-origin restriction (the origin of
  // subresource which is written in the web bundle must be same as the origin
  // of the web bundle), and we can't call
  // EmbeddedTestServer::RegisterRequestHandler() after starting the server.
  void RegisterWebBundleRequestHandler(const std::string& relative_url,
                                       const std::string* web_bundle) {
    https_server_.RegisterRequestHandler(base::BindLambdaForTesting(
        [relative_url, web_bundle](const net::test_server::HttpRequest& request)
            -> std::unique_ptr<net::test_server::HttpResponse> {
          if (request.relative_url == relative_url) {
            auto response =
                std::make_unique<net::test_server::BasicHttpResponse>();
            response->set_code(net::HTTP_OK);
            response->set_content_type("application/webbundle");
            response->set_content(*web_bundle);
            return std::move(response);
          }
          return nullptr;
        }));
  }

  void CreateWebBundle() {
    std::string pass_js_url_str = https_server_.GetURL("/pass.js").spec();
    // Currently the web bundle format requires a valid GURL for the fallback
    // URL of a web bundle.
    std::string flbk_js_url_str = https_server_.GetURL("/fallback.js").spec();
    web_package::test::WebBundleBuilder builder(flbk_js_url_str, "");
    auto pass_js_location = builder.AddResponse(
        {{":status", "200"}, {"content-type", "application/javascript"}},
        "document.title = 'script loaded';");
    builder.AddIndexEntry(pass_js_url_str, "", {pass_js_location});
    std::vector<uint8_t> bundle = builder.CreateBundle();
    web_bundle_ = std::string(bundle.begin(), bundle.end());
  }

  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList scoped_feature_list_;

 private:
  std::string web_bundle_;
};

IN_PROC_BROWSER_TEST_P(SubresourceWebBundlesBrowserTest,
                       SubresourceWebBundles) {
  EXPECT_EQ(IsSubresourceWebBundlesEnabled(),
            base::FeatureList::IsEnabled(features::kSubresourceWebBundles));
  CreateWebBundle();
  GURL page_url = https_server_.GetURL("/test.html");
  ui_test_utils::NavigateToURL(browser(), page_url);
  EXPECT_EQ(page_url, web_contents()->GetLastCommittedURL());

  if (IsSubresourceWebBundlesEnabled()) {
    base::string16 expected_title = base::ASCIIToUTF16("script loaded");
    content::TitleWatcher title_watcher(web_contents(), expected_title);
    EXPECT_TRUE(TryLoadScript("pass.js"));
    EXPECT_EQ(expected_title, title_watcher.WaitAndGetTitle());
  } else {
    EXPECT_FALSE(TryLoadScript("pass.js"));
  }
}

INSTANTIATE_TEST_SUITE_P(SubresourceWebBundlesBrowserTest,
                         SubresourceWebBundlesBrowserTest,
                         ::testing::Bool());
