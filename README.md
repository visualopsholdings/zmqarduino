# ZMQArduino

A C++ service for using ZMQ to deal with arduinos.

This exposes an Arduino to a service that only has to PUSH and PULL using ZeroMQ. The idea 
is to use this service running on a raspberry PI (or any *nix*, even the Mac) and then use
it to send or receive any data from an arduino.

ZMQ is used is so that services can easily asynchronously receive
data coming from an Arduino which is difficult to code in Python, or JavaScript etc.

This service runs as a daemon on a computer and then another process connects
to it (even remotely) and find out the arduinos that are connected by name (in the sketch).

When an arduino is connected or disconnected it will get a message, so it can deal with that.

Eventually even programing the arduino will be supported because the idea is to use this to
run tools like AVR dude and any other tools that are supported in the Arduino IDE. The IDE
is written in TypeScript so there will be some work to reverse engineer it but it will be 
way better if it just needs a small service like this and binary tools.

That's the end goal. It's only VERY early days.

## Current functionality implemented:

### Detecting an arduino added to the /dev tree.

This just scans the /dev tree at the moment but works quite well. It scans the tree about once per second
for various things that might be arduinos. Works on Mac OS X and The PI.

The arduino needs to respond to the command "ID\n" over serial with a name which identifies it.

The "tinylogo" project has a very easy way to do this if you want to use it for your sketches. check
that out.

### Sending and receving data to/from an Arduino

Correctly implements a serial connection to an Arduino on Mac OS X and the PI.

## Usage

On your machine that you will connect with an arduino:

```
$ ./ZMQArduino
```

Then just create a zmq PULL socket on port 5559, and a push port on port 5558 and you can Send JSON
commands and receive things from the service.

## API

### sent

#### Tell the service you are connected

```
{ 
  connected: "me"
}
```

The service will send back any "added" arduinos.

#### Send data to an arduino

```
{ 
  send: { 
    name: "arduino", 
    data: "FLASH" 
  } 
}
```

Send "FLASH" to the arduino called "arduino"

### recieved

#### New arduino added

```
{ added: "arduino" }
```
  
An arduino called "arduino" was added to the machine.

#### Arduino removed

```
{ removed: "arduino" }
```
  
An arduino called "arduino" was removed from the machine.

#### error

```
{ error: "foo" }
```
  
A "foo" error happened.

#### Data sent

```
{ sent: "arduino" }
```
  
Data was sent to the Arduino "arduino" 

#### Data received

```
{ 
  received: { 
    name: "arduino", 
    data: "FLASH" 
  } 
}
```
  
Data was received from the Arduino "arduino".

## Development

The development process for all of this code used a normal Linux environment with the BOOST
libraries and a C++ compiler.

So on your Linux (or mac using Homebrew etc), get all you need:

```
$ sudo apt-get -y install g++ gcc make cmake libboost-all-dev
```

The build the prequesites we need:

```
git clone https://github.com/zeromq/libzmq.git
cd libzmq
mkdir build
cd build
cmake ..
make
sudo make install
cd ../..

git clone https://github.com/zeromq/cppzmq
cd cppzmq
mkdir build
cd build
cmake ..
make
sudo make install
cd ../..

git clone https://github.com/nlohmann/json
cd json
mkdir build
cd build
cmake ..
make
sudo make install
cd ../..
```

And then inside the "zmqarduino" folder on your machine, run:

```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

Then to run the command:

```
$ ./ZMQArduino
```

For an M1 or M2 mac, use this command for cmake for everything.

```
$ cmake -DCMAKE_OSX_ARCHITECTURES="arm64" ..
```

## Current development focus

### Remote detection of ESP32's connected through Wifi (they work over serial).

## Credits

### Terraneo Federico

Thank you so much for the serial classes! This was a major pain for me to work out and
these classes (with a few tweeks) worked Perfectly!

### Zero MQ

The greatest way to communicate between machines:

https://zeromq.org/

### Boost and the C++ community

https://www.boost.org/
https://isocpp.org/

## Change Log

### 19 Jun 2023
- Initial checkin, and then added the concept of the "ID" of the arduino.

### 21 Jun 2023
- Added "received".

### 26 Jun 2023
- Added support for multiple arduinos.
