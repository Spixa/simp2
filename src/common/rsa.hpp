#pragma once

#include <botan/auto_rng.h>
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <botan/pk_keys.h>
#include <botan/pkcs8.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/rsa.h>
#include <iostream>

#include <cstdlib>
#include <memory.h>
#include <memory>
#include <unordered_map>

using namespace Botan;

namespace simp {
namespace cipher {
namespace rsa {
class rsa_decryptor {
public:
  typedef std::shared_ptr<rsa_decryptor> ptr;
  typedef secure_vector<uint8_t> bytes;
  static constexpr int EXPONENT_MAX = 65537;

public:
  rsa_decryptor();
  rsa_decryptor(rsa_decryptor const &other) = delete;
  rsa_decryptor &operator=(rsa_decryptor const &other) = delete;
  virtual ~rsa_decryptor() {}

  static rsa_decryptor::ptr create();

public:
  bytes decrypt(bytes ciphertext);
  BigInt public_key();

private:
  AutoSeeded_RNG rng;
  std::unique_ptr<RSA_PrivateKey> pk;
};

class rsa_encryptor {
public:
  typedef std::shared_ptr<rsa_encryptor> ptr;
  typedef secure_vector<uint8_t> bytes;
  static constexpr int EXPONENT_MAX = 65537;

public:
  rsa_encryptor() = default;
  rsa_encryptor(const rsa_encryptor &other) = delete;
  rsa_encryptor &operator=(const rsa_encryptor &other) = delete;
  virtual ~rsa_encryptor() {}

  static rsa_encryptor::ptr create();

public:
  std::vector<uint8_t> encrypt(uint32_t id, std::string const &plaintext);
  void new_key(uint32_t id, BigInt const &n) {
    keys.insert({id, RSA_PublicKey(n, EXPONENT_MAX)});
  }

  void rm_key(uint32_t id) { keys.erase(id); }

private:
  std::unordered_map<uint32_t, RSA_PublicKey> keys;
  AutoSeeded_RNG rng{};
};
} // namespace rsa
} // namespace cipher
} // namespace simp
