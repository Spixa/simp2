cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 20)

project(simp2 VERSION 1.0)

# Required packages
find_package(Threads REQUIRED)

# ASIO
include(FetchContent)
FetchContent_Declare(asio-cmake
  GIT_REPOSITORY https://github.com/kingsamchen/asio-cmake.git
  GIT_TAG        origin/master
)
set(ASIO_CMAKE_ASIO_TAG asio-1-12-2)
FetchContent_MakeAvailable(asio-cmake)

# SOURCES
set(SIMPSERVER_SOURCES
  ${CMAKE_SOURCE_DIR}/src/server/main.cpp
)

set(SIMPCLIENT_SOURCES
  ${CMAKE_SOURCE_DIR}/src/client/main.cpp
)

add_executable(simpserver ${SIMPSERVER_SOURCES})
add_executable(simpclient ${SIMPCLIENT_SOURCES})

include_directories(${CMAKE_SOURCE_DIR}/src)
link_libraries(asio)
