#include <Arduino.h>
#include "WiFiS3.h"
#include <Wire.h>
#include <LiquidCrystal.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>LED kontroller</title>
  </head>
  <body>
    <main
      style="
        font-family: sans-serif;
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 0.5rem;
      "
    >
      <h1>Skru av og på LED lyset</h1>
      <div style="display: flex; justify-content: center; gap: 0.5rem">
        <a
          href="/H"
          style="
            background-color: #8de4bd;
            border-radius: 25px;
            padding: 10px 25px;
            text-decoration: none;
            color: black;
          "
          >På</a
        >
        <a
          href="/L"
          style="
            background-color: #f57f7d;
            border-radius: 25px;
            padding: 10px 25px;
            text-decoration: none;
            color: black;
          "
          >Av</a
        >
      </div>
      <form action="/lcd" method="post">
        <input
          id="message"
          type="text"
          name="message"
          placeholder="Skriv inn noe"
          autofocus
          style="
            padding: 10px 15px;
            border-radius: 25px;
            border: 1px solid #ccc;
            margin-top: 1rem;
          "
        />
        <button
          type="submit"
          style="
            background-color: #8de4bd;
            border-radius: 25px;
            padding: 10px 25px;
            border: none;
            margin-top: 1rem;
          "
        >
          Send
        </button>
      </form>
    </main>
  </body>
  <script>
    const message = document.getElementById("message");
    message.addEventListener("input", () => {
      if (message.value.length > 32) {
        message.value = message.value.slice(0, 32);
      }
    });

    const form = document.querySelector("form");
    form.addEventListener("submit", (e) => {
      e.preventDefault();
      const formData = new FormData(form);
      fetch("/lcd", {
        method: "POST",
        body: formData,
      }).then(() => {
        form.reset();
      });
    });
  </script>
</html>

)rawliteral";

char ssid[] = "";
char password[] = "";

int led =  LED_BUILTIN;
int status = WL_IDLE_STATUS;
String message = "";
WiFiServer server(80);
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}

void setup() {
  Serial.begin(9600);      // initialize serial communication
  pinMode(led, OUTPUT);      // set the LED pin mode
  lcd.begin(16, 2);          // set up the LCD's number of columns and rows:

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, password);
    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status
}


void loop() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out to the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print(index_html);
            
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_BUILTIN, LOW);                // GET /L turns the LED off
        }
        if (currentLine.endsWith("POST /lcd")) {
          String postData = "";
          while (client.available()) {
            char c = client.read();
            postData += c; // Collect all incoming data into a string
          }

          // Find the "message" field in the postData
          String boundary = postData.substring(0, postData.indexOf("\r\n")); // Extract boundary
          int messageIndex = postData.indexOf("name=\"message\"");
          if (messageIndex != -1) {
            int valueStart = postData.indexOf("\r\n\r\n", messageIndex) + 4; // Locate the value start
            int valueEnd = postData.indexOf(boundary, valueStart) - 4;       // Locate the value end
            String message = postData.substring(valueStart, valueEnd);

            // remove "------WebKitFormBoundary..." from the message
            int boundaryIndex = message.indexOf("------");
            if (boundaryIndex != -1) {
              message = message.substring(0, boundaryIndex);
            }
            message.trim();

            Serial.println();
            Serial.println("Message: " + message);
            Serial.println("length: " + String(message.length()));
            Serial.println();
            
            lcd.clear();
            delay(10);
            if (message.length() > 16 && message.length() <= 32) {
              lcd.setCursor(0, 0);
              lcd.print(message.substring(0, 16));
              lcd.setCursor(0, 1);
              lcd.print(message.substring(16, 32));
            } else if (message.length() > 32) {
              lcd.setCursor(0, 0);
              lcd.print("Message too long");
            } else {
              lcd.setCursor(0, 0);
              lcd.print(message);
            }
            message = "";
          }

          // Send a response back to the client
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.print(index_html);
          client.println();
          break;
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}
