#ifndef FREDCOM_H
#define FREDCOM_H
#include <stdint.h>
#include "QueueRingBuffer.h"
#include <Arduino.h>

//#define USEFLOAT32
//#define USEFLOAT64

enum varType {
    var_uint8_t = 1,
    var_uint16_t = 2,
    var_uint32_t = 3,
    var_uint64_t = 4,
    var_int8_t = 5,
    var_int16_t = 6,
    var_int32_t = 7,
    var_int64_t = 8,
    var_float32_t = 9,
    var_float64_t = 10,
};

struct varSendPackage {
    varType type;
    union
    {
        uint8_t var_uint8_t;
        uint16_t var_uint16_t;
        uint32_t var_uint32_t;
        uint64_t var_uint64_t;
        int8_t var_int8_t;
        int16_t var_int16_t;
        int32_t var_int32_t;
        int64_t var_int64_t;
#ifdef USEFLOAT32
        float32_t var_float32_t;
#else
        float var_float32_t;
#endif // USEFLOAT32
#ifdef USEFLOAT64
        float64_t var_float64_t;
#else
        double var_float64_t;
#endif // USEFLOAT64
    };
};

template <uint16_t receiveBufferSize, uint16_t sendBufferSize> class FredCom {
public:
    void(*messageCallback)(uint8_t op, QueueRingBuffer<receiveBufferSize>* data);
    void(*uint8Callback)(uint8_t val);
    void(*uint16Callback)(uint16_t val);
    void(*uint32Callback)(uint32_t val);
    void(*uint64Callback)(uint64_t val);
    void(*int8Callback)(int8_t val);
    void(*int16Callback)(int16_t val);
    void(*int32Callback)(int32_t val);
    void(*int64Callback)(int64_t val);
#ifdef USEFLOAT32
    void(*float32Callback)(float32_t val);
#else
    void(*float32Callback)(float val);
#endif
#ifdef USEFLOAT64
    void(*float64Callback)(float64_t val);
#else
    void(*float64Callback)(double val);
#endif

    FredCom() {
        messageCallback = 0;

        in_cobsIsInMessage = false;
        in_lastCode = 0;
        in_nonZeroBytesRemaining = 0;

        out_lastCodeIndex = 0;
        out_transmissionID = 0;
    }

    void setup() {
        Serial.write((const uint8_t)0);//make sure that pc gets fresh frame before we send stuff
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

        for (int i = offset; i < len + offset; i++)
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
                    if (in_lastCode != 0xFF && in_cobsIsInMessage) { //first byte doesnt mean 0
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
                    uint8_t nackReason = 0;
                    uint8_t r1, r2;

                    if (frameValid && len != receiveBuffer.getContentLen()) {//check for correct length
                        frameValid = false;
                        r1 = len;
                        r2 = receiveBuffer.getContentLen();
                        nackReason = 1;
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
                            nackReason = 2;
                        }
                    }
                    if (in_nonZeroBytesRemaining != 0) {
                        frameValid = false;
                        nackReason = 3;
                    }
                    if (frameValid) {
                        switch (op)
                        {
                        case 250:
                            break;
                        case 251:
                            break;
                        case 252:
                            break;
                        case 253:
                            break;
                        case 254:
                            break;
                        case 255:
                        {
                            varSendPackage *pckg = (varSendPackage*)receiveBuffer.getBuffer();
                            switch (pckg->type)
                            {
                            case varType::var_uint8_t:
                                if (uint8Callback) {
                                    uint8Callback(pckg->var_uint8_t);
                                }
                                break;
                            case varType::var_uint16_t:
                                if (uint16Callback) {
                                    uint16Callback(pckg->var_uint16_t);
                                }
                                break;
                            case varType::var_uint32_t:
                                if (uint32Callback) {
                                    uint32Callback(pckg->var_uint32_t);
                                }
                                break;
                            case varType::var_uint64_t:
                                if (uint64Callback) {
                                    uint64Callback(pckg->var_uint64_t);
                                }
                                break;
                            case varType::var_int8_t:
                                if (int8Callback) {
                                    int8Callback(pckg->var_int8_t);
                                }
                                break;
                            case varType::var_int16_t:
                                if (int16Callback) {
                                    int16Callback(pckg->var_int16_t);
                                }
                                break;
                            case varType::var_int32_t:
                                if (int32Callback) {
                                    int32Callback(pckg->var_int32_t);
                                }
                                break;
                            case varType::var_int64_t:
                                if (int64Callback) {
                                    int64Callback(pckg->var_int64_t);
                                }
                                break;
                            case varType::var_float32_t:
                                if (float32Callback) {
                                    float32Callback(pckg->var_float32_t);
                                }
                                break;
                            case varType::var_float64_t:
                                if (float64Callback) {
                                    float64Callback(pckg->var_float64_t);
                                }
                                break;
                            }
                        }
                        break;
                        default:
                            if (messageCallback) {
                                messageCallback(op, &receiveBuffer);
                            }
                            break;
                        }
                        sendACK(tid);
                    }
                    else {
                        sendNACK(tid, nackReason, r1, r2); //nack transmission id
                    }//else error in transmission. ignore frame
                    receiveBuffer.clear();
                }
                in_cobsIsInMessage = false;
            }
        }
    }

    void send(uint8_t val) {
        varSendPackage pack;
        pack.type = varType::var_uint8_t;
        pack.var_uint8_t = val;
        sendMessage(255, (uint8_t*)&pack, 0, sizeof(pack));
    }

    void send(uint16_t val) {
        varSendPackage pack;
        pack.type = varType::var_uint16_t;
        pack.var_uint16_t = val;
        sendMessage(255, (uint8_t*)&pack, 0, sizeof(pack));
    }

    void send(uint32_t val) {
        varSendPackage pack;
        pack.type = varType::var_uint32_t;
        pack.var_uint32_t = val;
        sendMessage(255, (uint8_t*)&pack, 0, sizeof(pack));
    }

    void send(uint64_t val) {
        varSendPackage pack;
        pack.type = varType::var_uint64_t;
        pack.var_uint64_t = val;
        sendMessage(255, (uint8_t*)&pack, 0, sizeof(pack));
    }

    void send(int8_t val) {
        varSendPackage pack;
        pack.type = varType::var_int8_t;
        pack.var_int8_t = val;
        sendMessage(255, (uint8_t*)&pack, 0, sizeof(pack));
    }

    void send(int16_t val) {
        varSendPackage pack;
        pack.type = varType::var_int16_t;
        pack.var_int16_t = val;
        sendMessage(255, (uint8_t*)&pack, 0, sizeof(pack));
    }

    void send(int32_t val) {
        varSendPackage pack;
        pack.type = varType::var_int32_t;
        pack.var_int32_t = val;
        sendMessage(255, (uint8_t*)&pack, 0, sizeof(pack));
    }

    void send(int64_t val) {
        varSendPackage pack;
        pack.type = varType::var_int64_t;
        pack.var_int64_t = val;
        sendMessage(255, (uint8_t*)&pack, 0, sizeof(pack));
    }

#ifdef USEFLOAT32
    void send(float32_t val) {
#else
    void send(float val) {
#endif // USEFLOAT32
        varSendPackage pack;
        pack.type = varType::var_float32_t;
        pack.var_float32_t = val;
        sendMessage(255, (uint8_t*)&pack, 0, sizeof(pack));
    }

#ifdef USEFLOAT64
    void send(float64_t val) {
#else
    void send(double val) {
#endif // USEFLOAT64
        varSendPackage pack;
        pack.type = varType::var_float64_t;
        pack.var_float64_t = val;
        sendMessage(255, (uint8_t*)&pack, 0, sizeof(pack));
    }
private:
    int in_nonZeroBytesRemaining;
    bool in_cobsIsInMessage;
    uint8_t in_lastCode;

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
    void sendNACK(uint8_t id, uint8_t reason, uint8_t r1, uint8_t r2) {
        uint8_t data[4] = {
            id,
            reason,
            r1,
            r2
        };
        sendMessage(251, data, 0, 4);
    }
    };
#endif // !FREDCOM_H