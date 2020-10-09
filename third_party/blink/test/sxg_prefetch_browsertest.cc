/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/loader/prefetch_browsertest_base.h"
#include "content/browser/web_package/mock_signed_exchange_handler.h"
#include "content/browser/web_package/prefetched_signed_exchange_cache.h"
#include "content/browser/web_package/signed_exchange_error.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/base/hash_value.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace {

std::string GetHeaderIntegrityString(const net::SHA256HashValue& hash) {
  std::string header_integrity_string = net::HashValue(hash).ToString();
  // Change "sha256/" to "sha256-".
  header_integrity_string[6] = '-';
  return header_integrity_string;
}

content::PrefetchedSignedExchangeCache::EntryMap GetCachedExchanges(
    content::Shell* shell) {
  content::RenderViewHost* rvh = shell->web_contents()->GetRenderViewHost();
  content::RenderFrameHostImpl* rfh =
      static_cast<content::RenderFrameHostImpl*>(rvh->GetMainFrame());
  scoped_refptr<content::PrefetchedSignedExchangeCache> cache =
      rfh->EnsurePrefetchedSignedExchangeCache();
  content::PrefetchedSignedExchangeCache::EntryMap results;
  for (const auto& exchanges_it : cache->GetExchanges())
    results[exchanges_it.first] = exchanges_it.second->Clone();
  return results;
}

content::PrefetchBrowserTestBase::ResponseEntry
CreateSignedExchangeResponseEntry(
    const std::string& content,
    const std::vector<std::pair<std::string, std::string>>& additional_headers =
        {}) {
  std::vector<std::pair<std::string, std::string>> headers = {
      {"x-content-type-options", "nosniff"}};

  for (const auto& additional_header : additional_headers) {
    headers.emplace_back(
        std::make_pair(additional_header.first, additional_header.second));
  }

  // We mock the SignedExchangeHandler, so just return the content as
  // "application/signed-exchange;v=b3".
  return content::PrefetchBrowserTestBase::ResponseEntry(
      content, "application/signed-exchange;v=b3", headers);
}

std::string CreateAlternateLinkHeader(const GURL& sxg_url,
                                      const GURL& inner_url) {
  return base::StringPrintf(
      "<%s>;"
      "rel=\"alternate\";"
      "type=\"application/signed-exchange;v=b3\";"
      "anchor=\"%s\"",
      sxg_url.spec().c_str(), inner_url.spec().c_str());
}

std::string CreateAllowedAltSxgLinkHeader(
    const GURL& inner_url,
    const net::SHA256HashValue& header_integrity) {
  return base::StringPrintf(
      "<%s>;rel=\"allowed-alt-sxg\";header-integrity=\"%s\"",
      inner_url.spec().c_str(),
      GetHeaderIntegrityString(header_integrity).c_str());
}

std::string CreatePreloadLinkHeader(const GURL& url, const char* as) {
  return base::StringPrintf("<%s>;rel=\"preload\";as=\"%s\"",
                            url.spec().c_str(), as);
}

}  // namespace

class SXGBrowserTestBase : public ::testing::WithParamInterface<bool>,
                           public content::PrefetchBrowserTestBase {
 public:
  SXGBrowserTestBase() {}
  ~SXGBrowserTestBase() = default;

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    content::PrefetchBrowserTestBase::SetUpOnMainThread();
  }

  bool IsSignedExchangePrefetchFeatureEnabled() { return GetParam(); }

  void CheckHistogramsTotalCounts(const GURL& sxg_url,
                                  const std::string& title,
                                  RequestCounter* request_counter) {
    base::HistogramTester histograms;
    // Subsequent navigation to the target URL wouldn't hit the network for
    // the target URL. The target content should still be read correctly.
    // The content is loaded from HTTPCache.
    NavigateToURLAndWaitTitle(sxg_url, title);
    EXPECT_EQ(1, request_counter->GetRequestCount());

    static const std::array<std::string, 4> counts = {
        "PrefetchedSignedExchangeCache.Count",
        "PrefetchedSignedExchangeCache.BodySize",
        "PrefetchedSignedExchangeCache.BodySizeTotal",
        "PrefetchedSignedExchangeCache.HeadersSizeTotal"};
    for (const auto& count : counts)
      histograms.ExpectTotalCount(count, 0);
  }

  void CheckHistogramsBucketCounts(const GURL& sxg_url,
                                   const std::string& title,
                                   RequestCounter* request_counter,
                                   int64_t content_size,
                                   int64_t content_size_total,
                                   int64_t headers_size) {
    base::HistogramTester histograms;
    // Subsequent navigation to the target URL wouldn't hit the network for
    // the target URL. The target content should still be read correctly.
    // The content is loaded from PrefetchedSignedExchangeCache.
    NavigateToURLAndWaitTitle(sxg_url, title);
    EXPECT_EQ(1, request_counter->GetRequestCount());

    const std::map<std::string, int64_t> counts = {
        {"PrefetchedSignedExchangeCache.Count", 1},
        {"PrefetchedSignedExchangeCache.BodySize", content_size},
        {"PrefetchedSignedExchangeCache.BodySizeTotal", content_size_total},
        {"PrefetchedSignedExchangeCache.HeadersSizeTotal", headers_size}};
    for (auto& count : counts)
      histograms.ExpectBucketCount(count.first, count.second, 1);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

class SXGSubresourcePrefetchBrowserTest : public SXGBrowserTestBase {
 public:
  SXGSubresourcePrefetchBrowserTest() {}

  void SetUp() override {
    std::vector<base::Feature> enabled_features;
    if (IsSignedExchangePrefetchFeatureEnabled()) {
      enabled_features.push_back(features::kSignedExchangeSubresourcePrefetch);
    }
    scoped_feature_list_.InitWithFeatures(
        enabled_features,
        {features::kSignedExchangePrefetchCacheForNavigations});
    SXGBrowserTestBase::SetUp();
  }

 protected:
  void LoadPrefetchMainResourceSXGTestPage(
      const std::string& prefetch_page_path,
      const std::string& sxg_path,
      const std::string& inner_url_path,
      const net::SHA256HashValue& header_integrity,
      const std::string& content,
      RequestCounter* request_counter) {
    const GURL prefetch_page_url =
        embedded_test_server()->GetURL(prefetch_page_path);
    const GURL sxg_url = embedded_test_server()->GetURL(sxg_path);
    const GURL inner_url = embedded_test_server()->GetURL(inner_url_path);

    RegisterResponse(prefetch_page_path,
                     ResponseEntry(base::StringPrintf(
                         "<body><link rel='prefetch' href='%s'></body>",
                         sxg_url.spec().c_str())));
    RegisterResponse(sxg_path, CreateSignedExchangeResponseEntry(
                                   content, {} /* sxg_outer_headers */));

    content::MockSignedExchangeHandlerFactory factory(
        {content::MockSignedExchangeHandlerParams(
        sxg_url, content::SignedExchangeLoadResult::kSuccess, net::OK,
        inner_url, "text/html", {} /* sxg_inner_headers */, header_integrity,
        base::Time())});
    ScopedSignedExchangeHandlerFactory scoped_factory(&factory);

    EXPECT_EQ(0, request_counter->GetRequestCount());

    EXPECT_TRUE(content::NavigateToURL(shell(), prefetch_page_url));
    WaitUntilLoaded(sxg_url);
  }
};

class SXGPrefetchCacheForNavigationsBrowserTest : public SXGBrowserTestBase {
 public:
  SXGPrefetchCacheForNavigationsBrowserTest() {}

  void SetUp() override {
    std::vector<base::Feature> enabled_features;
    if (IsSignedExchangePrefetchFeatureEnabled()) {
      enabled_features.push_back(
          features::kSignedExchangePrefetchCacheForNavigations);
    }
    scoped_feature_list_.InitWithFeatures(
        enabled_features, {features::kSignedExchangeSubresourcePrefetch});
    SXGBrowserTestBase::SetUp();
  }
};

IN_PROC_BROWSER_TEST_P(SXGSubresourcePrefetchBrowserTest, SXGPrefetch) {
  const char* prefetch_page_path = "/prefetch.html";
  const char* sxg_path = "/target.sxg";
  const char* inner_url_path = "/target.html";
  const net::SHA256HashValue header_integrity = {{0x01}};
  const std::string content =
      "<head><title>Prefetch Target (SXG)</title></head>";

  auto sxg_request_counter =
      RequestCounter::CreateAndMonitor(embedded_test_server(), sxg_path);
  RegisterRequestHandler(embedded_test_server());
  ASSERT_TRUE(embedded_test_server()->Start());

  LoadPrefetchMainResourceSXGTestPage(prefetch_page_path, sxg_path,
                                      inner_url_path, header_integrity, content,
                                      sxg_request_counter.get());
  EXPECT_EQ(1, sxg_request_counter->GetRequestCount());

  const GURL sxg_url = embedded_test_server()->GetURL(sxg_path);
  const GURL inner_url = embedded_test_server()->GetURL(inner_url_path);

  if (!IsSignedExchangePrefetchFeatureEnabled()) {
    EXPECT_TRUE(GetCachedExchanges(shell()).empty());

    // Shutdown the server.
    EXPECT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());

    // Need to setup MockSignedExchangeHandlerFactory because the SXG is
    // loaded from HTTPCache.
    content::MockSignedExchangeHandlerFactory factory(
        {content::MockSignedExchangeHandlerParams(
            sxg_url, content::SignedExchangeLoadResult::kSuccess, net::OK,
            inner_url, "text/html", {}, header_integrity,
            base::Time() /* signature_expire_time */)});
    ScopedSignedExchangeHandlerFactory scoped_factory(&factory);
    CheckHistogramsTotalCounts(sxg_url, "Prefetch Target (SXG)",
                               sxg_request_counter.get());
    return;
  }

  const auto cached_exchanges = GetCachedExchanges(shell());
  EXPECT_EQ(1u, cached_exchanges.size());
  const auto it = cached_exchanges.find(sxg_url);
  ASSERT_TRUE(it != cached_exchanges.end());
  const std::unique_ptr<const content::PrefetchedSignedExchangeCacheEntry>&
      exchange = it->second;
  EXPECT_EQ(sxg_url, exchange->outer_url());
  EXPECT_EQ(inner_url, exchange->inner_url());
  EXPECT_EQ(header_integrity, *exchange->header_integrity());
  const int64_t headers_size_total =
      exchange->outer_response()->headers->raw_headers().size() +
      exchange->inner_response()->headers->raw_headers().size();

  // Shutdown the server.
  EXPECT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());

  CheckHistogramsBucketCounts(sxg_url, "Prefetch Target (SXG)",
                              sxg_request_counter.get(), content.size(),
                              content.size(), headers_size_total);
}

IN_PROC_BROWSER_TEST_P(SXGPrefetchCacheForNavigationsBrowserTest,
                       SXGPrefetch) {
  const char* prefetch_page_path = "/prefetch.html";
  const char* page_sxg_path = "/target.sxg";
  const char* page_inner_url_path = "/target.html";
  const char* script_sxg_path = "/script_js.sxg";
  const char* script_inner_url_path = "/script.js";

  auto page_sxg_request_counter =
      RequestCounter::CreateAndMonitor(embedded_test_server(), page_sxg_path);
  auto script_sxg_request_counter =
      RequestCounter::CreateAndMonitor(embedded_test_server(), script_sxg_path);
  auto script_request_counter = RequestCounter::CreateAndMonitor(
      embedded_test_server(), script_inner_url_path);
  RegisterRequestHandler(embedded_test_server());
  ASSERT_TRUE(embedded_test_server()->Start());

  const GURL prefetch_page_url =
      embedded_test_server()->GetURL(prefetch_page_path);
  const GURL sxg_page_url = embedded_test_server()->GetURL(page_sxg_path);
  const GURL inner_url_page_url =
      embedded_test_server()->GetURL(page_inner_url_path);
  const GURL sxg_script_url = embedded_test_server()->GetURL(script_sxg_path);
  const GURL inner_url_script_url =
      embedded_test_server()->GetURL(script_inner_url_path);

  const net::SHA256HashValue page_header_integrity = {{0x01}};
  const net::SHA256HashValue script_header_integrity = {{0x02}};

  const std::string outer_link_header =
      CreateAlternateLinkHeader(sxg_script_url, inner_url_script_url);
  const std::string inner_link_headers = base::JoinString(
      {CreateAllowedAltSxgLinkHeader(inner_url_script_url,
                                     script_header_integrity),
       CreatePreloadLinkHeader(inner_url_script_url, "script")},
      ",");
  const std::string page_sxg_content =
      "<head><title>Prefetch Target (SXG)</title>"
      "<script src=\"./script.js\"></script></head>";
  const std::string script_sxg_content = "document.title=\"done\";";
  RegisterResponse(prefetch_page_path,
                   ResponseEntry(base::StringPrintf(
                       "<body><link rel='prefetch' href='%s'></body>",
                       sxg_page_url.spec().c_str())));
  RegisterResponse(
      script_inner_url_path,
      ResponseEntry("document.title=\"from server\";", "text/javascript",
                    {{"cache-control", "public, max-age=600"}}));
  RegisterResponse(page_sxg_path,
                   CreateSignedExchangeResponseEntry(
                       page_sxg_content, {{"link", outer_link_header}}));
  RegisterResponse(script_sxg_path,
                   CreateSignedExchangeResponseEntry(script_sxg_content));
  content::MockSignedExchangeHandlerFactory factory(
      {content::MockSignedExchangeHandlerParams(
           sxg_page_url, content::SignedExchangeLoadResult::kSuccess, net::OK,
           inner_url_page_url, "text/html", {{"Link", inner_link_headers}},
           page_header_integrity),
       content::MockSignedExchangeHandlerParams(
           sxg_script_url, content::SignedExchangeLoadResult::kSuccess, net::OK,
           inner_url_script_url, "text/javascript", {},
           script_header_integrity)});
  ScopedSignedExchangeHandlerFactory scoped_factory(&factory);

  EXPECT_EQ(0, GetPrefetchURLLoaderCallCount());
  EXPECT_TRUE(content::NavigateToURL(shell(), prefetch_page_url));

  WaitUntilLoaded(sxg_page_url);
  WaitUntilLoaded(inner_url_script_url);

  EXPECT_EQ(1, page_sxg_request_counter->GetRequestCount());
  EXPECT_EQ(0, script_sxg_request_counter->GetRequestCount());
  EXPECT_EQ(1, script_request_counter->GetRequestCount());
  EXPECT_EQ(1, GetPrefetchURLLoaderCallCount());

  const auto cached_exchanges = GetCachedExchanges(shell());
  int64_t expected_body_size_total = 0u;
  int64_t expected_headers_size_total = 0u;

  if (IsSignedExchangePrefetchFeatureEnabled()) {
    EXPECT_EQ(1u, cached_exchanges.size());
    const auto page_it = cached_exchanges.find(sxg_page_url);
    ASSERT_TRUE(page_it != cached_exchanges.end());
    const std::unique_ptr<const content::PrefetchedSignedExchangeCacheEntry>&
        page_exchange = page_it->second;
    EXPECT_EQ(sxg_page_url, page_exchange->outer_url());
    EXPECT_EQ(inner_url_page_url, page_exchange->inner_url());
    EXPECT_EQ(page_header_integrity, *page_exchange->header_integrity());
    expected_body_size_total += page_sxg_content.size();
    expected_headers_size_total +=
        page_exchange->outer_response()->headers->raw_headers().size() +
        page_exchange->inner_response()->headers->raw_headers().size();
    CheckHistogramsBucketCounts(
        sxg_page_url, "from server", page_sxg_request_counter.get(),
        page_sxg_content.size(), expected_body_size_total,
        expected_headers_size_total);
    EXPECT_EQ(0, script_sxg_request_counter->GetRequestCount());
    EXPECT_EQ(1, script_request_counter->GetRequestCount());
  } else {
    EXPECT_EQ(0u, cached_exchanges.size());
    CheckHistogramsTotalCounts(sxg_page_url, "from server",
                               page_sxg_request_counter.get());
  }
}

INSTANTIATE_TEST_SUITE_P(SXGPrefetchBrowserTest,
                         SXGSubresourcePrefetchBrowserTest,
                         ::testing::Bool());

INSTANTIATE_TEST_SUITE_P(SXGPrefetchBrowserTest,
                         SXGPrefetchCacheForNavigationsBrowserTest,
                         ::testing::Bool());
