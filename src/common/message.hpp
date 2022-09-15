#pragma once

#include "common.hpp"
#include <sstream>

namespace simp {
namespace {
  void lex(std::string const& s, std::vector<std::string> &v, char lex){
    std::string temp = "";

    for(int i=0;i<s.size();++i){
      if(s[i]==lex){
        v.push_back(temp);
        temp = "";
      }
      else{
        temp.push_back(s[i]);
      }
    }
    v.push_back(temp);
  }
}


template<typename T>
struct message_header
{
  T id;
  uint32_t size = 0;
};


template<typename T>
struct message
{
  message_header<T> header{};
  std::vector<uint8_t> body;

  size_t size() const {
    return  body.size();
  }

  friend std::ostream& operator <<(std::ostream& os, const message<T>& msg) {
    os << "[\n\t{id=" << int(msg.header.id) << "},\n\t{size=" << msg.header.size << "},\n\t{content=\"" << std::string{msg.body.begin(), msg.body.end()} << "\"}\n]";
    return os;
  }

  void set_content(std::vector<std::string> const& msg) {

    std::stringstream ss;

    for (auto x : msg) {
      ss << x;
      ss << '\x01';
    }

    const auto str = ss.str().erase(ss.str().size() - 1, 1);
    header.size = str.size();

    std::vector<uint8_t> tmp{str.begin(), str.end()};
    body = tmp;
  }

  std::vector<std::string> body_to_string() {
    std::string str_data{body.begin(), body.end()};
    std::vector<std::string> ret;

    lex(str_data, ret, '\x01');

    return ret;
  }
};

template<typename T>
class connection;

template<typename T>
struct owned_message
{
  std::shared_ptr<connection<T>> remote = nullptr;
  message<T> msg;

  friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg) {
    os << msg.msg;
    return os;
  }
};

};
