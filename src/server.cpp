/*
  server.cpp
  
  Author: Paul Hamilton (paul@visualops.com)
  Date: 18-Jun-2023
    
  This work is licensed under the Creative Commons Attribution 4.0 International License. 
  To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or 
  send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

  https://github.com/visualopsholdings/zmqarduino
*/

#include "server.hpp"
#include "BufferedAsyncSerial.h"

#include <iostream>
#include <chrono>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;
using json = nlohmann::json;
namespace fs = boost::filesystem;
using namespace boost::posix_time;

Server::~Server() {
  if (_serial) {
    _serial->close();
    delete _serial;
  }
}

void Server::sendjson(const json &m) {
  string msg = m.dump();
  zmq::message_t zmsg(msg.length());
  memcpy(zmsg.data(), msg.c_str(), msg.length());
  _push->send(zmsg);
}

void Server::connectserial(const string &name, const string &path, int baud) {

  cout << "connecting to " << path << endl;

  if (_serial) {
    _serial->close();
    delete _serial;
    _serial = 0;
  }
  
  try {
    _serial = new BufferedAsyncSerial(path, baud);
  }
  catch (boost::system::system_error& e) {
    cout << "open error: " << e.what() << endl;
  }
  
  if (!_serial || !_serial->isOpen() || _serial->errorStatus()) {
    json msg;
    msg["error"] = "couldn't open port";
    sendjson(msg);
    return;
  }
  
  this_thread::sleep_for(chrono::milliseconds(500));
  _serial->writeString("OFF\n");
  this_thread::sleep_for(chrono::milliseconds(100));
  
}

void Server::sendserial(const string &name, const string &data) {

  cout << "sending to " << name << endl;

  if (!_serial || !_serial->isOpen() || _serial->errorStatus()) {
    cout << "error while sending" << endl;
    json msg;
    msg["error"] = "couldnt send";
    sendjson(msg);
    return;
  }
  
  _serial->writeString(data + "\n");
  
  json msg;
  msg["sent"] = name;
  sendjson(msg);
  
}

boost::optional<string> Server::getstring(const json::iterator &json, const string &name) {

  json::iterator i = json->find(name);
  if (i == json->end()) {
    return boost::none;
  }
  string s = *i;
  return s;
  
}

boost::optional<int> Server::getint(const json::iterator &json, const string &name) {

  json::iterator i = json->find(name);
  if (i == json->end()) {
    return boost::none;
  }
  return (int)*i;
  
}

boost::optional<json::iterator> Server::get(json *json, const string &name) {

  json::iterator i = json->find(name);
  if (i == json->end()) {
    return boost::none;
  }
  return i;
  
}

void Server::getdevs(vector<string> *devs) {

  devs->clear();
  fs::directory_iterator end_iter;
  for (fs::directory_iterator i("/dev"); i != end_iter; i++) {
    string f = i->path().string();
    if (f.find("/dev/cu.usb") != string::npos) {
      devs->push_back(f);
    }
  }
  
}

void Server::opendevs(const vector<string> &devs) {

  if (devs.size() > 1) {
    std::cout << "can only handle 1 new device" << endl;
  }
  else if (devs.size() > 0) {
    connectserial("arduino", devs[0], 9600);        
    json msg;
    msg["added"] = "arduino";
    sendjson(msg);
  }
  
}

void Server::handladdremove() {

  vector<string> devs;
  getdevs(&devs);
  
  vector<string> added;
  set_difference(devs.begin(), devs.end(), _curdevs.begin(), _curdevs.end(),
    std::inserter(added, added.begin()));
  opendevs(added);
  
  vector<string> removed;
  set_difference(_curdevs.begin(), _curdevs.end(), devs.begin(), devs.end(),
    std::inserter(removed, removed.begin()));
    
  if (removed.size() > 1) {
    std::cout << "can only handle 1 new device" << endl;
  }
  else if (removed.size() > 0) {
    json msg;
    msg["removed"] = "arduino";
    sendjson(msg);
    if (_serial) {
      // no point closing, it's gone :-(
      delete _serial;
      _serial = 0;
    }
  }
  
  _curdevs = devs;
  
}

void Server::run() {
  
  getdevs(&_curdevs);
  opendevs(_curdevs);
  
  ptime start = second_clock::local_time();

  while (1) {

//    cout << "loop" << endl;
    
		zmq::message_t reply;
		if (_pull->recv(&reply, ZMQ_DONTWAIT)) {
		  string s((const char *)reply.data(), reply.size());
		  json json = json::parse(s);
		  {
        boost::optional<json::iterator> j = get(&json, "send");
        if (j) {
          boost::optional<string> name = getstring(*j, "name");
          boost::optional<string> data = getstring(*j, "data");
          if (!name || !data) {
            cout << "missing name or data" << endl;
            continue;
          }
          sendserial(*name, *data);
          continue;
        }
		  }
		}

    boost::this_thread::sleep_for(boost::chrono::milliseconds(40));
    
    if (_serial) {
      cout << _serial->readStringUntil("\n");
    }
	  
	  ptime cur = second_clock::local_time();
    time_duration diff = cur - start;
    if (diff.total_seconds() > 2) {
      handladdremove();
      start = cur;
    }
    
	}
	
}
