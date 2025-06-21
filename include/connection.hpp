/*
  connection.hpp
  
  Author: Paul Hamilton (paul@visualops.com)
  Date: 26-Jun-2023
    
  A single connection to an arduino
  
  This work is licensed under the Creative Commons Attribution 4.0 International License. 
  To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or 
  send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

  https://github.com/visualopsholdings/zmqarduino
*/

#ifndef H_connection
#define H_connection

#include <string>
#include <boost/optional.hpp>

class BufferedAsyncSerial;
class Server;

class Connection {

public:
  Connection(const std::string &path, BufferedAsyncSerial *serial): _path(path), _serial(serial), _waitingid(true) {}
  
  void close();
  void destroy();
  bool matchid(const std::string &id);
  bool matchpath(const std::string &path);
  bool isgood();
  void write(const std::string &data);
  void doread(Server *server);
  void added(Server *server);
  void sendid(Server *server);
  void describe(std::ostream &str);
  
  std::string _path;
  std::string _stream;
  std::string _user;
  std::string _sequence;

private:
 
  BufferedAsyncSerial *_serial;
  boost::optional<std::string> _id;
  bool _waitingid;
};

#endif // H_connection
