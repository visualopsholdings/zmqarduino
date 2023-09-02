/*
  server.hpp
  
  Author: Paul Hamilton (paul@visualops.com)
  Date: 18-Jun-2023
    
  The main server for ZMQ to Arduino integration.
  
  This work is licensed under the Creative Commons Attribution 4.0 International License. 
  To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or 
  send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

  https://github.com/visualopsholdings/zmqarduino
*/

#ifndef H_server
#define H_server

#include "connection.hpp"

#include <nlohmann/json.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/optional.hpp>
#include <zmq.hpp>

class Server {

public:
  Server(zmq::socket_t *pull, zmq::socket_t *push, int cadence, int baudrate) : 
    _pull(pull), _push(push), _cadence(cadence), _baudrate(baudrate) {}
  ~Server();
  
  void run();
  void sendjson(const nlohmann::json &m);
  
private:
  zmq::socket_t *_pull;
  zmq::socket_t *_push;
  std::vector<Connection *> _connections;
  std::vector<std::string> _curdevs;
  int _cadence;
  int _baudrate;
  
  void connect(const std::string &path, int baud);
  void sendserial(Connection *conn, const std::string &data);
  Connection *find(const std::string &name);
  Connection *finddevice(const std::string &device);
  
  boost::optional<std::string> getstring(const nlohmann::json::iterator &json, const std::string &name);
  boost::optional<int> getint(const nlohmann::json::iterator &json, const std::string &name);
  boost::optional<nlohmann::json::iterator> get(nlohmann::json *json, const std::string &name);
  void getdevs(std::vector<std::string> *devs);
  void opendevs(const std::vector<std::string> &devs);
  void handladdremove();
  void remove(const std::string &path);
  bool anyneedsid();
  
};

#endif // H_server
