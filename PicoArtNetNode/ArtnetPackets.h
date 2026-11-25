
enum ArtDeviceType {
  StNode = 0x00,
  StController = 0x01,
  StMedia = 0x02,
  StRemote = 0x03,
  StBackup = 0x04,
  StConfig = 0x05,
  StVisual = 0x06,
};

struct ArtPollReply {
  char id[8] = "Art-Net";  // constant by standard
  uint16_t opcode = 0x2100;
  /* index 0 is the high byte*/
  uint8_t ip[4] = {};
  uint16_t port = ART_NET_PORT;  // constant by standard
  uint16_t version = 14;         // constant by standard
  uint8_t sw = 0;
  uint8_t subsw = 0;
  uint16_t oem = 0xffff;
  uint8_t ubea = 0;
  uint8_t status = 0b11010000;
  uint16_t esta = 0xffff;
  char shortName[18] = {};
  char longName[64] = {};
  char report[64] = {};
  uint16_t portCount = 0;
  uint8_t portTypes[4] = {};
  uint8_t allowedInputTypes[4] = {};
  uint8_t outputTypes[4] = {};
  uint8_t transmittedUniverses[4] = {};
  uint8_t outputPortUniverseBindings[4] = {};
  uint8_t acnPrioraty = 0;
  uint8_t macroState = 0;
  uint8_t remoteTriggerState = 0;
  ArtDeviceType deviceType = StNode;
  /* index 0 is the high byte*/
  uint8_t macAddr[6] = {};

  void serializeIntoBuffer(uint8_t* buffer) {
    // Id
    memcpy(buffer, ART_NET_ID, sizeof(ART_NET_ID));  // 0-7

    opcode = ART_POLL_REPLY;
    buffer[8] = opcode;       // 8 L
    buffer[9] = opcode >> 8;  // 9 H

    // sender ip
    buffer[10] = ip[0];  // H
    buffer[11] = ip[1];
    buffer[12] = ip[2];
    buffer[13] = ip[3];  // L

    // network port
    buffer[14] = port;       //L
    buffer[15] = port >> 8;  // H

    // version
    buffer[16] = version >> 8;  // H
    buffer[17] = version;       // L

    // ArtNet sub, net switch
    buffer[18] = sw;
    buffer[19] = subsw;

    // OEM product id
    buffer[20] = oem >> 8;  // H
    buffer[21] = oem;       // L

    // UBEA support
    buffer[22] = ubea;

    // Status
    buffer[23] = status;

    // manufacturer code
    buffer[24] = esta;       // L
    buffer[25] = esta >> 8;  // H

    memcpy(&buffer[26], shortName, 18);  // 26-43

    memcpy(&buffer[44], longName, 64);  // 44-107

    memcpy(&buffer[108], report, 64);  // 108-171

    // Port count
    buffer[172] = portCount >> 8;  // H
    buffer[173] = portCount;       // L

    // Port types
    buffer[174] = portTypes[0];
    buffer[175] = portTypes[1];
    buffer[176] = portTypes[2];
    buffer[177] = portTypes[3];

    // Port input fromat
    buffer[178] = allowedInputTypes[0];
    buffer[179] = allowedInputTypes[1];
    buffer[180] = allowedInputTypes[2];
    buffer[181] = allowedInputTypes[3];

    // Port output fromat
    buffer[182] = outputTypes[0];
    buffer[183] = outputTypes[1];
    buffer[184] = outputTypes[2];
    buffer[185] = outputTypes[3];

    // Output universes
    buffer[182] = transmittedUniverses[0];
    buffer[183] = transmittedUniverses[1];
    buffer[184] = transmittedUniverses[2];
    buffer[185] = transmittedUniverses[3];

    // Input universe config
    buffer[190] = outputPortUniverseBindings[0];
    buffer[191] = outputPortUniverseBindings[1];
    buffer[192] = outputPortUniverseBindings[2];
    buffer[193] = outputPortUniverseBindings[3];

    buffer[194] = acnPrioraty;
    buffer[195] = macroState;
    buffer[196] = remoteTriggerState;

    // unused by spec
    buffer[197] = 0;
    buffer[198] = 0;
    buffer[199] = 0;

    // style
    buffer[200] = (uint8_t)deviceType;

    // MAC address
    buffer[201] = macAddr[0];  // H
    buffer[202] = macAddr[1];
    buffer[203] = macAddr[2];
    buffer[204] = macAddr[3];
    buffer[205] = macAddr[4];
    buffer[206] = macAddr[5];  // L
  }

  static ArtPollReply deserializeFromBuffer(uint8_t* buffer) {
    ArtPollReply reply;

    // Id
    memcpy(reply.id, buffer, sizeof(reply.id));  // 0-7

    // Opcode
    reply.opcode = buffer[8] | buffer[9] << 8;

    // sender ip
    reply.ip[0] = buffer[10];  // H
    reply.ip[1] = buffer[11];
    reply.ip[2] = buffer[12];
    reply.ip[3] = buffer[13];  // L

    // network port
    reply.port = buffer[14] | buffer[15] << 8;

    // version
    reply.version = buffer[16] << 8 | buffer[17];

    // ArtNet sub, net switch
    reply.sw = buffer[18];
    reply.subsw = buffer[19];

    // OEM product id
    reply.oem = buffer[20] << 8 | buffer[21];

    // UBEA support
    reply.ubea = buffer[22];

    // Status
    reply.status = buffer[23];

    // manufacturer code
    reply.esta = buffer[24] | buffer[25] << 8;

    memcpy(reply.shortName, &buffer[26], 18);  // 26-43

    memcpy(reply.longName, &buffer[44], 64);  // 44-107

    memcpy(reply.report, &buffer[108], 64);  // 108-171

    // Port count
    reply.portCount = buffer[172] << 8 | buffer[173];

    // Port types
    reply.portTypes[0] = buffer[174];
    reply.portTypes[1] = buffer[175];
    reply.portTypes[2] = buffer[176];
    reply.portTypes[3] = buffer[177];

    // Port input fromat
    reply.allowedInputTypes[0] = buffer[178];
    reply.allowedInputTypes[1] = buffer[179];
    reply.allowedInputTypes[2] = buffer[180];
    reply.allowedInputTypes[3] = buffer[181];

    // Port output fromat
    reply.outputTypes[0] = buffer[182];
    reply.outputTypes[1] = buffer[183];
    reply.outputTypes[2] = buffer[184];
    reply.outputTypes[3] = buffer[185];

    // Output universes
    reply.transmittedUniverses[0] = buffer[182];
    reply.transmittedUniverses[1] = buffer[183];
    reply.transmittedUniverses[2] = buffer[184];
    reply.transmittedUniverses[3] = buffer[185];

    // Input universe config
    reply.outputPortUniverseBindings[0] = buffer[190];
    reply.outputPortUniverseBindings[1] = buffer[191];
    reply.outputPortUniverseBindings[2] = buffer[192];
    reply.outputPortUniverseBindings[3] = buffer[193];

    reply.acnPrioraty = buffer[194];
    reply.macroState = buffer[195];
    reply.remoteTriggerState = buffer[196];

    // style
    reply.deviceType = (ArtDeviceType)buffer[200];

    // MAC address
    reply.macAddr[0] = buffer[201];  // H
    reply.macAddr[1] = buffer[202];
    reply.macAddr[2] = buffer[203];
    reply.macAddr[3] = buffer[204];
    reply.macAddr[4] = buffer[205];
    reply.macAddr[5] = buffer[206];  // L

    return reply;
  }
};

struct ArtPoll {
  char id[8] = "Art-Net";  // constant by standard
  uint16_t opcode = 0x2000;
  uint16_t version = 14;  // constant by standard
  uint8_t flags = 0b00000010;
  uint8_t diagnosticsPrioraty = 0;

  void serializeIntoBuffer(uint8_t* buffer) {
    memcpy(buffer, ART_NET_ID, sizeof(ART_NET_ID));  // 0-7

    buffer[8] = opcode;       // 8 L
    buffer[9] = opcode >> 8;  // 9 H

    buffer[10] = version >> 8;  // H
    buffer[11] = version;       // L

    buffer[12] = flags;

    buffer[13] = diagnosticsPrioraty;
  }

  static ArtPoll deserializeFromBuffer(uint8_t* buffer) {
    ArtPoll pollPacket;

    memcpy(pollPacket.id, buffer, sizeof(pollPacket.id));  // 0-7

    pollPacket.opcode = buffer[8] | buffer[9] << 8;

    pollPacket.version = buffer[10] << 8 | buffer[11];

    pollPacket.flags = buffer[12];

    pollPacket.diagnosticsPrioraty = buffer[13];

    return pollPacket;
  }
};