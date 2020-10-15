/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define VerifySeedSignature VerifySeedSignature_ChromiumImpl
#include "../../../../components/variations/variations_seed_store.cc"
#undef VerifySeedSignature

namespace variations {

// TODO(Moritz Haller): Explain plus todos for using our signature
VerifySignatureResult VerifySeedSignature(
    const std::string& seed_bytes,
    const std::string& base64_seed_signature) {
  return VerifySignatureResult::VALID_SIGNATURE;
}

}  // namespace variations
