/*
  zmqclient.hpp
  
  Author: Paul Hamilton (paul@visualops.com)
  Date: 21-Jun-2025
    
  A ZMQ Client for Arduino integration.
  
  This work is licensed under the Creative Commons Attribution 4.0 International License. 
  To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or 
  send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

  https://github.com/visualopsholdings/zmqarduino
*/

#ifndef H_zmqclient
#define H_zmqclient

#include <string>
#include <map>
#include <boost/json.hpp>
#include <zmq.hpp>

using namespace std;
using json = boost::json::value;

class Server;
class Channel;

typedef function<void (json *)> msgHandler;

class ZMQClient : public enable_shared_from_this<ZMQClient> {

public:
  ZMQClient(Server *server, int req);
  
  void run();
  void send(const string &userid, const string &streamid, const string &seqid, const string &text);

private:
  Server *_server;
  shared_ptr<zmq::context_t> _context;
  shared_ptr<zmq::socket_t> _req;
  map<string, msgHandler> _reqmessages;
  
  void receive();
  
  static bool getString(json *j, const string &name, string *value);
  static bool getBool(json *j, const string &name, bool *value);
  static void handle_reply(const zmq::message_t &reply, map<string, msgHandler> *handlers, const string &name);
  void send(const json &j);
  bool trySend(const string &m);
  
  // msg handlers
  void ackMsg(json *);
  void errMsg(json *);
  
};

#endif // H_zmqclient
