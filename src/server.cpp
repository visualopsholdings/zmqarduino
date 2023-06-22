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
#include <boost/algorithm/string.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>

// how often we check the device tree in seconds.
#define DEVICE_CHECK_CADENCE  2

// this is hard coded, we need to work out how we might probe for this.
#define BAUD_RATE             9600

// so we don't hammer the CPU, we sleep a little while each loop.
#define SLEEP_TIME            20

using namespace std;
using json = nlohmann::json;
namespace fs = boost::filesystem;
using namespace boost::posix_time;

Server::~Server() {
  close();
}

void Server::close() {
  if (_serial) {
    _serial->close();
    delete _serial;
    _serial = 0;
  }
}

void Server::sendjson(const json &m) {

  string msg = m.dump();
  zmq::message_t zmsg(msg.length());
  memcpy(zmsg.data(), msg.c_str(), msg.length());
  _push->send(zmsg, ZMQ_DONTWAIT);

}

void Server::connectserial(const string &path, int baud) {

  cout << "connecting to " << path << endl;

  close();
  
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
  
  // this is a terrible hack but for some reason the first send doesn't TAKE
  // so we need to wait a bit and send it again.
  // works just fine after that.
  this_thread::sleep_for(chrono::milliseconds(500));
  _serial->writeString("ID\n");
  this_thread::sleep_for(chrono::milliseconds(100));
  _serial->clear();
  this_thread::sleep_for(chrono::milliseconds(100));
  _serial->writeString("ID\n");
  _waitingid = true;
  
}

void Server::sendserial(const string &name, const string &data) {

  cout << "sending to " << name << endl;

  if (_id) {
    if (*_id != name) {
      cout << "cant send to " << name << endl;
      json msg;
      msg["error"] = "don't know " + name;
      sendjson(msg);
      return;
    }
  }
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
  #ifdef __APPLE__
    if (f.find("/dev/cu.usb") != string::npos) {
  #else
    if (f.find("/dev/ttyUSB") != string::npos) {
  #endif
      devs->push_back(f);
    }
  }
  
}

void Server::opendevs(const vector<string> &devs) {

  if (devs.size() > 1) {
    std::cout << "can only handle 1 new device" << endl;
  }
  else if (devs.size() > 0) {
    connectserial(devs[0], BAUD_RATE);
  }

}

void Server::doread() {

  if (_serial) {
    stringstream s;
    s << _serial->readStringUntil("\n");
    string st = s.str();
    if (st.length() > 0) {
      cout << "got " << st << endl;
      if (_waitingid) {
        _waitingid = false;
        boost::trim(st);
        _id = st;
        json msg;
        msg["added"] = *_id;
        sendjson(msg);
      }
      else if (_id) {
        json msg;
        json data;
        data["name"] = *_id;
        boost::trim(st);
        data["data"] = st;
        msg["received"] = data;
        sendjson(msg);
        cout << s.str();
        cout.flush();
      }
    }
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
    if (_id) {
      json msg;
      msg["removed"] = *_id;
      sendjson(msg);
      _id = boost::none;
    }
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

//    cout << "." << endl;
      
    zmq::message_t reply;
    if (_pull->recv(&reply, ZMQ_DONTWAIT)) {
      string s((const char *)reply.data(), reply.size());
      json doc = json::parse(s);
      {
        // a client has connected.
        boost::optional<json::iterator> connected = get(&doc, "connected");
        if (connected) {
          string name = **connected;
          cout << name << " connected" << endl;
          if (_id) {
            json msg;
            msg["added"] = *_id;
            sendjson(msg);
          }
          else {
          }
          continue;
        }
      }
      {
        // a client want's to send data.
        boost::optional<json::iterator> j = get(&doc, "send");
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

    boost::this_thread::sleep_for(boost::chrono::milliseconds(SLEEP_TIME));

    doread();

    // every so often, check the device tree.
    ptime cur = second_clock::local_time();
    time_duration diff = cur - start;
    if (diff.total_seconds() > DEVICE_CHECK_CADENCE) {
      handladdremove();
      start = cur;
    }
    
  }

}
