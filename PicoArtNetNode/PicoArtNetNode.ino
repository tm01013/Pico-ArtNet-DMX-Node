/*
  Original created 10/18/2023 by Taylor Mingos
  Custom modifications by 2026 TM.
*/
// WiFi is WiFiNINA under the hood

#include <Arduino.h>
#include "ArtnetWifi.h"
#include <DmxOutput.h>
#include "OLED.h"
#include "PicoWebServer.h"
#include <limits>

#define INF std::numeric_limits<int>::max()

// Configure outputs
#define OUTPUT_A_UNIVERSE 0  // Which universe maps to output A
#define OUTPUT_B_UNIVERSE 1  // Which universe maps to output B
#define OUTPUT_A_GPIO 0
#define OUTPUT_B_GPIO 1

// Configure WiFi
const char* SSID = "ArtNet";
const char* PASSWORD = "qizk7486";

// Create a universe
#define UNIVERSE_LENGTH 512
uint8_t channels_A[UNIVERSE_LENGTH + 1];
uint8_t channels_B[UNIVERSE_LENGTH + 1];

// Init Art-Net
ArtnetWifi artnet;

// Init two DMX outputs
DmxOutput dmx[2];

// For controller discovery
#define POLLING_ENABLED 1
#define DISCONNECT_TREASHOLD_MILIS 6000
#define POLLING_RATE 3000
enum ControllerConnectionState {
  NO_CONTROLLER,
  CONNECTED,
  DISCONNECTED,
};
struct Controller {
  char name[18] = {};
  unsigned long lastSeen = 0;

  Controller(const char* shortName, unsigned long time) {
    memcpy(name, shortName, strlen(shortName) >= sizeof(name) ? sizeof(name) : strlen(shortName));
    lastSeen = time;
  }
  Controller() {}

  bool operator==(Controller b) {
    return (memcmp(name, b.name, sizeof(name)) == 0) && (lastSeen == b.lastSeen);
  }
};
String currentSSID = SSID;
Controller controllers[2] = { Controller() };
ControllerConnectionState controllerConnectionState = NO_CONTROLLER;

// Init OLED display for displaying WiFi status and configuration info
OLED oled(display, OUTPUT_A_UNIVERSE, OUTPUT_B_UNIVERSE);

bool createAP(void) {
  bool connected = true;
  int attempts = 0;

  oled.displayWifiConnecting(SSID, nullptr);
  Serial.println("Connecting");

  // Create Aaccess Point

  // These ip's are not random! Dont touch them!
  // Geteway must be 192.168.4.1
  // Subnet to be 255.255.255.0
  IPAddress local_ip(192, 168, 4, 2);
  IPAddress gateway_ip(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_ip, gateway_ip, subnet);
  WiFi.setHostname("pi");
  WiFi.softAP(SSID, PASSWORD, 1, 0, 10);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
    if (attempts > 30) {
      connected = false;
      break;
    }
    attempts++;
  }

  // Display WiFi connection result
  if (connected) {
    print_wifi_details(SSID);
    oled.displayConnected(SSID, WiFi.softAPIP().toString().c_str(), PASSWORD);
    currentSSID = SSID;

    uint8_t mac[6];
    WiFi.macAddress(mac);
    artnet.setSelfIp(WiFi.softAPIP(), mac);

  } else {
    Serial.println("WiFi Connection Failed");
    oled.displayWifiError();
    delay(5000);
  }

  return connected;
}

// Connect to Wifi
bool connectToWifi(const char* ssid, const char* password, int max_attempts, int delay_ms) {
  bool connected = true;
  int attempts = 0;

  // Connect to Wifi
  oled.displayWifiConnecting(ssid, password);
  //Serial.print("Connecting");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(delay_ms);
    Serial.print("#");
    if (attempts > max_attempts) {
      connected = false;
      break;
    }
    attempts++;
  }

  // Display WiFi connection result
  if (connected) {
    print_wifi_details(ssid);
    oled.displayConnected(ssid, WiFi.localIP().toString().c_str());
    currentSSID = ssid;

    uint8_t mac[6];
    WiFi.macAddress(mac);
    artnet.setSelfIp(WiFi.localIP(), mac);
  } else {
    //Serial.println("WiFi Connection Failed");
    //oled.displayWifiError();
    //delay(3000);

    Serial.print("Reason code: ");
    Serial.println(WiFi.reasonCode());
  }

  return connected;
}

bool searchWifi(const char* ssid, const char* password) {
  int attempts = 0;
  const int max_attempts = 10;

  bool is_connected = false;

  Serial.println("Searching for networks...");
  oled.displaySearching();

  while (attempts < max_attempts && !is_connected) {
    int numNetworks = WiFi.scanNetworks();

    if (numNetworks == -1) {
      continue;
    }

    for (int i = 0; i < numNetworks; i++) {
      String current_ssid = WiFi.SSID(i);
      Serial.println("\"" + current_ssid + "\"");
      if (ssid != current_ssid) {
        continue;
      }

      is_connected = connectToWifi(current_ssid.c_str(), password, 2, 500);

      if (is_connected) {
        return true;
      }

      oled.displaySearching();
    }

    attempts++;
    Serial.print(".");
    delay(500);
  }

  oled.displayWifiError();
  while (true) {}  // get stuck
}

void webSetup() {
  Serial.println("Settings mode!");

  while (!createAP()) {}
  oled.displaySettingsMode(SSID, PASSWORD, ("http://" + WiFi.softAPIP().toString() + "/").c_str());

  SettingsServer server;
  server.init();

  String ssid = "";
  String passw = "";
  bool is_setup_complete = false;
  while (!is_setup_complete) {
    server.run();

    if (server.is_results_avaiable()) {
      WiFiSettings settings = server.get_results();
      ssid = settings.ssid;
      passw = settings.password;
      ssid.trim();
      passw.trim();

      is_setup_complete = true;
    }
  }

  server.end();  // stop web server
  WiFi.end();    // End AP
  delay(1000);   // AP is practicly async so wait a little

  unsigned long start = millis();  // if it's still active wait further
  while (WiFi.status() == WL_AP_LISTENING && millis() - start < 3000) {
    delay(100);
  }

  WiFi.config(0, 0, 0);  // Clear config


  Serial.println("SSID: " + ssid);
  Serial.println("Password: " + passw);

  delay(500);

  bool connected = connectToWifi(ssid.c_str(), passw.c_str(), 5, 500);  // seems to be unreliable
  if (!connected) {
    // So if first try failed: try searching:
    searchWifi(ssid.c_str(), passw.c_str());  // appears to be working reliably this time
    // No clue why this happens, just that it works

    //oled.displayWifiError();
    //while (true) {}  // get stuck
  }

  return;
}

void print_wifi_details(String ssid) {
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(IPAddress(WiFi.localIP()));
  Serial.print("AP IP: ");
  Serial.println(IPAddress(WiFi.softAPIP()));
  Serial.print("GATEWAY: ");
  Serial.println(IPAddress(WiFi.gatewayIP()));
  Serial.print("SUBNET: ");
  Serial.println(IPAddress(WiFi.subnetMask()));
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
  /*Serial.print("DMX: Univ: "); // NOTE: print statements impact preformance !
  Serial.print(universe, DEC);
  Serial.print(", Data Length: ");
  Serial.print(length, DEC);
  Serial.print(", Sequence: ");
  Serial.print(sequence, DEC);
  Serial.print(", Data: ");*/

  // for (int i = 0; i < length; i++) {
  //   /*if (i < 16) {
  //     Serial.print(data[i], DEC);
  //     Serial.print(" ");
  //   }*/
  //   channels[i + 1] = data[i];
  // }

  readIntoUniverse(universe, length, data);

  //Serial.println();

  sendDmxFrame(universe);

  // // Send out the universe if it's a universe we care about
  // if (universe == OUTPUT_A_UNIVERSE) {
  //   dmx[0].write(channels, UNIVERSE_LENGTH);
  // } else if (universe == OUTPUT_B_UNIVERSE) {
  //   dmx[1].write(channels, UNIVERSE_LENGTH);
  // }

  while (dmx[0].busy() || dmx[1].busy()) {
    // Do nothing while the DMX frame transmits
  }
}

void readIntoUniverse(uint16_t universe, uint16_t length, uint8_t* data) {
  if (universe == OUTPUT_A_UNIVERSE) {
    for (int i = 0; i < length; i++) {
      channels_A[i + 1] = data[i];
    }
  } else if (universe == OUTPUT_B_UNIVERSE) {
    for (int i = 0; i < length; i++) {
      channels_B[i + 1] = data[i];
    }
  }
}

void sendDmxFrame(uint16_t universe) {
  if (!(dmx[0].busy() || dmx[1].busy())) {
    if (universe == OUTPUT_A_UNIVERSE) {
      dmx[0].write(channels_A, UNIVERSE_LENGTH);
    } else if (universe == OUTPUT_B_UNIVERSE) {
      dmx[1].write(channels_B, UNIVERSE_LENGTH);
    }
  }
}

void onPollReply(ArtPollReply reply) {
  //Serial.println(reply.shortName);

  bool isSeceenRefreshNeeded = false;

  unsigned long currentTime = millis();

  size_t arrayLength = sizeof(controllers) / sizeof(controllers[0]);

  bool isControllerNew = true;

  // check if controller disconnected
  for (int i = 0; i < arrayLength; i++) {
    if (memcmp(controllers[i].name, reply.shortName, sizeof(controllers[i].name)) == 0) {
      controllers[i].lastSeen = currentTime;

      isControllerNew = false;

      controllerConnectionState = CONNECTED;
    }

    // if (currentTime - controllers[i].lastSeen > POLLING_RATE + DISCONNECT_TREASHOLD_MILIS) {
    //   controllers[i] = Controller();

    //   isSeceenRefreshNeeded = true;
    // }
  }
  // // Slide array elements
  // if (isSeceenRefreshNeeded) {
  //   for (int i = 1; i < arrayLength; i++) {
  //     if (controllers[i - 1] == Controller()) {  // aka empty
  //       controllers[i - 1] = controllers[i];
  //       controllers[i] = Controller();
  //     }
  //   }
  // }

  int firstEmptyIndex = INF;
  for (int i = 0; i < arrayLength; i++) {
    // Serial.print(controllers[i].name);
    // Serial.print(" ");
    // Serial.println(controllers[i].lastSeen);

    if (controllers[i] == Controller()) {  // aka empty
      firstEmptyIndex = i;
      break;
    }
  }

  //Serial.println(firstEmptyIndex);

  // Add in new controller
  if (isControllerNew && firstEmptyIndex != INF) {
    controllers[firstEmptyIndex] = Controller(reply.shortName, currentTime);

    isSeceenRefreshNeeded = true;
    firstEmptyIndex++;
    controllerConnectionState = CONNECTED;

    //Serial.println("Controller added");
  }

  /*if (firstEmptyIndex == 0) {
    oled.displayControllersDisconnected(currentSSID.c_str());
    //Serial.println("Controllers disconnected");
  } else*/
  if (isSeceenRefreshNeeded) {
    oled.displayControllersConnected(currentSSID.c_str(), controllers[0].name, controllers[1].name);
  }
  // else {
  //   Serial.println("No action");
  // }
}

void cleanupControllers() {
  bool isSeceenRefreshNeeded = false;

  unsigned long currentTime = millis();

  size_t arrayLength = sizeof(controllers) / sizeof(controllers[0]);

  for (int i = 0; i < arrayLength; i++) {
    if (controllers[i] == Controller()) continue;

    if (currentTime - controllers[i].lastSeen > POLLING_RATE + DISCONNECT_TREASHOLD_MILIS) {
      controllers[i] = Controller();

      isSeceenRefreshNeeded = true;
    }
  }

  // Slide array elements
  if (isSeceenRefreshNeeded) {
    for (int i = 1; i < arrayLength; i++) {
      if (controllers[i - 1] == Controller()) {
        controllers[i - 1] = controllers[i];
        controllers[i] = Controller();
      }
    }
  }

  bool isArrayEmpty = true;
  for (int i = 0; i < arrayLength; i++) {
    if (controllers[i] != Controller()) {
      isArrayEmpty = false;
      break;
    }
  }

  if (isArrayEmpty && controllerConnectionState == CONNECTED) {
    controllerConnectionState = DISCONNECTED;
    oled.displayControllersDisconnected(currentSSID.c_str());
  } else if (isSeceenRefreshNeeded) {
    oled.displayControllersConnected(currentSSID.c_str(), controllers[0].name, controllers[1].name);
  }
}

void setup() {
  // Set up serial for debug
  Serial.begin(115200);

  oled.initDisplay();

  // Turn on Pico's on-board LED
  // This shows that the Pico is turned on
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(D13, INPUT_PULLDOWN);
  delay(250);  // Wait is needed otherwise the button press later wont be registered becouse code is so fast.

  // Start the DMX outputs
  dmx[0].begin(OUTPUT_A_GPIO);
  dmx[1].begin(OUTPUT_B_GPIO);

  // Set all channels to zero
  for (int i = 1; i < UNIVERSE_LENGTH + 1; i++) {
    channels_A[i] = 0;
    channels_B[i] = 0;
  }

  WiFi.setTimeout(2500);

  // Check if settings button is pressed
  int buttonState = digitalRead(D13);
  if (buttonState == HIGH) {
    //while(!searchWifi()) {}
    webSetup();
  } else {
    // Create AP
    while (!createAP()) {}
  }

  //while (!createAP()) {}

  artnet.setArtDmxCallback(onDmxFrame);
  artnet.setArtPollReplyCallback(onPollReply);
  artnet.begin("", "DOBOZ", "Varazslatos teglatest", 2);
}

void loop() {
  artnet.read();
}

void loop1() {
  if (POLLING_ENABLED) {
    delay(POLLING_RATE);
    artnet.pollOtherNodes();
    cleanupControllers();
  }
}