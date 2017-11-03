cmake_minimum_required(VERSION 3.9)

project(quasar-spotify)

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
endif()

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/quasar")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Core Network REQUIRED)

set(SOURCES
    spotifyquasar.cpp
    spotify_quasar.cpp)

add_library(quasar-spotify SHARED ${SOURCES})
target_compile_features(quasar-spotify PUBLIC cxx_std_17)
target_link_libraries(quasar-spotify Qt5::Core Qt5::Network)
target_include_directories(quasar-spotify PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} api/)

install(TARGETS quasar-spotify DESTINATION quasar/plugins)
