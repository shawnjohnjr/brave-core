/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/brave_cosmetic_resources_tab_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/renderer/render_frame.h"


namespace {
const char k_observing_script[] =
    "(function() {"
      "const queriedIds = new Set();"
      "const queriedClasses = new Set();"
      "var notYetQueriedClasses;"
      "var notYetQueriedIds;"
      "var cosmeticObserver;"
      "const fetchNewClassIdRules = function () {"
  "if ((!notYetQueriedClasses || notYetQueriedClasses.length === 0) &&"
      "(!notYetQueriedIds || notYetQueriedIds.length === 0)) {"
    "return;"
  "};"
  "cf_worker.testFunction(3);"
  "/*chrome.runtime.sendMessage({"
    "type: 'hiddenClassIdSelectors',"
    "classes: notYetQueriedClasses || [],"
    "ids: notYetQueriedIds || []"
  "})*/"
  "notYetQueriedClasses = [];"
  "notYetQueriedIds = [];"
"};"

"function isElement (node) {"
  "return (node.nodeType === 1);"
"};"

"function asElement (node) {"
  "return isElement(node) ? node : null;"
"};"

"const handleMutations = MutationCallback = function (mutations) {"
  "for (const aMutation of mutations) {"
  //" console.log('!!!aMutation.type == ' + aMutation.type);"
    "if (aMutation.type === 'attributes') {"
      "const changedElm = aMutation.target;"
      "switch (aMutation.attributeName) {"
        "case 'class':"
          "for (const aClassName of changedElm.classList.values()) {"
            "if (queriedClasses.has(aClassName) === false) {"
              "notYetQueriedClasses.push(aClassName);"
              "queriedClasses.add(aClassName);"
            "};"
          "};"
          "break;"
        "case 'id':"
          "const mutatedId = changedElm.id;"
          "if (queriedIds.has(mutatedId) === false) {"
            "notYetQueriedIds.push(mutatedId);"
            "queriedIds.add(mutatedId);"
          "};"
          "break;"
      "};"
    "} else if (aMutation.addedNodes.length > 0) {"
      "for (const node of aMutation.addedNodes) {"
        "const element = asElement(node);"
        "if (!element) {"
          "continue;"
        "};"
        "const id = element.id;"
        "if (id && !queriedIds.has(id)) {"
          "notYetQueriedIds.push(id);"
          "queriedIds.add(id);"
        "};"
        "const classList = element.classList;"
        "if (classList) {"
          "for (const className of classList.values()) {"
            "if (className && !queriedClasses.has(className)) {"
              "notYetQueriedClasses.push(className);"
              "queriedClasses.add(className);"
            "};"
          "};"
        "};"
      "};"
    "};"
  "};"
  "fetchNewClassIdRules();"
"};"

    "const startObserving = () => {"
    "    const elmWithClassOrId = document.querySelectorAll('[class],[id]');"
  " for (const elm of elmWithClassOrId) {"
    " for (const aClassName of elm.classList.values()) {"
      " queriedClasses.add(aClassName);"
      " /*console.log('!!!aClassName == ' + aClassName);*/"
    "}"
    " const elmId = elm.getAttribute('id');"
    " if (elmId) {"
      " queriedIds.add(elmId);"
    " }"
  "};"
  "notYetQueriedClasses = Array.from(queriedClasses);"
  "notYetQueriedIds = Array.from(queriedIds);"
  "fetchNewClassIdRules();"
  "cosmeticObserver = new MutationObserver(handleMutations);"
  "let observerConfig = {"
    "subtree: true,"
    "childList: true,"
    "attributeFilter: ['id', 'class']"
  "};"
  "cosmeticObserver.observe(document.documentElement, observerConfig);"
  "};"
  "startObserving();"
    "})();";
bool ShouldDoCosmeticFiltering(content::WebContents* contents,
    const GURL& url) {
  Profile* profile = Profile::FromBrowserContext(contents->GetBrowserContext());
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);

  return ::brave_shields::ShouldDoCosmeticFiltering(map, url);
}

std::unique_ptr<base::ListValue> GetUrlCosmeticResourcesOnTaskRunner(
    const std::string& url) {
  auto result_list = std::make_unique<base::ListValue>();

  base::Optional<base::Value> resources = g_brave_browser_process->
      ad_block_service()->UrlCosmeticResources(url);

  if (!resources || !resources->is_dict()) {
    return result_list;
  }

  base::Optional<base::Value> regional_resources = g_brave_browser_process->
      ad_block_regional_service_manager()->UrlCosmeticResources(url);

  if (regional_resources && regional_resources->is_dict()) {
    ::brave_shields::MergeResourcesInto(
        std::move(*regional_resources), &*resources, /*force_hide=*/false);
  }

  base::Optional<base::Value> custom_resources = g_brave_browser_process->
      ad_block_custom_filters_service()->UrlCosmeticResources(url);

  if (custom_resources && custom_resources->is_dict()) {
    ::brave_shields::MergeResourcesInto(
        std::move(*custom_resources), &*resources, /*force_hide=*/true);
  }

  result_list->Append(std::move(*resources));

  return result_list;
}

void GetUrlCosmeticResourcesOnUI(content::RenderFrameHost* render_frame_host,
  std::unique_ptr<base::ListValue> resources) {
  if (!resources) {
    return;
  }
  for (auto i = resources->GetList().begin();
      i < resources->GetList().end(); i++) {
    base::DictionaryValue* resources_dict;
    if (!i->GetAsDictionary(&resources_dict)) {
      continue;
    }
    std::string to_inject;
    resources_dict->GetString("injected_script", &to_inject);
    if (to_inject.length() > 1) {
      render_frame_host->ExecuteJavaScriptInIsolatedWorld(
          base::UTF8ToUTF16(to_inject),
          base::NullCallback(), ISOLATED_WORLD_ID_CHROME_INTERNAL);
    }
  }
}
}  // namespace


BraveCosmeticResourcesTabHelper::BraveCosmeticResourcesTabHelper(
    content::WebContents* contents)
    : WebContentsObserver(contents) {
}

BraveCosmeticResourcesTabHelper::~BraveCosmeticResourcesTabHelper() {
}

void BraveCosmeticResourcesTabHelper::ProcessURL(
    content::WebContents* contents,
    content::RenderFrameHost* render_frame_host, const GURL& url,
    const bool& main_frame) {
  if (!ShouldDoCosmeticFiltering(contents, url)) {
    return;
  }
  g_brave_browser_process->ad_block_service()->GetTaskRunner()->
      PostTaskAndReplyWithResult(FROM_HERE,
          base::BindOnce(&GetUrlCosmeticResourcesOnTaskRunner, url.spec()),
          base::BindOnce(&GetUrlCosmeticResourcesOnUI, render_frame_host));
  if (!main_frame)
    return;
  // Non-scriptlet cosmetic filters are only applied on the top-level frame
  if (web_contents()->GetMainFrame()) {
    web_contents()->GetMainFrame()->ExecuteJavaScriptInIsolatedWorld(
        base::UTF8ToUTF16(k_observing_script),
        base::NullCallback(), ISOLATED_WORLD_ID_CHROME_INTERNAL);
  }
}

void BraveCosmeticResourcesTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  ProcessURL(web_contents(), web_contents()->GetMainFrame(),
      web_contents()->GetLastCommittedURL(), true);
}

void BraveCosmeticResourcesTabHelper::ResourceLoadComplete(
    content::RenderFrameHost* render_frame_host,
    const content::GlobalRequestID& request_id,
    const blink::mojom::ResourceLoadInfo& resource_load_info) {
  ProcessURL(web_contents(), render_frame_host, resource_load_info.final_url, false);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveCosmeticResourcesTabHelper)
