/*
 * SimpleReceiverForHashCodes.cpp
 *
 * Demonstrates receiving hash codes of unknown protocols with IRremote
 *
 *  This file is part of Arduino-IRremote https://github.com/Arduino-IRremote/Arduino-IRremote.
 *
 ************************************************************************************
 * MIT License
 *
 * Copyright (c) 2024 Armin Joachimsmeyer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************************
 */
#define RECORD_GAP_MICROS 12000
#include <Arduino.h>

/*
 * Specify which protocol(s) should be used for decoding.
 * This must be done before the #include <IRremote.hpp>
 */
#define DECODE_LG
#define DECODE_HASH             // special decoder for all protocols
#define DECODE_NEC
#define DEBUG
#define RAW_BUFFER_LENGTH  1000 // Especially useful for unknown and probably long protocols
//#define DEBUG                 // Activate this for lots of lovely debug output from the decoders.

/*
 * This include defines the actual pin number for pins like IR_RECEIVE_PIN, IR_SEND_PIN for many different boards and architectures
 */
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp> // include the library
#include <ir_LG.hpp>
#include <IRremoteInt.h>

#define LG_KHZ          38

#define LG_ADDRESS_BITS          8
#define LG_COMMAND_BITS         16
#define LG_CHECKSUM_BITS         4
#define LG_BITS                 (LG_ADDRESS_BITS + LG_COMMAND_BITS + LG_CHECKSUM_BITS) // 28

#define LG_UNIT                 500 // 19 periods of 38 kHz

#define LG3_HEADER_MARK       3100 
#define LG3_HEADER_SPACE       9700

#define LG_BIT_MARK             LG_UNIT
#define LG_ONE_SPACE             550
#define LG_ZERO_SPACE           1550

struct PulseDistanceWidthProtocolConstants LG3ProtocolConstants = { 0, LG_KHZ, LG3_HEADER_MARK, LG3_HEADER_SPACE, LG_BIT_MARK,
LG_ONE_SPACE, LG_BIT_MARK, LG_ZERO_SPACE, PROTOCOL_IS_MSB_FIRST, (LG_REPEAT_PERIOD / MICROS_IN_ONE_MILLI), &sendNECSpecialRepeat };
// computeLGRawDataAndChecksum(aAddress, aCommand)
void sendLG3() {
    IrSender.sendPulseDistanceWidth(&LG3ProtocolConstants, 0b0111011111110111110011100011, LG_BITS, 0);
}

uint32_t computeLG3RawDataAndChecksum(uint8_t aAddress, uint16_t aCommand) {
    uint32_t tRawData = ((uint32_t) aAddress << (LG_COMMAND_BITS + LG_CHECKSUM_BITS)) | ((uint32_t) aCommand << LG_CHECKSUM_BITS);
    // TODO guess checksum logic
    uint8_t tChecksum = 0;
    uint32_t tTempForChecksum = (uint32_t) (tRawData >> LG_CHECKSUM_BITS);
    Serial.println((uint32_t) 1 << 30, BIN);
    Serial.println((uint32_t) aAddress, BIN);
    Serial.println("address");
    Serial.println((uint32_t) aAddress << (LG_COMMAND_BITS + LG_CHECKSUM_BITS), BIN);
    Serial.println("command");
    Serial.println((uint32_t) aCommand << LG_CHECKSUM_BITS, BIN);
    Serial.println("tRawData");
    Serial.println(tRawData, BIN);
    for (int i = 0; i < 6; ++i) {
        Serial.println(tTempForChecksum, BIN);
        tChecksum += tTempForChecksum & 0xF; // add low nibble

        tTempForChecksum >>= 4; // shift by a nibble
    }
    Serial.println("tChecksum");
    Serial.println(tChecksum, BIN);
    return (tRawData | ((~tChecksum) & 0xF));
}

void setup() {
    Serial.begin(115200);
    while (!Serial)
        ; // Wait for Serial to become available. Is optimized away for some cores.

    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));

    // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

    Serial.println(F("Ready to receive unknown IR signals at pin " STR(IR_RECEIVE_PIN) " and decode it with hash decoder."));
    Serial.println(computeLG3RawDataAndChecksum(0b01110111, 0b1111011111001110), BIN); // checksum is 0011 - 18'
    Serial.println();
    // original checksum logic does not work
    Serial.println(IrSender.computeLGRawDataAndChecksum(0b01110111, 0b1111011111001110), BIN);
    Serial.println();
    Serial.println(computeLG3RawDataAndChecksum(0b01110111, 0b1111011110111110), BIN); // checksum is 0010 - 19'
    Serial.println();
    Serial.println(IrSender.computeLGRawDataAndChecksum(0b01110111, 0b1111011110111110), BIN);
    Serial.println();
    Serial.println(computeLG3RawDataAndChecksum(0b01110111, 0b1111011110101110), BIN); // checksum is 0001 - 20'
    Serial.println();
    Serial.println(IrSender.computeLGRawDataAndChecksum(0b01110111, 0b1111011110101110), BIN);
}



void loop() {
    /*
     * Check if received data is available and if yes, try to decode it.
     * Decoded hash result is in IrReceiver.decodedIRData.decodedRawData
     */
    if (IrReceiver.available()) {
        IrReceiver.initDecodedIRData(); // is required, if we do not call decode();
        // IrReceiver.decodeHash();
        IrReceiver.decode();
        IrReceiver.resume(); // Early enable receiving of the next IR frame
        /*
         * Print a summary and then timing of received data
         */
        IrReceiver.printIRResultShort(&Serial);
        IrReceiver.printIRResultRawFormatted(&Serial, true);
        IrReceiver.printIRSendUsage(&Serial);

        Serial.println();
        // delay(1000);
        // sendLG3();
        

        /*
         * Finally, check the received data and perform actions according to the received command
         */
        // auto tDecodedRawData = IrReceiver.decodedIRData.decodedRawData; // uint32_t on 8 and 16 bit CPUs and uint64_t on 32 and 64 bit CPUs
    }
}