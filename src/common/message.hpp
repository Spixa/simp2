#pragma once

#include <exception>
#include <sstream>
#include <vector>
#include <cinttypes>
#include <array>
#include <string>
#include <tuple>
#include <memory.h>
#include <iostream>

#include "protocol.hpp"

namespace simp {

namespace {
void lex(std::array<char, MAX_IP_PACK_SIZE> const& s, uint16_t& id, std::vector<std::string> &v, char lex){
  std::string temp = "";

  int seperator_count{0};
  for(int i=0;i<s.size();++i){
    if(s[i]==lex){

      if (seperator_count != 0) 
        v.push_back(temp);
      else { 
        try {
          id = std::stoi(temp);
        } catch(std::exception& e) {
          id = 1;
        }
      }

      temp = "";
      seperator_count++;
    }
    else{
      temp.push_back(s[i]);
    }
  }
  v.push_back(temp);
}
}


class chat_message {
public:
  chat_message() {
    // packet_.fill('\0');
  }

  std::array<char, simp::MAX_IP_PACK_SIZE>& get_data() { return packet_; }

  static std::tuple<uint16_t, std::vector<std::string>> decode(std::array<char, MAX_IP_PACK_SIZE> const& msg, char seperator = '\x01') {
    uint16_t return_id;
    std::vector<std::string> return_parameters;

    lex(msg, return_id, return_parameters, seperator);

    return std::make_tuple(return_id, return_parameters);
  }

  static void tidy_print(chat_message o) {
    auto g = chat_message::decode(o.get_data());
    
    std::cout << std::get<uint16_t>(g);

    auto v = std::get<std::vector<std::string>>(g);

    for (auto x : v) {
      std::cout << " -- " << x;
    }
    std::cout << std::endl;
  }

  void set_message(uint16_t pack_id, std::vector<std::string> const& strings, char seperator = '\x01' ) {
    // std::stringstream ss{std::stringstream::out | std::ios::binary};
    // ss << pack_id;

    // for (auto x : strings) {
    //   ss << seperator << x;
    // }

    // ss.seekg(0, std::ios::end);
    // int size = ss.tellg();
    // std::cout << "Size: " << size << std::endl;

    // const auto str = ss.str(); // ss.view() is better on C++20

    std::string str;

    str += std::to_string(pack_id);

    for (auto x : strings) {
      str += std::string(seperator + x);
    }

    std::cout << "string data: [" << str.data() << "] with size: " << str.size() << std::endl;

    std::copy(str.begin(), str.end(), packet_.begin());
    std::cout << "packet data: " << packet_.data() << std::endl;

  }
private:
  std::array<char, MAX_IP_PACK_SIZE> packet_{};
};

};