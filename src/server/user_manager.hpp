#include <string>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <tinyxml2.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace simp {
    class user_manager {
    public:

        user_manager(std::string const& server_name) : server_dir_(server_name + "/db/userdata.xml") {
            reload_file();
            spdlog::get("console")->info("User manager v1 enabled (XML database)");
        }

        bool check_user_exists(std::string const& username) {
            reload_file();
            spdlog::get("console")->debug("Checking if user: " + username + " exists");

            tinyxml2::XMLNode *users = doc.FirstChildElement("users");
            if (users == nullptr) {
                spdlog::error(server_dir_ + " doesn't contain node <user>");
                throw std::runtime_error("An error occured while reading " + server_dir_);
            }
        }

        bool check_password(std::string const& username, std::string const& passwd);
        bool register_user(std::string const& username, std::string const& passwd);
        bool change_user_password(std::string const& username, std::string const& old_pass, std::string const& new_pass);
    private:
        void reload_file() {
            // bool firstTime = false;
            // if (!fs::exists(server_dir_)) {
            //     firstTime = true;
            // }

            // spdlog::get("console")->debug("reload_file(" + server_dir_ + ") called");
            // tinyxml2::XMLError result = doc.LoadFile(server_dir_.data());
            // if (result != tinyxml2::XML_SUCCESS) {
            //     spdlog::error("An error occured while reading " + server_dir_);
            //     throw std::runtime_error("An error occured while reading " + server_dir_);
            // }

            // if (firstTime) {
            //     tinyxml2::XMLElement* eUsers = doc.NewElement("users");
            //     doc.InsertEndChild(eUsers);
            // }

        }

        std::string server_dir_;
        tinyxml2::XMLDocument doc;
    };
}

