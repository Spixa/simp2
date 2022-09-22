#include "aes.hpp"
#include <iostream>

namespace simp {
  namespace cipher {
    std::unique_ptr<Cipher_Mode> Aes::encryptor = Cipher_Mode::create("AES-256/GCM", ENCRYPTION);
    std::unique_ptr<Cipher_Mode> Aes::decryptor = Cipher_Mode::create("AES-256/GCM", DECRYPTION);
    AutoSeeded_RNG Aes::rng = AutoSeeded_RNG{};
  
    secure_vector<uint8_t> Aes::encrypt(const std::string & str, const char keyhex[]) {
      // defining variables
      secure_vector<uint8_t> input;
      secure_vector<uint8_t> key;
      
      try {
        key = hex_decode_locked(keyhex);
      } catch (std::exception e) {
        std::cerr<< "Error: encryption\n";
      }

      input = secure_vector<uint8_t>(str.data(), str.data() + str.length());
      secure_vector<uint8_t> iv = rng.random_vec(encryptor->default_nonce_length()); 

      encryptor->set_key(key);
      encryptor->start(iv);
      encryptor->finish(input);

      std::string res_str{hex_encode(iv) + hex_encode(input)}; 
      return hex_decode_locked(res_str);
    }
  std::unordered_map<std::string, secure_vector<uint8_t>> Aes::decrypt(const secure_vector<uint8_t>& ct, const char keyhex[]) {
    secure_vector<uint8_t> output;
    secure_vector<uint8_t> iv;
    secure_vector<uint8_t> key;

    try {
      key = hex_decode_locked(keyhex);
    } catch (std::exception e) {
      std::cerr<< "Error: decryptionion\n";
    }
    
    for (int i = 0; i < 12; i++) {
      iv.push_back(ct[i]);
    }

    for (int i = 12; i < ct.size(); i++) {
      output.push_back(ct[i]);
    }

    // start to decryptorompile 
    decryptor->set_key(key);
    decryptor->start(iv);

    decryptor->finish(output, 0); 

    std::unordered_map<std::string, secure_vector<uint8_t>> map;
    map["iv"] = iv;
    map["msg"] = output; 

    return map;
  }
  }
}