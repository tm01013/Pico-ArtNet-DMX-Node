#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

// SSD1306 display connected to I2C (SDA, SCL pins)
// On a RP2040:                 4(SDA), 5(SCL)
#define OLED_RESET -1        // Reset pin # 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define SCREEN_ADDRESS 0x3C  // Note: Some 128x64 displays start at 0x3D

#define WIFI_ICON_WIDTH     16
#define WIFI_ICON_HEIGHT    16
const unsigned char wifi_bmp [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x3e, 0x7c, 0x70, 0x0e, 0xc7, 0xe3, 0x1f, 0xf8, 0x38, 0x1c, 
	0x33, 0xcc, 0x0f, 0xf0, 0x0c, 0x30, 0x01, 0x80, 0x03, 0xc0, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};

const unsigned char ap_bmp [] PROGMEM = {
	0x07, 0xE0, 0x08, 0x10, 0x13, 0xC8, 0x24, 0x24, 0x29, 0x94, 0x03, 0xC0, 0x01, 0x80, 0x02, 0x40,
  0x06, 0x60, 0x04, 0x20, 0x0A, 0x50, 0x09, 0x90, 0x12, 0x48, 0x34, 0x2C, 0x28, 0x14, 0x78, 0x1E
};

const uint8_t hourglass_bmp [] PROGMEM = {
  0x7F, 0xFE, 0x20, 0x04, 0x20, 0x04, 0x10, 0x08, 0x17, 0xE8, 0x0B, 0xD0, 0x0D, 0xB0, 0x04, 0x20,
  0x05, 0x20, 0x0C, 0xB0, 0x08, 0x10, 0x11, 0x88, 0x13, 0xC8, 0x27, 0xE4, 0x2F, 0xF4, 0x7F, 0xFE
};

const uint8_t wifi_error_bmp [] PROGMEM = {
  0x00, 0x00, 0x07, 0xE0, 0x07, 0xE0, 0x37, 0xEC, 0x73, 0xCE, 0xC3, 0xC3, 0x13, 0xC8, 0x3B, 0xDC,
  0x31, 0x8C, 0x09, 0x90, 0x09, 0x90, 0x00, 0x00, 0x01, 0x80, 0x03, 0xC0, 0x01, 0x80, 0x00, 0x00
};

const uint8_t search_bmp [] PROGMEM = {
  0x00, 0x00, 0x0E, 0x00, 0x1B, 0x00, 0x20, 0x80, 0x60, 0xC0, 0x40, 0x40, 0x60, 0xC0, 0x20, 0x80,
  0x1B, 0xC0, 0x0E, 0xE0, 0x00, 0x70, 0x00, 0x38, 0x00, 0x1C, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00
};

// Unused
const uint8_t cog_bmp [] PROGMEM = {
  0x00, 0x00, 0x01, 0x80, 0x11, 0x88, 0x3B, 0xDC, 0x1F, 0xF8, 0x0F, 0xF0, 0x1E, 0x78, 0x7C, 0x3E,
  0x7C, 0x3E, 0x1E, 0x78, 0x0F, 0xF0, 0x1F, 0xF8, 0x3B, 0xDC, 0x11, 0x88, 0x01, 0x80, 0x00, 0x00
};

class OLED {
  private:
    Adafruit_SSD1306& displayRef;
    int universeA;
    int universeB;

    void _drawIcon(const unsigned char icon[]) {
      displayRef.drawBitmap(
          (displayRef.width() - WIFI_ICON_WIDTH - 5), 5,
          icon, WIFI_ICON_WIDTH, WIFI_ICON_HEIGHT, 1);
    }

    void _displayNetworkInfo(const char* ssid, const char *address, const char* password ) {
      displayRef.setCursor(0, 5);

      int title_size_offset = 7;
      if (strlen(ssid) <= 7) {
        displayRef.setTextSize(2);
        title_size_offset = 14;
      }

      displayRef.println(ssid);

      displayRef.setTextSize(1);
      displayRef.setCursor(0, 5 + title_size_offset + 5);
      displayRef.println(address);

      if (password != nullptr) {
        displayRef.setCursor(0, 5 + title_size_offset + 5 + 7 + 3);
        displayRef.println(password);
      }
      else {
        displayRef.println();
      }

      displayRef.println();
    }

    // TODO
    void _displayUniverseConfig() {
      char univA[4];
      char univB[4];
      String str;
      str = String(universeA);
      str.toCharArray(univA, 4);
      str = String(universeB);
      str.toCharArray(univB, 4);

      displayRef.drawLine(0, 50, displayRef.width(), 50, SSD1306_WHITE);
      displayRef.setCursor(0, 55);
      displayRef.write("U");
      displayRef.write(univA);
      displayRef.write(" -> A | ");
      displayRef.write("U");
      displayRef.write(univB);
      displayRef.write(" -> B");
      displayRef.display();

      displayRef.setCursor(0, 5);
    }
  
  public:
    OLED(Adafruit_SSD1306& display, int univA, int univB) 
      : displayRef(display), universeA(univA), universeB(univB) { }

    void initDisplay() {
      // Start up SSD1306 display
      if(!displayRef.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed
      }
      
      // Configure display
      displayRef.setTextSize(1);
      displayRef.setTextColor(SSD1306_WHITE);
    }

    void displayWifiConnecting(const char* ssid, const char* passworld) {
      displayRef.clearDisplay();
      _drawIcon(hourglass_bmp);
      _displayNetworkInfo(ssid, "Connecting...", passworld);

      displayRef.display();
    }

    void displayWifiError() {
      displayRef.clearDisplay();
      _drawIcon(wifi_error_bmp);

      displayRef.setCursor(0, 5);
      displayRef.setTextSize(2);
      displayRef.println("Error");

      displayRef.display();
    }

    void displaySearching() {
      displayRef.clearDisplay();
      _drawIcon(search_bmp);
      displayRef.setCursor(0, 5);
      displayRef.println("Searching...");
      displayRef.display();
    }

    void displayConnected(const char* ssid, const char *ipAddress) {
      displayRef.clearDisplay();
      _drawIcon(wifi_bmp);
      _displayUniverseConfig();
      _displayNetworkInfo(ssid, ipAddress, nullptr);
      displayRef.display();
    }
    void displayConnected(const char* ssid, const char *ipAddress, const char* password) {
      displayRef.clearDisplay();
      _drawIcon(ap_bmp);
      _displayUniverseConfig();
      _displayNetworkInfo(ssid, ipAddress, password);
      displayRef.display();
    }

    /*void displaySettingsMode(const char* ssid, const char* password, const char* url_address) {
      displayRef.clearDisplay();
      _drawIcon(cog_bmp);

      _displayNetworkInfo(ssid, url_address, password);
      displayRef.display();
    }*/
};


