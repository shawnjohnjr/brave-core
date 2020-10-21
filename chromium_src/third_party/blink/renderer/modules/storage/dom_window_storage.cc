/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../../../third_party/blink/renderer/modules/storage/dom_window_storage.cc"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/modules/storage/brave_dom_window_storage.h"

namespace blink {

namespace {

// If storage is null and there was an exception then clear the exception unless
// it was caused by CanAccessSessionStorage for the document security origin
// (sandbox, data urls, etc...)
void MaybeClearAccessDeniedException(StorageArea* storage,
                                     const LocalDOMWindow& window,
                                     ExceptionState* exception_state) {
  if (!storage && exception_state->HadException()) {
    if (!window.GetSecurityOrigin()->CanAccessSessionStorage())
      return;

    // clear the access denied exception for better webcompat
    exception_state->ClearException();
  }
}

}  // namespace

class EphemeralStorageNamespaces
    : public GarbageCollected<EphemeralStorageNamespaces>,
      public Supplement<Page> {
 public:
  EphemeralStorageNamespaces(StorageController* controller,
                             const String& session_storage_id,
                             const String& local_storage_id);
  virtual ~EphemeralStorageNamespaces() = default;

  static const char kSupplementName[];
  static EphemeralStorageNamespaces* From(Page* page, LocalDOMWindow* window);

  StorageNamespace* session_storage() { return session_storage_.Get(); }
  StorageNamespace* local_storage() { return local_storage_.Get(); }
  void Trace(Visitor* visitor) const override;

 private:
  Member<StorageNamespace> session_storage_;
  Member<StorageNamespace> local_storage_;
};

const char EphemeralStorageNamespaces::kSupplementName[] =
    "EphemeralStorageNamespaces";

EphemeralStorageNamespaces::EphemeralStorageNamespaces(
    StorageController* controller,
    const String& session_storage_id,
    const String& local_storage_id)
    : session_storage_(
          MakeGarbageCollected<StorageNamespace>(controller,
                                                 session_storage_id)),
      local_storage_(MakeGarbageCollected<StorageNamespace>(controller,
                                                            local_storage_id)) {
}

void EphemeralStorageNamespaces::Trace(Visitor* visitor) const {
  visitor->Trace(session_storage_);
  visitor->Trace(local_storage_);
  Supplement<Page>::Trace(visitor);
}

// static
EphemeralStorageNamespaces* EphemeralStorageNamespaces::From(
    Page* page,
    LocalDOMWindow* window) {
  DCHECK(window);
  if (!page)
    return nullptr;

  EphemeralStorageNamespaces* supplement =
      Supplement<Page>::From<EphemeralStorageNamespaces>(page);
  if (supplement)
    return supplement;

  Document* document = window->GetFrame()->GetDocument();
  auto* web_frame = WebLocalFrameImpl::FromFrame(document->GetFrame());
  WebViewImpl* webview = web_frame->ViewImpl();
  if (!webview || !webview->Client())
    return nullptr;
  String session_storage_id =
      String::FromUTF8(webview->Client()->GetSessionStorageNamespaceId()) +
      String("/ephemeral-session-storage");

  auto* security_origin =
      page->MainFrame()->GetSecurityContext()->GetSecurityOrigin();
  String domain = security_origin->RegistrableDomain();
  // RegistrableDomain might return an empty string if this host is an IP
  // address or a file URL.
  if (domain.IsEmpty())
    domain = String::FromUTF8(security_origin->ToUrlOrigin().Serialize());

  String local_storage_id = domain + String("/ephemeral-local-storage");
  supplement = MakeGarbageCollected<EphemeralStorageNamespaces>(
      StorageController::GetInstance(), session_storage_id, local_storage_id);

  ProvideTo(*page, supplement);
  return supplement;
}

// static
const char BraveDOMWindowStorage::kSupplementName[] = "BraveDOMWindowStorage";

// static
BraveDOMWindowStorage& BraveDOMWindowStorage::From(LocalDOMWindow& window) {
  BraveDOMWindowStorage* supplement =
      Supplement<LocalDOMWindow>::From<BraveDOMWindowStorage>(window);
  if (!supplement) {
    supplement = MakeGarbageCollected<BraveDOMWindowStorage>(window);
    ProvideTo(window, supplement);
  }
  return *supplement;
}

// static
StorageArea* BraveDOMWindowStorage::sessionStorage(
    LocalDOMWindow& window,
    ExceptionState& exception_state) {
  return From(window).sessionStorage(exception_state);
}

// static
StorageArea* BraveDOMWindowStorage::localStorage(
    LocalDOMWindow& window,
    ExceptionState& exception_state) {
  return From(window).localStorage(exception_state);
}

BraveDOMWindowStorage::BraveDOMWindowStorage(LocalDOMWindow& window)
    : Supplement<LocalDOMWindow>(window) {}

StorageArea* BraveDOMWindowStorage::sessionStorage(
    ExceptionState& exception_state) {
  LocalDOMWindow* window = GetSupplementable();
  auto* storage =
      DOMWindowStorage::From(*window).sessionStorage(exception_state);

  MaybeClearAccessDeniedException(storage, *window, &exception_state);
  if (base::FeatureList::IsEnabled(features::kBraveEphemeralStorage)) {
    storage = ephemeralSessionStorage(storage);
  }

  return storage;
}

StorageArea* BraveDOMWindowStorage::ephemeralSessionStorage(
    StorageArea* non_ephemeral_storage) {
  if (!base::FeatureList::IsEnabled(features::kBraveEphemeralStorage))
    return non_ephemeral_storage;

  LocalDOMWindow* window = GetSupplementable();
  if (!window->IsCrossSiteSubframe())
    return non_ephemeral_storage;

  // If we were not able to create non-ephemeral session storage for this
  // window, then don't attempt to create an ephemeral version.
  if (!non_ephemeral_storage)
    return nullptr;

  if (ephemeral_session_storage_)
    return ephemeral_session_storage_;

  Page* page = window->GetFrame()->GetDocument()->GetPage();
  EphemeralStorageNamespaces* namespaces =
      EphemeralStorageNamespaces::From(page, window);
  if (!namespaces)
    return nullptr;

  auto storage_area =
      namespaces->session_storage()->GetCachedArea(window->GetSecurityOrigin());

  Document* document = window->GetFrame()->GetDocument();
  ephemeral_session_storage_ =
      StorageArea::Create(document->GetFrame(), std::move(storage_area),
                          StorageArea::StorageType::kSessionStorage);
  return ephemeral_session_storage_;
}

StorageArea* BraveDOMWindowStorage::localStorage(
    ExceptionState& exception_state) {
  LocalDOMWindow* window = GetSupplementable();
  auto* storage = DOMWindowStorage::From(*window).localStorage(exception_state);

  MaybeClearAccessDeniedException(storage, *window, &exception_state);
  if (base::FeatureList::IsEnabled(features::kBraveEphemeralStorage)) {
    storage = ephemeralLocalStorage(storage);
  }

  return storage;
}

StorageArea* BraveDOMWindowStorage::ephemeralLocalStorage(
    StorageArea* non_ephemeral_storage) {
  if (!base::FeatureList::IsEnabled(features::kBraveEphemeralStorage))
    return non_ephemeral_storage;

  LocalDOMWindow* window = GetSupplementable();
  if (!window->IsCrossSiteSubframe())
    return non_ephemeral_storage;

  // If we were not able to create non-ephemeral local storage for this window,
  // then don't attempt to create an ephemeral version.
  if (!non_ephemeral_storage)
    return nullptr;

  if (ephemeral_local_storage_)
    return ephemeral_local_storage_;

  Page* page = window->GetFrame()->GetDocument()->GetPage();
  EphemeralStorageNamespaces* namespaces =
      EphemeralStorageNamespaces::From(page, window);
  if (!namespaces)
    return nullptr;

  auto storage_area =
      namespaces->local_storage()->GetCachedArea(window->GetSecurityOrigin());

  Document* document = window->GetFrame()->GetDocument();
  ephemeral_local_storage_ =
      StorageArea::Create(document->GetFrame(), std::move(storage_area),
                          StorageArea::StorageType::kSessionStorage);
  return ephemeral_local_storage_;
}

void BraveDOMWindowStorage::Trace(Visitor* visitor) const {
  visitor->Trace(ephemeral_session_storage_);
  visitor->Trace(ephemeral_local_storage_);
  Supplement<LocalDOMWindow>::Trace(visitor);
}

}  // namespace blink
