#include <ArtnetWifi.h>

struct WiFiSettings {
  String ssid;
  String password;
};

class SettingsServer {
  WiFiServer server;
  String ssid_input;
  String passw_input;
private:
  String _extract_input(String line, const char* id) {
    /*
    Example header line:
    GET /?text1=aaa&text2=bbb HTTP/1.1
          -----.^^^ -----.^^^!,,,,,,,,
  */

    String userInput = "";

    String attribute = id;
    attribute += "=";

    int textPos = line.indexOf(attribute);
    if (textPos != -1) {
      int startPos = textPos + attribute.length();
      int endPos = line.indexOf('&', startPos);  // extract until next attribute
      if (endPos == -1) {
        endPos = line.indexOf(' ', startPos);  // if last attribute select until header part end
        if (endPos == -1) {
          endPos = line.length();  // otherwise select whole thing
        }
      }

      userInput = line.substring(startPos, endPos);

      // cleanup url encoding
      userInput = _decode_url_special_chars(userInput);
    }

    return userInput;
  }
  String _decode_url_special_chars(String input) {
    input.replace("+", " ");
    input.replace("%20", " ");
    input.replace("%21", "!");
    input.replace("%22", "\"");
    input.replace("%23", "#");
    input.replace("%24", "$");
    input.replace("%25", "%");
    input.replace("%26", "&");
    input.replace("%27", "\'");
    input.replace("%28", "(");
    input.replace("%29", ")");
    input.replace("%2A", "*");
    input.replace("%2B", "+");
    input.replace("%2C", ",");
    input.replace("%2D", "-");
    input.replace("%2E", ".");
    input.replace("%2F", "/");
    input.replace("%3A", ":");
    input.replace("%3B", ";");
    input.replace("%3C", "<");
    input.replace("%3D", "=");
    input.replace("%3E", ">");
    input.replace("%3F", "?");
    input.replace("%40", "@");
    input.replace("%5B", "[");
    input.replace("%5C", "\\");
    input.replace("%5D", "]");
    input.replace("%5E", "^");
    input.replace("%5F", "_");
    input.replace("%60", "`");
    input.replace("%7B", "{");
    input.replace("%7C", "|");
    input.replace("%7D", "}");
    input.replace("%7E", "~");

    return input;
  }
public:
  SettingsServer() {
    WiFiServer wifi_server(80);
    server = wifi_server;

    ssid_input = "";
    passw_input = "";
  }
  void init() {
    server.begin();
  }
  void end() {
    server.end();
  }
  bool is_results_avaiable() {
    return passw_input.length() >= 8 && ssid_input.length() > 1;
  }
  WiFiSettings get_results() {
    WiFiSettings result;
    result.ssid = ssid_input;
    result.password = passw_input;
    return result;
  }
  void run() {
    // Excpected to run in a loop

    WiFiClient client = server.available();  // listen for incoming clients

    if (client) {  // if you get a client,

      Serial.println("new client");  // print a message out the serial port

      String currentLine = "";  // make a String to hold incoming data from the client
      String userInput_ssid = "";
      String userInput_passw = "";

      while (client.connected()) {  // loop while the client's connected

        if (client.available()) {  // if there's bytes to read from the client,

          char c = client.read();  // read a byte, then

          Serial.write(c);  // print it out the serial monitor

          if (c == '\n') {  // if the byte is a newline character

            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {

              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();

              // the content of the HTTP response follows the header:
              client.print("<div style='display: flex'>");
              client.print("  <div style='margin: auto'>");
              client.print("    <h2>ArtNet Node WiFi Settings</h2>");
              client.print("    <form method='GET'>");
              client.print("      <label for='ssid'>Network name:</label><br/>");
              client.print("      <input type='text' id='ssid' name='ssid' style='width: 100%' /><br/><br/>");
              client.print("      <label for='passw'>Password:</label><br/>");
              client.print("      <input type='text' id='passw' name='passw' style='width: 100%' /><br/>");
              client.print("      <br/>");
              client.print("      <input type='submit' value='Connect' style='margin: auto; display: flex' />");
              client.print("    </form>");
              client.print("  </div>");
              client.print("</div>");



              client.println();  // The HTTP response ends with another blank line

              // break out of the while loop:
              break;
            }
            // if you got a newline, then clear the current line,
            // also extract any returned data
            else {

              // We add any potentially returned data to their respective buffers,
              // if the current line has no return data just add nothing(""):
              userInput_ssid += _extract_input(currentLine, "ssid");
              userInput_passw += _extract_input(currentLine, "passw");

              currentLine = "";
            }

          }
          // if you got anything else but a carriage return character,
          // add it to the end of the currentLine
          else if (c != '\r') {
            currentLine += c;
          }
        }
      }

      // Client is done transmitting:
      ssid_input = userInput_ssid;
      passw_input = userInput_passw;
      Serial.println("User input SSID: \"" + userInput_ssid + "\"");
      Serial.println("User input PASSWORD: \"" + userInput_passw + "\"");

      // close the connection:
      client.stop();
      Serial.println("client disconnected");
    }
  }
};