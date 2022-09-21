cmake_minimum_required(VERSION 3.11)

# ----------------------------------------------------------
# Open Secure Instant Messagin Protocol v2.x CMakeLists.txt
# Licensed under {PLACEHOLDER_LICENSE_NAME}
# ----------------------------------------------------------

include(FetchContent)
# ----------------------------------------------------------
# Function TUI library

# set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
# FetchContent_Declare(ftxui
#   GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
#   GIT_TAG v3.0.0
# )

# FetchContent_GetProperties(ftxui)
# if(NOT ftxui_POPULATED)
#   FetchContent_Populate(ftxui)
#   add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
# endif()

# ----------------------------------------------------------
# ASIO
FetchContent_Declare(asio-cmake
  GIT_REPOSITORY https://github.com/kingsamchen/asio-cmake.git
  GIT_TAG        origin/master
)
set(ASIO_CMAKE_ASIO_TAG asio-1-12-2)
FetchContent_MakeAvailable(asio-cmake)

# ----------------------------------------------------------
# A fast and simple to use logging library.
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.8.2)

FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
  message("Cloning spdlog...")
  FetchContent_Populate(spdlog)
  add_subdirectory(
    ${spdlog_SOURCE_DIR}
    ${spdlog_BINARY_DIR})
endif()

# ----------------------------------------------------------
# Project definition
project(simp2 VERSION 1.0)
# add_definitions(-std=c++17 -std=gnu++17)

# ----------------------------------------------------------
# Required packages
find_package(Threads REQUIRED)

link_directories(${SPDLOG_LIB}/lib)

# ----------------------------------------------------------
# Sources
set(SIMPSERVER_SOURCES
  ${CMAKE_SOURCE_DIR}/src/server/main.cpp
)

set(SIMPCLIENT_SOURCES
  ${CMAKE_SOURCE_DIR}/src/client/main.cpp
)

# ----------------------------------------------------------
# Executables
add_executable(simpserver ${SIMPSERVER_SOURCES})
add_executable(simpclient ${SIMPCLIENT_SOURCES})

# ----------------------------------------------------------
# Include sources
target_include_directories(simpserver PUBLIC ${CMAKE_SOURCE_DIR}/src ${asio_SOURCE_DIR} ${SPDLOG_LIB}/include)
target_include_directories(simpclient PUBLIC ${CMAKE_SOURCE_DIR}/src ${asio_SOURCE_DIR} ${SPDLOG_LIB}/include)

# ----------------------------------------------------------
# Linkage
link_libraries(asio spdlog)
