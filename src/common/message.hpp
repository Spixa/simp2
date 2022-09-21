#pragma once

#include "common.hpp"
#include <cassert>
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


struct message_header
{
  uint32_t size = 0;
};


template<typename T>
struct message
{
  message_header header{};
  std::vector<uint8_t> body;

  size_t size() const {
    return  body.size();
  }

  friend std::ostream& operator <<(std::ostream& os, const message<T>& msg) {
    os << "[\n\t{id=" << int(msg.id) << "},\n\t{size=" << msg.header.size << "},\n\t{content=\"" << std::string{msg.body.begin(), msg.body.end()} << "\"}\n]";
    return os;
  }

  void set_content(T id, std::vector<std::string> const& msg) {
    std::stringstream ss;
    
    // Write the ID onto the stringstream
    ss << static_cast<uint32_t>(id);

    // Write the messages seperated with the \x01 byte
    for (auto x : msg) {
      ss << '\x01';
      ss << x;
    }

    // resize the header according to the message size
    const auto str = ss.str();
    header.size = str.size();

    // begin writing message to a temporary byte vector
    std::vector<uint8_t> tmp{str.begin(), str.end()};

    // Copy to the main body
    body = tmp;
  }

  /*
  This function gets the message ID which can be accessed with message::get_id() method
  And also returns a vector of string used to retrieve single messages
  */
  std::vector<std::string> decode_message() {
    // Used for assertion later on
    hasBodyDecoded = true;

    // Copy byte data into a string buffer
    std::string str_data{body.begin(), body.end()};

    // Return value
    std::vector<std::string> ret;

    // Lex data for each \x01 from str_data and write it to the ret vec
    lex(str_data, ret, '\x01');
    
    // retrieve the ID and downcast it to uint32_t
    id = std::atoi(ret.begin().base()->data());
    // Remove the ID from the vector
    ret.erase(ret.begin());
    
    return ret;
  }

  // Has to be ran after decode_message();
  T get_id() {
    assert(hasBodyDecoded && "Message was not decoded");

    return static_cast<T>(id);
  }
private:
  bool hasBodyDecoded = false;
  uint32_t id{};
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
