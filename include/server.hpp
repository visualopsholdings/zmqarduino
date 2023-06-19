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

#include <nlohmann/json.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/optional.hpp>
#include <zmq.hpp>

class BufferedAsyncSerial;

class Server {

public:
  Server(zmq::socket_t *pull, zmq::socket_t *push) : _pull(pull), _push(push), _serial(0), _waitingid(false) {}
  ~Server();
  
  void run();
  
private:
  zmq::socket_t *_pull;
  zmq::socket_t *_push;
  BufferedAsyncSerial *_serial;
  std::vector<std::string> _curdevs;
  bool _waitingid;
  boost::optional<std::string> _id;
  
  void close();
  void connectserial(const std::string &path, int baud);
  void sendserial(const std::string &name, const std::string &data);
  void sendjson(const nlohmann::json &m);
  
  boost::optional<std::string> getstring(const nlohmann::json::iterator &json, const std::string &name);
  boost::optional<int> getint(const nlohmann::json::iterator &json, const std::string &name);
  boost::optional<nlohmann::json::iterator> get(nlohmann::json *json, const std::string &name);
  void getdevs(std::vector<std::string> *devs);
  void opendevs(const std::vector<std::string> &devs);
  void handladdremove();
  void doread();
  
};

#endif // H_server
