/*The MIT License (MIT)

Copyright (c) 2014 Nathanaël Lécaudé
https://github.com/natcl/Artnet, http://forum.pjrc.com/threads/24688-Artnet-to-OctoWS2811

Copyright (c) 2016,2019 Stephan Ruloff
https://github.com/rstephan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "ArtnetWifi.h"
#include <Arduino.h>

const char ArtnetWifi::artnetId[] = ART_NET_ID;

ArtnetWifi::ArtnetWifi()
  : artDmxCallback(nullptr) {}

void ArtnetWifi::begin(String hostname, const char* nameShort, const char* nameLong, uint16_t numberOfPorts) {
  Udp.begin(ART_NET_PORT);

  size_t input_length = strlen(nameShort);
  size_t characters_copied = (input_length >= 18) ? 17 : input_length;  // leave room for NUL

  memcpy(shortName, nameShort, characters_copied);
  memset(&shortName[characters_copied], 0, 18 - characters_copied);  // zero out leftower space

  input_length = strlen(nameLong);
  characters_copied = input_length >= 64 ? 63 : input_length;
  memcpy(longName, nameLong, characters_copied);
  memcpy(&longName[characters_copied], 0, 64 - characters_copied);


  portCount = numberOfPorts;
  host = hostname;
  sequence = 1;
  physical = 0;
}

void ArtnetWifi::stop(void) {
  Udp.stop();
}

uint16_t ArtnetWifi::read() {
  packetSize = Udp.parsePacket();

  if (packetSize <= MAX_BUFFER_ARTNET && packetSize > 0) {
    senderIp = Udp.remoteIP();
    Udp.read(artnetPacket, MAX_BUFFER_ARTNET);

    // Check that packetID is "Art-Net" else ignore
    if (memcmp(artnetPacket, artnetId, sizeof(artnetId)) != 0) {
      return 0;
    }

    // for (int i = 0; i < packetSize; i++) {
    //   Serial.print(artnetPacket[i], HEX);
    // }
    // Serial.print(" ");

    opcode = artnetPacket[8] | artnetPacket[9] << 8;
    //Serial.println(opcode);

    if (opcode == ART_DMX) {
      sequence = artnetPacket[12];
      incomingUniverse = artnetPacket[14] | artnetPacket[15] << 8;
      dmxDataLength = artnetPacket[17] | artnetPacket[16] << 8;

      if (artDmxCallback) (*artDmxCallback)(incomingUniverse, dmxDataLength, sequence, artnetPacket + ART_DMX_START);
#if !defined(ARDUINO_AVR_UNO_WIFI_REV2)
      if (artDmxFunc) {
        artDmxFunc(incomingUniverse, dmxDataLength, sequence, artnetPacket + ART_DMX_START);
      }
#endif
      return ART_DMX;
    }
    if (opcode == ART_POLL) {
      senderIp = Udp.remoteIP();
      pollReply(senderIp);

      return ART_POLL;
    }
    if (opcode == ART_SYNC) {
      return ART_SYNC;
    }
  }

  return 0;
}

void ArtnetWifi::pollReply(arduino::IPAddress ip) {
  memset(pollReplyBuffer, 0, POLL_PACKET_SIZE);

  memcpy(pollReplyBuffer, artnetId, sizeof(artnetId));  // 0-7
  opcode = ART_POLL_REPLY;

  pollReplyBuffer[8] = opcode;       // 8 L
  pollReplyBuffer[9] = opcode >> 8;  // 9 H

  uint32_t selfip_int = (uint32_t)selfIp;

  pollReplyBuffer[10] = selfip_int;  // L
  pollReplyBuffer[11] = selfip_int >> 8;
  pollReplyBuffer[12] = selfip_int >> 16;
  pollReplyBuffer[13] = selfip_int >> 24;  // L

  uint16_t port = 0x1936;
  pollReplyBuffer[14] = port;       //L
  pollReplyBuffer[15] = port >> 8;  // H

  uint16_t version = 14;
  pollReplyBuffer[16] = version >> 8;  // H
  pollReplyBuffer[17] = version;       // L

  // ArtNet sub, net switch
  pollReplyBuffer[18] = 0;
  pollReplyBuffer[19] = 0;

  // OEM product id
  pollReplyBuffer[20] = 0xff;
  pollReplyBuffer[21] = 0xff;

  pollReplyBuffer[22] = 0;  // UBEA support

  // Status
  pollReplyBuffer[23] = 0b11010000;

  // manufacturer code
  pollReplyBuffer[24] = 0xff;
  pollReplyBuffer[25] = 0xff;

  memcpy(&pollReplyBuffer[26], shortName, strlen(shortName));

  memcpy(&pollReplyBuffer[44], longName, strlen(longName));  // 44-107

  pollReplyBuffer[172] = portCount >> 8;  // H
  pollReplyBuffer[173] = portCount;       // L

  // Port types
  pollReplyBuffer[174] = portCount >= 1 ? PORT_ON_PATTERN : PORT_OFF_PATTERN;
  pollReplyBuffer[175] = portCount >= 2 ? PORT_ON_PATTERN : PORT_OFF_PATTERN;
  pollReplyBuffer[176] = portCount >= 3 ? PORT_ON_PATTERN : PORT_OFF_PATTERN;
  pollReplyBuffer[177] = portCount >= 4 ? PORT_ON_PATTERN : PORT_OFF_PATTERN;

  // Port input fromat
  pollReplyBuffer[178] = portCount >= 1 ? OUTPUT_TYPE_PORT_ON_PATTERN : OUTPUT_TYPE_PORT_OFF_PATTERN;
  pollReplyBuffer[179] = portCount >= 2 ? OUTPUT_TYPE_PORT_ON_PATTERN : OUTPUT_TYPE_PORT_OFF_PATTERN;
  pollReplyBuffer[180] = portCount >= 3 ? OUTPUT_TYPE_PORT_ON_PATTERN : OUTPUT_TYPE_PORT_OFF_PATTERN;
  pollReplyBuffer[181] = portCount >= 4 ? OUTPUT_TYPE_PORT_ON_PATTERN : OUTPUT_TYPE_PORT_OFF_PATTERN;

  // Input universe config
  pollReplyBuffer[190] = portCount >= 1 ? 0 : 0;  // Univ 0
  pollReplyBuffer[191] = portCount >= 2 ? 1 : 0;
  pollReplyBuffer[192] = portCount >= 3 ? 2 : 0;
  pollReplyBuffer[193] = portCount >= 4 ? 3 : 0;  // Univ 4

  // MAC address
  pollReplyBuffer[201] = macAddr[0];  // H
  pollReplyBuffer[202] = macAddr[1];
  pollReplyBuffer[203] = macAddr[2];
  pollReplyBuffer[204] = macAddr[3];
  pollReplyBuffer[205] = macAddr[4];
  pollReplyBuffer[206] = macAddr[5];  // L

  // Rest of the fields are left zero

  Udp.beginPacket(ip, ART_NET_PORT);
  Udp.write(pollReplyBuffer, POLL_PACKET_SIZE);
  Udp.endPacket();
}

uint16_t ArtnetWifi::makePacket(void) {
  uint16_t len;
  uint16_t version;

  memcpy(artnetPacket, artnetId, sizeof(artnetId));
  opcode = ART_DMX;
  artnetPacket[8] = opcode;
  artnetPacket[9] = opcode >> 8;
  version = 14;
  artnetPacket[11] = version;
  artnetPacket[10] = version >> 8;
  artnetPacket[12] = sequence;
  sequence++;
  if (!sequence) {
    sequence = 1;
  }
  artnetPacket[13] = physical;
  artnetPacket[14] = outgoingUniverse;
  artnetPacket[15] = outgoingUniverse >> 8;
  len = dmxDataLength + (dmxDataLength % 2);  // make a even number
  artnetPacket[17] = len;
  artnetPacket[16] = len >> 8;

  return len;
}

int ArtnetWifi::write(void) {
  uint16_t len;

  len = makePacket();
  Udp.beginPacket(host.c_str(), ART_NET_PORT);
  Udp.write(artnetPacket, ART_DMX_START + len);

  return Udp.endPacket();
}

int ArtnetWifi::write(IPAddress ip) {
  uint16_t len;

  len = makePacket();
  Udp.beginPacket(ip, ART_NET_PORT);
  Udp.write(artnetPacket, ART_DMX_START + len);

  return Udp.endPacket();
}

void ArtnetWifi::setByte(uint16_t pos, uint8_t value) {
  if (pos > 512) {
    return;
  }
  artnetPacket[ART_DMX_START + pos] = value;
}

void ArtnetWifi::printPacketHeader(void) {
  Serial.print("packet size = ");
  Serial.print(packetSize);
  Serial.print("\topcode = ");
  Serial.print(opcode, HEX);
  Serial.print("\tuniverse number = ");
  Serial.print(incomingUniverse);
  Serial.print("\tdata length = ");
  Serial.print(dmxDataLength);
  Serial.print("\tsequence n0. = ");
  Serial.println(sequence);
}

void ArtnetWifi::printPacketContent(void) {
  for (uint16_t i = ART_DMX_START; i < dmxDataLength; i++) {
    Serial.print(artnetPacket[i], DEC);
    Serial.print("  ");
  }
  Serial.println('\n');
}
