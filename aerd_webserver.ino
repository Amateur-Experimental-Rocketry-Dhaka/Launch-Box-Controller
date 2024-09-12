#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

ESP8266WebServer server;
WebSocketsServer webSocket = WebSocketsServer(81);

#define IGNITION_PIN 0
#define HIGHFLOW_PIN 2
#define LOWFLOW_PIN 4
#define COOLING_PIN 5

const int num_pins = 4;
const char *ssid = "AERD_Control_Box"; // SSID for the Access Point
const char *password = "aerd1234"; // Password for the Access Point

bool fireBtnPress = false;
bool stopBtnPress = false;
unsigned long previousTime = 0;
int currentState = 0;
int loadIndex = 0;
uint16_t delayValues[num_pins] = {0};

char webpage[] PROGMEM = R"=====(
<html>

<head>
  <title>AERD WebServer</title>
  <script>
    var Socket;

    function init() {
      Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
      Socket.onmessage = function (event) {
        // document.getElementById("rxConsole").value += event.data;
      }
    }

    function fireInTheHole() {
      Socket.send("fire");
    }

    function sendValues() {
      var ignition = document.getElementById("ignition").value;
      var highflow = document.getElementById("highflow").value;
      var lowflow = document.getElementById("lowflow").value;
      var cooling = document.getElementById("cooling").value;

      Socket.send(ignition + "," + highflow + "," + lowflow + "," + cooling);
    }

    function stopAllLoads() {
      Socket.send("stop");
    }
  </script>
  <style>
    body {
      margin-top: 30px;
    }

    label {
      font-size: 25px;
    }

    input[type=number] {
      width: 50%;
      padding: 12px 20px;
      margin: 8px 0;
      box-sizing: border-box;
      border: none;
      border-radius: 5px;
      background-color: #D3D3D3;
      color: black;
      font-size: 25px;
    }

    input[type=reset] {
      width: 30%;
      padding: 12px 20px;
      margin: 8px 0;
      box-sizing: border-box;
      border: none;
      border-radius: 5px;
      background-color: #D3D3D3;
      color: black;
      font-size: 25px;
      margin-left: 10%;
      cursor: pointer;
    }

    .head {
      display: flex;
      justify-content: center;
      align-items: center;
    }

    .content {
      max-width: 50%;
      margin-top: 50px;
      margin-bottom: auto;
      margin-right: auto;
      margin-left: auto;
      display: inherit;
      width: 100%;
    }

    .title {
      font-size: 80px;
      margin-top: 0;
      margin-bottom: 0;
    }

    .aerdlogo {
      width: 100px;
      height: 100px;
      margin-right: 50px;
      border-radius: 25%;
      padding-bottom: 0;
    }

    .btn-fire {
      background-color: #E74C3C;
      border: none;
      color: white;
      padding: 15px;
      width: 60%;
      text-align: center;
      text-decoration: none;
      font-size: 18px;
      font-weight: bold;
      border-radius: 5px;
      display: inline-block;
      margin: 4px 2px;
      cursor: pointer;
      margin-left: 20%;
    }

    .float {
      position: fixed;
      width: 70px;
      height: 70px;
      bottom: 40px;
      right: 40px;
      background-color: red;
      color: #FFF;
      border-radius: 50px;
      text-align: center;
      box-shadow: 2px 2px 3px #999;
      text-decoration: none;
      font-size: 16px;
      cursor: pointer;
    }
  </style>
</head>

<body onload="javascript:init()">
  <header class="head">
    <img class="aerdlogo" src="https://i.postimg.cc/YCv2LzQb/AERD.jpg" alt="AERD Logo">
    <h1 class="title">AERD Control Box WebServer</h1>
  </header>
  <form action="">
    <div class="content" style="margin-top: 60px;">
      <table border="0" cellpadding="10" cellspacing="0" align="center">
        
        <tr>
          <td>
            <div>
              <label for="ignition">Ignition Time:</label>
            </div>
          </td>
          <td>
            <div>
              <input type="number" id="ignition" name="ignition">
            </div>
          </td>
        </tr>

        <tr>
          <td>
            <div>
              <label for="highflow">High Flow Time:</label>
            </div>
          </td>
          <td>
            <div>
              <input type="number" id="highflow" name="highflow">
            </div>
          </td>
        </tr>

        <tr>
          <td>
            <div>
              <label for="lowflow">Low Flow Time:</label>
            </div>
          </td>
          <td>
            <div>
              <input type="number" id="lowflow" name="lowflow">
            </div>
          </td>
        </tr>
        
        <tr>
          <td>
            <div>
              <label for="cooling">Cooling Time:</label>
            </div>
          </td>
          <td>
            <div>
              <input type="number" id="cooling" name="cooling">
            </div>
          </td>
        </tr>

        <tr>
          <td>
            <button class="btn-fire" type="button" onclick="fireInTheHole(); sendValues();">
              Fire
            </button>
          </td>

          <td>
            <div>
              <input type="reset" value="Reset">
            </div>
          </td>
        </tr>
      </table>

      <button class="float" type="button" onclick="stopAllLoads();">
        Stop
      </button>
    </div>

  </form>

</body>

</html>
)=====";

void handleRoot() {
    server.send_P(200, "text/html", webpage);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_TEXT) {
        if (strcmp((char *)payload, "fire") == 0) {
            fireBtnPress = true;
        } else if (strcmp((char *)payload, "stop") == 0) {
            stopBtnPress = true;
        } else {
            char *ptr = (char *)payload;
            char *value;
            for (int i = 0; i < num_pins; i++) {
                value = strtok_r(ptr, ",", &ptr);
                delayValues[i] = (uint16_t)atoi(value);
            }
        }
    }
}

void setup() {
    pinMode(IGNITION_PIN, OUTPUT);
    pinMode(HIGHFLOW_PIN, OUTPUT);
    pinMode(LOWFLOW_PIN, OUTPUT);
    pinMode(COOLING_PIN, OUTPUT);

    Serial.begin(115200);
    delay(100);

    // Start Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    Serial.println("");
    Serial.print("Access Point SSID: ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    webSocket.loop();
    server.handleClient();

    if (fireBtnPress) {
        fireBtnPress = false;
        currentState = 1; // Start the sequence
        loadIndex = 0; // Reset load index
    }

    if (currentState == 1) { // Sequence is ongoing
        unsigned long currentTime = millis();
        unsigned long elapsedTime = currentTime - previousTime;

        if (elapsedTime >= delayValues[loadIndex] * 1000) {
            previousTime = currentTime;
            switch (loadIndex) {
                case 0:
                    digitalWrite(IGNITION_PIN, HIGH);
                    break;
                case 1:
                    digitalWrite(IGNITION_PIN, LOW);
                    digitalWrite(HIGHFLOW_PIN, HIGH);
                    break;
                case 2:
                    digitalWrite(HIGHFLOW_PIN, LOW);
                    digitalWrite(LOWFLOW_PIN, HIGH);
                    break;
                case 3:
                    digitalWrite(LOWFLOW_PIN, LOW);
                    digitalWrite(COOLING_PIN, HIGH);
                    break;
                case 4:
                    digitalWrite(COOLING_PIN, LOW);
                    currentState = 0; // Sequence completed
                    break;
            }
            loadIndex++; // Move to the next load
        }
    }

    if (stopBtnPress) {
        stopBtnPress = false;
        currentState = 0; // Stop the sequence
        digitalWrite(IGNITION_PIN, LOW);
        digitalWrite(HIGHFLOW_PIN, LOW);
        digitalWrite(LOWFLOW_PIN, LOW);
        digitalWrite(COOLING_PIN, LOW);
    }
}
