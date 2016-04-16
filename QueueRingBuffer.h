#ifndef QUEUERINGBUFFER_H
#define QUEUERINGBUFFER_H
#include <stdint.h>

template <uint16_t size> class QueueRingBuffer
{
public:
	QueueRingBuffer() {
		len = 0;
		offset = 0;
	}

	uint16_t getContentLen() {
		return len;
	}

	void enqueue(uint8_t val) {
		uint16_t index = (offset + len) % size;
		buffer[index] = val;
		if (len < size) {
			len++;
		}
		else { //overwrite old data
			offset = (offset + 1) % size;
		}
	}

	uint8_t dequeue() {
		if (len > 0) {
			uint8_t retval = buffer[offset];
			offset = (offset + 1) % size;
			len--;
			return retval;
		}
		return 0;
	}

	void clear() {
		offset = 0;
		len = 0;
	}

	void push(uint8_t val) {
		enqueue(val);
	}

	uint8_t pop() {
		if (len > 0) {
			uint8_t retval = buffer[(offset + len - 1) % size];
			len--;
			return retval;
		}
		return 0;
	}

	uint8_t peekBottom(uint16_t peekOffset = 0) {
		if (peekOffset < len) {
			uint16_t index = (offset + peekOffset) % size;
			return buffer[index];
		}
		return 0;
	}

	uint8_t peekTop(uint16_t peekOffset = 0) {
		if (peekOffset < len) {
			uint16_t index = (offset + len - peekOffset) % size;
			return buffer[index];
		}
		return 0;
	}

	void set(uint16_t index, uint8_t value) {
		index = (offset + index) % size;
		buffer[index] = value;
	}

	uint16_t getSize() {
		return size;
	}

private:
	uint16_t offset;
	uint16_t len;
	uint8_t buffer[size];
};
#endif // !QUEUERINGBUFFER_H