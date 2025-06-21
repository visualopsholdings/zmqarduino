/*
  zmqarduino.cpp
  
  Author: Paul Hamilton (paul@visualops.com)
  Date: 18-Jun-2023
    
  Command line driver for ZMQ to Arduino integration.
  
  This work is licensed under the Creative Commons Attribution 4.0 International License. 
  To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or 
  send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

  https://github.com/visualopsholdings/zmqarduino
*/

#include "server.hpp"

#include <iostream>
#include <boost/program_options.hpp> 
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <zmq.hpp>

namespace po = boost::program_options;

using namespace std;

int main(int argc, char *argv[]) {

  string version = "ZMQArduino 1.1, 21-Jun-2025.";

  int pushPort;
  int pullPort;
  int reqPort;
  int cadence;
  int baudrate;
  string logLevel;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("pullPort", po::value<int>(&pullPort)->default_value(5558), "ZMQ Pull port.")
    ("pushPort", po::value<int>(&pushPort)->default_value(5559), "ZMQ Push port.")
    ("reqPort", po::value<int>(&reqPort)->default_value(3013), "ZMQ Req port.")
    ("cadence", po::value<int>(&cadence)->default_value(200), "Device check cadence in milliseconds.")
    ("baudrate", po::value<int>(&baudrate)->default_value(9600), "Baud rate.")
    ("logLevel", po::value<string>(&logLevel)->default_value("info"), "Logging level [trace, debug, warn, info].")
    ("help", "produce help message")
    ;
  po::positional_options_description p;

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
          options(desc).positional(p).run(), vm);
  po::notify(vm);   

  if (logLevel == "trace") {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace);
  }
  else if (logLevel == "debug") {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
  }
  else if (logLevel == "warn") {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::warning);
  }
  else {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
  }

  boost::log::formatter logFmt =
         boost::log::expressions::format("%1%\tNodes IRC [%2%]\t%3%")
        %  boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f") 
        %  boost::log::expressions::attr< boost::log::trivial::severity_level>("Severity")
        %  boost::log::expressions::smessage;
  boost::log::add_common_attributes();
  boost::log::add_console_log(clog)->set_formatter(logFmt);

  BOOST_LOG_TRIVIAL(info) << version;

  if (vm.count("help")) {
    cout << desc << endl;
    return 1;
  }
 
  zmq::context_t context (1);
  zmq::socket_t pull(context, ZMQ_PULL);
  pull.bind("tcp://127.0.0.1:" + to_string(pullPort));
  cout << "Bind to ZMQ as PULL on " << pullPort << endl;

  zmq::socket_t push(context, ZMQ_PUSH);
  push.bind("tcp://127.0.0.1:" + to_string(pushPort));
  cout << "Bind to ZMQ as PUSH on " << pushPort << endl;
  
  Server server(&pull, &push, reqPort, cadence, baudrate);
  server.start();

}
