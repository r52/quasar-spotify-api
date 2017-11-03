cmake_minimum_required(VERSION 3.9)

project(quasar-spotify)

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
