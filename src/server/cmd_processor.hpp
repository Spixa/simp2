#include <memory>
#include <vector>
#include <deque>
#include "../common//message.hpp"
namespace simp {
  class CommandProcessor {
  public:
    typedef std::shared_ptr<CommandProcessor> Ptr;
    
    CommandProcessor() {}

    void process(const std::string& t) {
      std::vector<std::string> v;
      lex(t, v, ' ');
      std::deque<std::string> d{v.begin(), v.end()};

      d.pop_front();

      if (d[0] == "help") {
        spdlog::info("Help received.");
      }
    }
  private: 
  };
}