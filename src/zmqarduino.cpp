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
#include <zmq.hpp>

namespace po = boost::program_options;

using namespace std;

int main(int argc, char *argv[]) {

	int pushPort;
	int pullPort;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("pullPort", po::value<int>(&pullPort)->default_value(5558), "ZMQ Pull port.")
    ("pushPort", po::value<int>(&pushPort)->default_value(5559), "ZMQ Push port.")
    ("help", "produce help message")
    ;
  po::positional_options_description p;
  
  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
          options(desc).positional(p).run(), vm);
  po::notify(vm);   
    
	zmq::context_t context (1);
	zmq::socket_t pull(context, ZMQ_PULL);
	pull.bind("tcp://127.0.0.1:" + to_string(pullPort));
	cout << "Connect to ZMQ as PULL on " << pullPort << endl;
	
	zmq::socket_t push(context, ZMQ_PUSH);
	push.bind("tcp://127.0.0.1:" + to_string(pushPort));
	cout << "Connect to ZMQ as PUSH on " << pushPort << endl;
  
  Server server(&pull, &push);
  server.run();
  
}
