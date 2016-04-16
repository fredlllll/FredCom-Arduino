#ifndef FREDCOM_H
#define FREDCOM_H
#include <stdint.h>
#include <QueueRingBuffer.h>

template <uint16_t receiveBufferSize, uint16_t sendBufferSize> class FredCom {
public:
	void (*messageCallback)(uint8_t op, QueueRingBuffer<receiveBufferSize>* data);

	FredCom() {
		messageCallback = 0;

		in_cobsIsInMessage = false;
		in_lastCode = 0;
		in_nonZeroBytesRemaining = 0;

		out_lastCodeIndex = 0;
		out_transmissionID = 0;

		Serial.write(0);//make sure that pc gets fresh frame before we send stuff
	}

	void sendMessage(uint8_t op, uint8_t *data, uint16_t offset, uint8_t len) {
		sendBuffer.clear();
		sendBuffer.enqueue(1);
		out_lastCodeIndex = 0;

		uint8_t checksum = 127;

		COBSAppend(out_transmissionID);
		checksum *= out_transmissionID++;
		COBSAppend(len);
		checksum *= len;
		COBSAppend(op);
		checksum *= op;

		for (int i = 0; i < len; i++)
		{
			uint8_t b = data[i];
			COBSAppend(b);
			checksum *= b;
		}
		COBSAppend(checksum);

		sendBuffer.enqueue(0);
		while (sendBuffer.getContentLen() > 0) {
			Serial.write(sendBuffer.dequeue());
		}
	}

	void loop() {
		int bytes = Serial.available();
		for (int i = 0; i < bytes; i++) {
			uint8_t b = Serial.read();
			if (b > 0) {
				if (in_nonZeroBytesRemaining > 0) {
					receiveBuffer.enqueue(b);
					in_nonZeroBytesRemaining--;
				}
				else {
					in_nonZeroBytesRemaining = b - 1;
					if (in_lastCode != 0xFF && !in_cobsIsInMessage) { //first byte doesnt mean 0
						receiveBuffer.enqueue(0);
					}
					in_lastCode = b;
				}
				in_cobsIsInMessage = true;
			}
			else {
				if (in_cobsIsInMessage) { //end of frame
					uint8_t checksum = receiveBuffer.pop();

					uint8_t tid = receiveBuffer.dequeue();
					uint8_t len = receiveBuffer.dequeue();
					uint8_t op = receiveBuffer.dequeue();

					bool frameValid = true;

					if (frameValid && len != receiveBuffer.getContentLen()) {//check for correct length
						frameValid = false;
					}
					if (frameValid) {
						uint8_t testChecksum = 127;
						testChecksum *= tid;
						testChecksum *= len;
						testChecksum *= op;
						for (int i = 0; i < len; i++) {
							testChecksum *= receiveBuffer.peekBottom(i);
						}
						if (testChecksum != checksum) {
							frameValid = false;
						}
					}
					if (in_nonZeroBytesRemaining == 0 && frameValid) {
						if (messageCallback) {
							messageCallback(op, receiveBuffer);
						}
						sendACK(tid);
					}
					else {
						sendNACK(tid); //nack transmission id
					}//else error in transmission. ignore frame
					receiveBuffer.clear();
				}
				in_cobsIsInMessage = false;
			}
		}
	}
private:
	int in_nonZeroBytesRemaining;
	bool in_cobsIsInMessage;
	byte in_lastCode;

	uint8_t out_transmissionID;
	uint16_t out_lastCodeIndex;

	QueueRingBuffer<receiveBufferSize> receiveBuffer;
	QueueRingBuffer<sendBufferSize> sendBuffer;

	void COBSAppend(uint8_t val) {
		if (val == 0)
		{
			out_lastCodeIndex = sendBuffer.getContentLen();
			sendBuffer.enqueue(1);
		}
		else
		{
			sendBuffer.enqueue(val);
			sendBuffer.set(out_lastCodeIndex, sendBuffer.peekBottom(out_lastCodeIndex) + 1);
			if (sendBuffer.peekBottom(out_lastCodeIndex) == 0xFF)
			{
				out_lastCodeIndex = sendBuffer.getContentLen();
				sendBuffer.enqueue(1);
			}
		}
	}

	void sendACK(uint8_t id) {
		sendMessage(250, &id, 0, 1);
	}
	void sendNACK(uint8_t id) {
		sendMessage(251, &id, 0, 1);
	}
};
#endif // !FREDCOM_H