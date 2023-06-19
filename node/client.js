/*
  client.js
  
  Author: Paul Hamilton (paul@visualops.com)
  Date: 18-Jun-2023
    
  Simple client written in NodeJS.
  
  This work is licensed under the Creative Commons Attribution 4.0 International License. 
  To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or 
  send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

  https://github.com/visualopsholdings/zmqarduino
*/

var zmq = require('zeromq');

var pullPort = 5559;
var pushPort = 5558;

var pullSocket = zmq.socket('pull');
pullSocket.connect("tcp://127.0.0.1:" + pullPort);
console.log("Connect to ZMQ as PULL on", pullPort);

var pushSocket = zmq.socket('push');
pushSocket.connect("tcp://127.0.0.1:" + pushPort);
console.log("Connect to ZMQ as PUSH on", pushPort);

pullSocket.on('message', (msg) => {
  let json = JSON.parse(msg);
  if (json.sent) {
    console.log("sent");
  }
  else if (json.added) {
    console.log("added", json.added);
    pushSocket.send(JSON.stringify({ send: { name: "arduino", data: "FLASH" } }));
  }
  else if (json.removed) {
    console.log("removed", json.removed);
  }
  else if (json.error) {
    console.log("error", json.error);
  }
});
