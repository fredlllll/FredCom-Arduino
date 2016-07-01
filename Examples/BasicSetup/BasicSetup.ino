#include <FredCom.h>

FredCom<100, 100> comms;

void messageReceive(uint8_t op, QueueRingBuffer<100>* buffer);//function prototype

void setup() {
  Serial.begin(115200);
  comms.messageCallback = messageReceive;
  comms.setup();
}

void loop() {
  comms.loop();
}

void messageReceive(uint8_t op, QueueRingBuffer<100>* buffer) {
  switch (op) {
    case 1:
      { //brackets cause compiler would nag without them
        //handle opcode 1 here
        uint8_t reply[5] = {
          1,
          2,
          3,
          4,
          5
        };
        comms.sendMessage(1, reply, 0, 5);
      }
      break;
    default:
      break;
  }
}
