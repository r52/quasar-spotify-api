cmake_minimum_required(VERSION 3.21)

project(quasar-spotify-api)

set(CMAKE_AUTOMOC ON)

find_package(fmt CONFIG REQUIRED)
find_package(jsoncons CONFIG REQUIRED)
find_package(Qt6 6.4 CONFIG COMPONENTS Core Gui Network NetworkAuth REQUIRED)
qt_standard_project_setup()

add_library(quasar-spotify-api MODULE
    quasar-spotify-api.cpp
    spotifyapi.cpp
)
target_compile_features(quasar-spotify-api PRIVATE cxx_std_20)
target_compile_definitions(quasar-spotify-api PRIVATE JSONCONS_HAS_STD_SPAN JSONCONS_HAS_STD_ENDIAN)

target_link_libraries(quasar-spotify-api PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::NetworkAuth)
target_link_libraries(quasar-spotify-api PRIVATE fmt::fmt)
target_link_libraries(quasar-spotify-api PRIVATE jsoncons)
target_link_libraries(quasar-spotify-api PRIVATE quasar extension-api)

if(WIN32)
  add_custom_command(TARGET quasar-spotify-api POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:quasar-spotify-api> $<TARGET_FILE_DIR:quasar>/extensions/quasar-spotify-api.dll
  )
endif()

install(TARGETS quasar-spotify-api DESTINATION quasar/extensions)
