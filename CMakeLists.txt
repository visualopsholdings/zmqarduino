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
  find_package(Boost 1.85.0 COMPONENTS program_options filesystem unit_test_framework chrono thread log_setup log json REQUIRED)
  find_package(cppzmq)
  add_definitions(-DBOOST_ALL_DYN_LINK) 
  enable_testing()

  set(CMAKE_CXX_STANDARD 17)
#  set(CMAKE_BUILD_TYPE Debug)
  set(LIBS zmq)

if (UNIX AND NOT APPLE)
  add_definitions(-funwind-tables) 
endif ()
if (APPLE)
  include_directories(/usr/local/include)
  include_directories(/usr/local/include /opt/homebrew/Cellar/zeromq/4.3.5_1/include)
  LINK_DIRECTORIES(/opt/homebrew/Cellar/zeromq/4.3.5_1/lib)
endif ()
set(BOOSTLIBS ${Boost_CHRONO_LIBRARY} ${Boost_THREAD_LIBRARY} 
  ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} 
  ${Boost_LOG_LIBRARY} ${Boost_JSON_LIBRARY})
include_directories(include)

add_executable(ZMQArduino src/zmqarduino.cpp src/server.cpp src/connection.cpp 
    src/AsyncSerial.cpp src/BufferedAsyncSerial.cpp src/zmqclient.cpp)
  target_link_libraries(ZMQArduino ${LIBS} ${BOOSTLIBS})
