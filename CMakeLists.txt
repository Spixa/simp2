cmake_minimum_required(VERSION 3.11)

# ----------------------------------------------------------
# Open Secure Instant Messagin Protocol v2.x CMakeLists.txt
# Licensed under {PLACEHOLDER_LICENSE_NAME}
# ----------------------------------------------------------

project(simp2 VERSION 1.0)
# add_definitions(-std=c++20 -std=gnu++20)

# ----------------------------------------------------------
# Encryption library
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
ExternalProject_Add(BOTAN
    GIT_REPOSITORY https://github.com/randombit/botan.git
    GIT_TAG 2.19.2
    CONFIGURE_COMMAND python ${CMAKE_SOURCE_DIR}/build/BOTAN-prefix/src/BOTAN/configure.py --prefix=.
    BUILD_COMMAND "make"
		UPDATE_COMMAND ""
)
ExternalProject_Get_Property(BOTAN INSTALL_DIR)
set(BOTAN_LIB ${INSTALL_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/Modules)

# find_package(BOTAN QUIET)

include(FetchContent)
# ----------------------------------------------------------
# XML
FetchContent_Declare(tinyxml2
  GIT_REPOSITORY     https://github.com/leethomason/TinyXML2
  GIT_TAG            9.0.0
)
FetchContent_MakeAvailable(tinyxml2)
set(tinyxml2_LIBRARIES tinyxml2)
    
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
# FetchContent_Declare(asio-cmake
#   GIT_REPOSITORY https://github.com/kingsamchen/asio-cmake.git
#   GIT_TAG        origin/master
# )
# set(ASIO_CMAKE_ASIO_TAG asio-1-12-2)
# FetchContent_MakeAvailable(asio-cmake)

# ----------------------------------------------------------
# A fast and simple to use logging library.


# ----------------------------------------------------------
# Project definition


if(NOT TARGET spdlog)
    # Stand-alone build
    find_package(spdlog REQUIRED)
endif()


# ----------------------------------------------------------
# Required packages
find_package(Threads REQUIRED)

link_directories(${SPDLOG_LIB}/lib)

# ----------------------------------------------------------
# Sources
set(COMMON_SOURCES
  ${CMAKE_SOURCE_DIR}//src/common/aes.cpp
  ${CMAKE_SOURCE_DIR}/src/common/rsa.cpp
)
set(SIMPSERVER_SOURCES
  ${CMAKE_SOURCE_DIR}/src/server/main.cpp
  ${CMAKE_SOURCE_DIR}/src/server/server.cpp
  ${CMAKE_SOURCE_DIR}/src/server/setup.cpp
  ${CMAKE_SOURCE_DIR}/src/server/server_interface.cpp
)

set(SIMPCLIENT_SOURCES
  ${CMAKE_SOURCE_DIR}/src/client/main.cpp
)

# ----------------------------------------------------------
# Executables
add_executable(simpserver ${SIMPSERVER_SOURCES} ${COMMON_SOURCES})
add_executable(simpclient ${SIMPCLIENT_SOURCES} ${COMMON_SOURCES})

link_directories(${BOTAN_LIB}/lib)
include_directories(${BOTAN_LIB}/src/BOTAN-build/include/botan-3 ${SFML_LIB}/include)

# ----------------------------------------------------------
# Include sources
target_include_directories(simpserver PUBLIC ${CMAKE_SOURCE_DIR}/src ${asio_SOURCE_DIR} ${SPDLOG_LIB}/include ${BOTAN_INCLUDE_DIR})
target_include_directories(simpclient PUBLIC ${CMAKE_SOURCE_DIR}/src ${asio_SOURCE_DIR} ${SPDLOG_LIB}/include ${BOTAN_INCLUDE_DIR})

# ----------------------------------------------------------
# Linkage
target_link_libraries(simpserver PRIVATE ${tinxyml2_LIBRARIES} tinyxml2 ${BOTAN_LIBRARY})
target_link_libraries(simpclient PRIVATE ${tinxyml2_LIBRARIES} tinyxml2 ${BOTAN_LIBRARY})
link_libraries(asio spdlog:::spdlog ${tinyxml2_LIBRARIES})

# add_custom_command(TARGET simpserver POST_BUILD        # Adds a post-build event to MyTest
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
#         "${BOTAN_LIB}/src/BOTAN-build/libbotan-3.so.0"      # <--this is in-file
#         $<TARGET_FILE_DIR:simpserver>)                 # <--this is out-file path
