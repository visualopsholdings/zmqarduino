#   CMakeLists.txt
#
#   Author: Paul Hamilton (paul@visualops.com)
#   Date: 17-Jun-2023
#
#   Input file for ZMQ Serial driver etc for Visual Ops
#
#  This work is licensed under the Creative Commons Attribution 4.0 International License. 
#  To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or 
#  send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
#
#  https://github.com/visualopsholdings/zmqarduino

cmake_minimum_required(VERSION 3.5)

project (zmqarduino)
  find_package(Boost COMPONENTS program_options filesystem unit_test_framework chrono thread REQUIRED)
  find_package(cppzmq)
  add_definitions(-DBOOST_ALL_DYN_LINK) 
  enable_testing()

  set(CMAKE_CXX_STANDARD 11)
#  set(CMAKE_BUILD_TYPE Debug)

if (UNIX AND NOT APPLE)
  set(LIBS zmq)
  add_definitions(-funwind-tables) 
endif ()
if (APPLE)
  set(LIBS zmq)
  include_directories(/usr/local/include)
  LINK_DIRECTORIES(/opt/homebrew/Cellar/zeromq/4.3.4/lib)
endif ()
set(BOOSTLIBS ${Boost_CHRONO_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})

include_directories(include)

add_executable(ZMQArduino src/zmqarduino.cpp src/server.cpp src/AsyncSerial.cpp src/BufferedAsyncSerial.cpp)
  target_link_libraries(ZMQArduino zmq ${BOOSTLIBS})
