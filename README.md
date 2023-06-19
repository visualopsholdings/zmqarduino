# ZMQArduino

A C++ service for using ZMQ to deal with arduinos.

This exposes an Arduino to a service that only has to PUSH and PULL using ZeroMQ.

## Current functionality implemented:

### Detecting an arduino added to the /dev tree.

Extremely rudimentary, only works for Mac OS X and doesn't really probe the Arduino at all.

There can only be 1 arduino and for now it's called "arduino".

### sending data to an Arduino

Correctly implements a serial connection to an Arduino on Mac OS X.

## Usage

On your machine that you will connect with an arduino:

```
$ ./ZMQArduino
```

Then just create a zmq PULL socket on port 5559, and a push port on port 5558 and you can Send JSON
commands and receive things from the service.

## API

### sent

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

## Development

The development process for all of this code used a normal Linux environment with the BOOST
libraries and a C++ compiler.

So on your Linux (or mac using Homebrew etc), get all you need:

```
$ sudo apt-get -y install g++ gcc make cmake boost zmq
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

For an M1 or M2 mac, use this command for cmake

```
$ cmake -DCMAKE_OSX_ARCHITECTURES="arm64" ..
```

## Current development focus

### Getting it to work on a Raspberry PI.

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

