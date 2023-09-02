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

// so we don't hammer the CPU, we sleep a little while each loop.
#define SLEEP_TIME            20

using namespace std;
using json = nlohmann::json;
namespace fs = boost::filesystem;
using namespace boost::posix_time;

Server::~Server() {
  for (auto i : _connections) {
    i->close();
    delete i;
  }
  _connections.clear();
}

void Server::sendjson(const json &m) {

  string msg = m.dump();
  zmq::message_t zmsg(msg.length());
  memcpy(zmsg.data(), msg.c_str(), msg.length());
  _push->send(zmsg, ZMQ_DONTWAIT);

}

void Server::connect(const string &path, int baud) {

  cout << "connecting to " << path << endl;

  BufferedAsyncSerial *serial = 0;
  try {
    serial = new BufferedAsyncSerial(path, baud);
  }
  catch (boost::system::system_error& e) {
    cout << "open error: " << e.what() << endl;
    return;
  }
  
  if (!serial || !serial->isOpen() || serial->errorStatus()) {
    if (serial) {
      delete serial;
    }
    json msg;
    msg["error"] = "couldn't open port";
    sendjson(msg);
    return;
  }
  
  {
    json msg;
    msg["device"] = path;
    sendjson(msg);
  }
  
  // store it.
  _connections.push_back(new Connection(path, serial));
  
  // this is a terrible hack but for some reason the first send doesn't TAKE
  // so we need to wait a bit and send it again.
  // works just fine after that.
  this_thread::sleep_for(chrono::milliseconds(500));
  serial->writeString("ID\n");
  this_thread::sleep_for(chrono::milliseconds(100));
  serial->clear();
  this_thread::sleep_for(chrono::milliseconds(100));
  serial->writeString("ID\n");
  
}

Connection *Server::find(const std::string &name) {
  for (auto i : _connections) {
    if (i->matchid(name)) {
      return i;
    }
  }
  return 0;
}

Connection *Server::finddevice(const std::string &device) {
  for (auto i : _connections) {
    if (i->matchpath(device)) {
      return i;
    }
  }
  return 0;
}

void Server::sendserial(Connection *conn, const std::string &data) {

  cout << "sending to " << conn->_path << endl;

  if (!conn->isgood()) {
    cout << "error while sending" << endl;
    json msg;
    msg["error"] = "couldnt send";
    sendjson(msg);
    return;
  }
  
  conn->write(data);
  
  json msg;
  msg["sent"] = conn->_path;
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
    if (f.find("/dev/ttyUSB") != string::npos || f.find("/dev/ttyAC") != string::npos) {
  #endif
      devs->push_back(f);
    }
  }
  
}

void Server::opendevs(const vector<string> &devs) {

  for (auto i : devs) {
    connect(i, _baudrate);
  }

}

void Server::remove(const string &path) {
  for (vector<Connection *>::iterator i=_connections.begin(); i != _connections.end(); i++) {
    if ((*i)->matchpath(path)) {
      cout << "removed ";
      (*i)->describe(cout);
      cout << endl;
      json msg;
      msg["removed"] = path;
      sendjson(msg);
      (*i)->destroy();
      delete *i;
      _connections.erase(i);
      return;
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
    
  for (auto i: removed) {
    remove(i);
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
          for (auto i: _connections) {
            i->added(this);
          }
          continue;
        }
      }
      {
        // a client want's to send data.
        boost::optional<json::iterator> j = get(&doc, "send");
        if (j) {
          boost::optional<string> data = getstring(*j, "data");
          if (!data) {
            cout << "missing data" << endl;
            continue;
          }
          boost::optional<string> id = getstring(*j, "id");
          Connection *conn = 0;
          if (id) {
            conn = find(*id);
          }
          else {
            boost::optional<string> device = getstring(*j, "device");
            if (device) {
              conn = finddevice(*device);
            }
            else {
              if (_connections.size() > 0) {
                conn = _connections[0];
              }
              else {
                json msg;
                msg["error"] = "no id or device or no devices connected";
                sendjson(msg);
                continue;
              }
            }
          }
          if (!conn) {
            json msg;
            msg["error"] = "not connected ";
            sendjson(msg);
            continue;
          }          
          sendserial(conn, *data);
        }
      }
    }

    boost::this_thread::sleep_for(boost::chrono::milliseconds(SLEEP_TIME));

    for (auto i : _connections) {
      i->doread(this);
    }

    // every so often, check the device tree.
    ptime cur = microsec_clock::local_time();
    time_duration diff = cur - start;
    if (diff.total_milliseconds() > _cadence) {
      handladdremove();
      start = cur;
    }
    
  }

}
