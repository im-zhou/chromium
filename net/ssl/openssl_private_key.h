#ifndef CHROMIUM_OPEN_SSL_PRIVATE_KEY_H
#define CHROMIUM_OPEN_SSL_PRIVATE_KEY_H

#include "net/ssl/threaded_ssl_private_key.h"

namespace net {

class NET_EXPORT OpenSSLPrivateKey : public ThreadedSSLPrivateKey::Delegate {
 public:
  explicit OpenSSLPrivateKey(bssl::UniquePtr<EVP_PKEY> key);

  OpenSSLPrivateKey(const OpenSSLPrivateKey&) = delete;

  OpenSSLPrivateKey& operator=(const OpenSSLPrivateKey&) = delete;

  ~OpenSSLPrivateKey() override;

  std::string GetProviderName() override;

  std::vector<uint16_t> GetAlgorithmPreferences() override;

  Error Sign(uint16_t algorithm,
             base::span<const uint8_t> input,
             std::vector<uint8_t>* signature) override;

 private:
  bssl::UniquePtr<EVP_PKEY> key_;
};

// Loads a PEM-encoded private key file from |data| into an SSLPrivateKey
// object. Returns the new SSLPrivateKey or nullptr on error.
NET_EXPORT scoped_refptr<SSLPrivateKey> LoadPrivateKeyFromPEM(
    const base::StringPiece& data);

}  // namespace net

#endif  // CHROMIUM_OPEN_SSL_PRIVATE_KEY_H
