cmake_minimum_required(VERSION 2.8)
project( TerminalVideo )

#if(NOT CMAKE_BUILD_TYPE)
#  set(CMAKE_BUILD_TYPE Release)
#endif()

#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
#set(CMAKE_CXX_FLAGS_DEBUG "-g")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_BUILD_TYPE Debug)

#string(REGEX REPLACE "([\\/\\-]O)3" "\\12"
#  CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#string(REGEX REPLACE "([\\/\\-]O)3" "\\12"
#  CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
#string(REGEX REPLACE "([\\/\\-]O)3" "\\12"
#  CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
#string(REGEX REPLACE "([\\/\\-]O)3" "\\12"
#  CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

find_package(SFML REQUIRED system window graphics audio)
include_directories( ${SFML_INCLUDE_DIRS} )

add_executable( TerminalVideo main.cpp )
include_directories( "./" )

target_link_libraries( TerminalVideo ${OpenCV_LIBS} sfml-audio sfml-window)