cmake_minimum_required(VERSION 3.9)

project(quasar-spotify-api)

set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
        STRING "Choose the type of build." FORCE)

    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("DEBUG BUILD")
else()
    add_definitions(-DQT_MESSAGELOGCONTEXT)
    add_definitions(-DNDEBUG)
endif()

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/quasar")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 5.12 COMPONENTS Core Gui Network NetworkAuth REQUIRED)

set(SOURCES
    quasar-spotify-api.cpp
    spotifyapi.cpp)

add_library(quasar-spotify-api SHARED ${SOURCES})
target_compile_features(quasar-spotify-api PUBLIC cxx_std_17)
target_link_libraries(quasar-spotify-api Qt5::Core Qt5::Gui Qt5::Network Qt5::NetworkAuth)
target_include_directories(quasar-spotify-api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} quasar/extension-api/)

install(TARGETS quasar-spotify-api DESTINATION quasar/extensions)
