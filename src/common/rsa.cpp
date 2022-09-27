#include "rsa.hpp"

namespace simp {
namespace cipher {
namespace rsa {

// RSA DECRYPTOR BEGIN
rsa_decryptor::ptr rsa_decryptor::create() {
  return std::make_shared<rsa_decryptor>();
}

rsa_decryptor::rsa_decryptor()
    : rng(), pk(std::make_unique<RSA_PrivateKey>(rng, 4096)) {}

BigInt rsa_decryptor::public_key() { return pk.get()->get_n(); }

rsa_decryptor::bytes rsa_decryptor::decrypt(rsa_decryptor::bytes input) {
  PK_Decryptor_EME decryption_device(*pk, rng, "PKCS1v15");

  return decryption_device.decrypt(input);
}

// RSA DECRYPTOR END

rsa_encryptor::ptr rsa_encryptor::create() {
  return std::make_shared<rsa_encryptor>();
}

std::vector<uint8_t> rsa_encryptor::encrypt(uint32_t id,
                                            std::string const &plaintext) {
  // Encryption device
  PK_Encryptor_EME encryption_device(keys.at(id), rng, "PKCS1v15");

  // Write to secure buf
  rsa_encryptor::bytes pt(plaintext.data(),
                          plaintext.data() + plaintext.length());

  // Encrypt to ciphertext
  std::vector<uint8_t> ct = encryption_device.encrypt(pt, rng);
  return ct;
}

} // namespace rsa
} // namespace cipher
} // namespace simp
