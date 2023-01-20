#pragma once

#include <deque>
#include <inttypes.h>
#include <string>
#include <spdlog/spdlog.h>
#include <tinyxml2.h>

void setup_logger();

int handle_args(int argc, char** argv, uint16_t& port, std::string& version, std::string& name);

int run(std::deque<std::string> args, uint16_t& port, std::string& version, std::string& name);

int create(uint16_t& port, std::string& version, std::string& name, std::string n);

int parse(uint16_t& port, std::string& version, std::string& name);