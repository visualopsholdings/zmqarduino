/*
  connection.cpp
  
  Author: Paul Hamilton (paul@visualops.com)
  Date: 26-Jun-2023
    
  This work is licensed under the Creative Commons Attribution 4.0 International License. 
  To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or 
  send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

  https://github.com/visualopsholdings/zmqarduino
*/

#include "connection.hpp"

#include "server.hpp"
#include "BufferedAsyncSerial.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <boost/algorithm/string.hpp>

using namespace std;
using json = nlohmann::json;

void Connection::describe(ostream &str) {
  str << (_id ? *_id : "no id");
  str << " (" << _path << ")";
  if (_serial) {
    str << (_serial->isOpen() ? ", opened" : ", unopened");
  }
  if (_serial->errorStatus()) {
      str << ", has error";
  }
}

void Connection::added(Server *server) {

  json msg1;
  msg1["device"] = _path;
  server->sendjson(msg1);
  
  if (_id) {
    json data;
    data["device"] = _path;
    data["name"] = *_id;
    json msg2;
    msg2["id"] = data;
    server->sendjson(msg2);
  }

}

void Connection::doread(Server *server) {

  if (_serial) {
    stringstream s;
    s << _serial->readStringUntil("\n");
    string st = s.str();
    if (st.length() > 0) {
      if (_waitingid) {
        _waitingid = false;
        boost::trim(st);
        _id = st;
        json data;
        data["device"] = _path;
        data["name"] = *_id;
        json msg;
        msg["id"] = data;
        server->sendjson(msg);
        cout << "added ";
        describe(cout);
        cout << endl;
      }
      else {
        json data;
        data["device"] = _path;
        boost::trim(st);
        data["data"] = st;
        json msg;
        msg["received"] = data;
        server->sendjson(msg);
        cout << s.str();
        cout.flush();
      }
    }
  }
  
}

void Connection::close() {
  _serial->close();
  destroy();
}
void Connection::destroy() {
  if (_serial) {
    delete _serial;
  }
  _serial = 0;
}

bool Connection::matchid(const string &id) {
  return _id && *_id == id;
}
bool Connection::matchpath(const string &path) {
  return _path == path;
}

bool Connection::isgood() {
  return _serial && _serial->isOpen() && !_serial->errorStatus();
}

void Connection::write(const string &data) {
  _serial->writeString(data + "\n");
}

