#pragma once

#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <botan/pkcs8.h>
#include <botan/pk_keys.h>
#include <botan/pubkey.h>
#include <botan/rsa.h>
#include <string>

using namespace Botan;

namespace simp {
  namespace cipher {
    const int EXPONENT_MAX = 65537;
    class Aes {
    public:
      static secure_vector<uint8_t> encrypt(std::string const& msg, const char keyhex[]);
      static std::unordered_map<std::string, secure_vector<uint8_t>> decrypt(const secure_vector<uint8_t>& ct, const char keyhex[]);
    private:
      static std::unique_ptr<Cipher_Mode> encryptor, decryptor;
      static AutoSeeded_RNG rng;
    };
  }
}
