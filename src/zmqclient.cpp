/*
  zmqclient.cpp
  
  Author: Paul Hamilton (paul@visualops.com)
  Date: 21-Jun-2025
    
  This work is licensed under the Creative Commons Attribution 4.0 International License. 
  To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or 
  send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

  https://github.com/visualopsholdings/zmqarduino
*/

#include "zmqclient.hpp"

#include <boost/log/trivial.hpp>
#include <thread>

ZMQClient::ZMQClient(Server *server, int reqPort) : 
  _server(server) {

  _context.reset(new zmq::context_t(1));

  _req.reset(new zmq::socket_t(*_context, ZMQ_REQ));
  _req->connect("tcp://127.0.0.1:" + to_string(reqPort));
	BOOST_LOG_TRIVIAL(info) << "Connect to ZMQ as Local REQ on " << reqPort;
  
  // expect these as replies
  _reqmessages["ack"] = bind( &ZMQClient::ackMsg, this, placeholders::_1 );
  _reqmessages["err"] = bind( &ZMQClient::errMsg, this, placeholders::_1 );
  
}

void ZMQClient::run() {

  thread t(bind(&ZMQClient::receive, this));
  t.detach();
  
}

void ZMQClient::receive() {

  BOOST_LOG_TRIVIAL(trace) << "start receiving";

  zmq::pollitem_t items [] = {
      { *_req, 0, ZMQ_POLLIN, 0 }
  };
  const std::chrono::milliseconds timeout{500};
  while (1) {
    
//    BOOST_LOG_TRIVIAL(debug) << "polling for messages";
    zmq::message_t message;
    zmq::poll(items, 1, timeout);

//     BOOST_LOG_TRIVIAL(trace) << "sub events " << items[0].revents;
//     BOOST_LOG_TRIVIAL(trace) << "req events " << items[1].revents;

    if (items[0].revents & ZMQ_POLLIN) {
      BOOST_LOG_TRIVIAL(trace) << "got _req message";
      zmq::message_t reply;
      try {
#if CPPZMQ_VERSION == ZMQ_MAKE_VERSION(4, 3, 1)
        _req->recv(&reply);
#else
        auto res = _req->recv(reply, zmq::recv_flags::none);
#endif
        handle_reply(reply, &_reqmessages, "<-");
      }
      catch (zmq::error_t &e) {
        BOOST_LOG_TRIVIAL(warning) << "got exc with _req recv" << e.what() << "(" << e.num() << ")";
      }
    }
  }

}

void ZMQClient::handle_reply(const zmq::message_t &reply, map<string, msgHandler> *handlers, const string &name) {

  BOOST_LOG_TRIVIAL(trace) << "handling reply";

  // convert to JSON
  string r((const char *)reply.data(), reply.size());
  json doc = boost::json::parse(r);

  BOOST_LOG_TRIVIAL(debug) << name << " " << doc;

  // switch the handler based on the message type.
  string type;
  if (!getString(&doc, "type", &type)) {
    BOOST_LOG_TRIVIAL(error) << "no type";
    return;
  }

  map<string, msgHandler>::iterator handler = handlers->find(type);
  if (handler == handlers->end()) {
    BOOST_LOG_TRIVIAL(error) << "unknown reply type " << type;
    return;
  }
  handler->second(&doc);
  
}

bool ZMQClient::trySend(const string &m) {

  BOOST_LOG_TRIVIAL(trace) << "try sending " << m;

	zmq::message_t msg(m.length());
	memcpy(msg.data(), m.c_str(), m.length());
  try {
#if CPPZMQ_VERSION == ZMQ_MAKE_VERSION(4, 3, 1)
    _req->send(msg);
#else
    _req->send(msg, zmq::send_flags::none);
#endif
    return true;
  }
  catch (...) {
    return false;
  }

}

void ZMQClient::send(const json &j) {

  stringstream ss;
  ss << j;
  string m = ss.str();
  
  if (!trySend(m)) {
    for (int i=0; i<4; i++) {
	    this_thread::sleep_for(chrono::milliseconds(20));
      BOOST_LOG_TRIVIAL(trace) << "retrying send";
	    if (trySend(m)) {
	      return;
	    }
    }
  }

}

void ZMQClient::send(const string &userid, const string &streamid, const string &seqid, const string &text) {

	send({ 
	  { "type", "addobject" }, 
	  { "objtype", "idea" },
	  { "me", userid },
    { "stream", streamid },
	  { "text", text },
    { "sequence", seqid }
	});

}

bool ZMQClient::getString(json *j, const string &name, string *value) {

  try {
    *value = boost::json::value_to<string>(j->at(name));
    return true;
  }
  catch (const boost::system::system_error& ex) {
    return false;
  }

}

bool ZMQClient::getBool(json *j, const string &name, bool *value) {

  try {
    *value = boost::json::value_to<bool>(j->at(name));
    return true;
  }
  catch (const boost::system::system_error& ex) {
    return false;
  }

}

void ZMQClient::ackMsg(json *doc) {

  BOOST_LOG_TRIVIAL(info) << "acknowleged";

}

void ZMQClient::errMsg(json *doc) {

  BOOST_LOG_TRIVIAL(error) << "err" << *doc;

}
