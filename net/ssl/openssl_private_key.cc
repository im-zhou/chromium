#include "net/base/net_errors.h"
#include "net/ssl/ssl_platform_key_util.h"
#include "third_party/boringssl/src/include/openssl/base.h"
#include "third_party/boringssl/src/include/openssl/bio.h"
#include "third_party/boringssl/src/include/openssl/digest.h"
#include "third_party/boringssl/src/include/openssl/evp.h"
#include "third_party/boringssl/src/include/openssl/pem.h"
#include "third_party/boringssl/src/include/openssl/rsa.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"

#include "openssl_private_key.h"

namespace net {

OpenSSLPrivateKey::OpenSSLPrivateKey(bssl::UniquePtr<EVP_PKEY> key)
    : key_(std::move(key)) {}

OpenSSLPrivateKey::~OpenSSLPrivateKey() = default;

std::string OpenSSLPrivateKey::GetProviderName() {
  return "EVP_PKEY";
}

std::vector<uint16_t> OpenSSLPrivateKey::GetAlgorithmPreferences() {
  return SSLPrivateKey::DefaultAlgorithmPreferences(EVP_PKEY_id(key_.get()),
                                                    true /* supports PSS */);
}

Error OpenSSLPrivateKey::Sign(uint16_t algorithm,
                              base::span<const uint8_t> input,
                              std::vector<uint8_t>* signature) {
  bssl::ScopedEVP_MD_CTX ctx;
  EVP_PKEY_CTX* pctx;
  if (!EVP_DigestSignInit(ctx.get(), &pctx,
                          SSL_get_signature_algorithm_digest(algorithm),
                          nullptr, key_.get())) {
    return ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED;
  }
  if (SSL_is_signature_algorithm_rsa_pss(algorithm)) {
    if (!EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PSS_PADDING) ||
        !EVP_PKEY_CTX_set_rsa_pss_saltlen(pctx, -1 /* hash length */)) {
      return ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED;
    }
  }
  size_t sig_len = 0;
  if (!EVP_DigestSign(ctx.get(), nullptr, &sig_len, input.data(),
                      input.size())) {
    return ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED;
  }
  signature->resize(sig_len);
  if (!EVP_DigestSign(ctx.get(), signature->data(), &sig_len, input.data(),
                      input.size())) {
    return ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED;
  }
  signature->resize(sig_len);
  return OK;
}

scoped_refptr<SSLPrivateKey> LoadPrivateKeyFromPEM(
    const base::StringPiece& data) {
  bssl::UniquePtr<BIO> bio(BIO_new_mem_buf(const_cast<char*>(data.data()),
                                           static_cast<int>(data.size())));
  if (!bio) {
    LOG(ERROR) << "Could not allocate BIO for buffer?";
    return nullptr;
  }
  bssl::UniquePtr<EVP_PKEY> key(
      PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr));
  if (!key) {
    LOG(ERROR) << "Could not decode private key data.";
    return nullptr;
  }
  return base::MakeRefCounted<ThreadedSSLPrivateKey>(
      std::make_unique<OpenSSLPrivateKey>(std::move(key)),
      GetSSLPlatformKeyTaskRunner());
}

}  // namespace net
