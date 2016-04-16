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
you can receive messages when you set the function pointer "messageCallback" of the FredCom instance
you should call "loop" in your loop to receive messages.
you can send messages with "sendMessage"

for pc:
TODO
