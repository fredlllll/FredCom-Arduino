********************************************************************************************
Arduino FredCom Library - Version 1.0.0
by Frederik Gelder <frederik.gelder@freenet.de>

Copyright 2016 Frederik Gelder

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************************************


Donation link:
https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=B566VJ7BJWQGL

 
I made this library to make communication with the computer easier, as i had many issues with garbage bytes that were coming out of nowhere.
This library uses COBS to encode the packages and a makeshift checksum to confirm validity of the package. each package also contains a len byte which is also used to validate the package.

a package consists of an opcode and its payload. opcodes >=250 are reserved.

for arduino:
create a variable "FredCom<200,100> comms" to use the lib. 200 stands for receiveBufferSize and 100 for sendBufferSize. both can be in the range of 0-65535 and roughly determine the max size of the messages you can receive and send. the object will have to fit into your controllers memory, so dont choose higher numbers than needed
at the moment it is hardcoded to use "Serial" for raw transmission. you have to initialize the serial interface yourself before you use the lib. call setup() on the object after that.
you can receive messages when you set the function pointer "messageCallback" of the FredCom instance. it will give you the opcode and a RingQueueBuffer when a message is received. use the dequeue() and peekBottom() methods to get to your payload data. dequeue removed the byte from the buffer, while peekBottom just returns it. peekBottom also takes an index, so you can access all bytes in the buffer. invalid indices will return 0. use getContentLen() to check for the length of your payload.
you have to call "loop" in your loop to receive messages.
you can send messages with "sendMessage"

see examples for more info

for pc:
create a FredCom variable. The Instance needs the portname and baudrate and will try to connect at instantiation.
use the receiveMessage event to receive messages. it will give you the opcode of the message and a List<byte> of the payloads bytes.
you dont have to call a loop() cause the instance creates its own thread for that.
use sendMessage to send a message to the controller.
