/********************************************************
 * Solar Heater Controller
 * 
 * Copyright (C) 2017 Darren Poulson <darren.poulson@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ********************************************************/

#include <config.h>

#include <PID_v1.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SimpleTimer.h>
#include <PubSubClient.h>

#define TEMP_INT 0
#define TEMP_EXT 1
#define TEMP_TOP 2
#define RELAY_PIN 6

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
double Kp=2, Ki=5, Kd=1;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

float temp_int, temp_ext, temp_top;

boolean relay_state;

int WindowSize = 5000;
unsigned long windowStartTime;

char buf[10];
String value;
char mptt_location[16];

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }  
    // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(OTA_HOSTNAME);

  // No authentication by default
  //ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  client.setServer(mqtt_server, 1883);

  windowStartTime = millis();

  //initialize the variables we're linked to
  Setpoint = 20;

  //tell the PID to range between 0 and the full window size
  myPID.SetOutputLimits(0, WindowSize);

  //turn the PID on
  myPID.SetMode(AUTOMATIC);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("SolarHeater1")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop()
{
  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
  }
  
  // Read in temperatures
  digitalWrite(TEMP_INT, HIGH);
  delay(100);
  temp_int = ((analogRead(A0)*(1000/1023))-500)/10
  digitalWrite(TEMP_INT, LOW);
  digitalWrite(TEMP_EXT, HIGH);
  delay(100);
  temp_ext = ((analogRead(A0)*(1000/1023))-500)/10
  digitalWrite(TEMP_EXT, LOW);
  digitalWrite(TEMP_TOP, HIGH);
  delay(100);
  temp_top = ((analogRead(A0)*(1000/1023))-500)/10
  digitalWrite(TEMP_TOP, LOW);
  Input = temp_int;
  myPID.Compute();

  /************************************************
   * turn the output pin on/off based on pid output
   ************************************************/
  if (millis() - windowStartTime > WindowSize)
  { //time to shift the Relay Window
    windowStartTime += WindowSize;
  }
  if (Output < millis() - windowStartTime) {
    digitalWrite(RELAY_PIN, HIGH);
    relay_state=true;
  }
  else {
    digitalWrite(RELAY_PIN, LOW);
    relay_state=false;
  }

  // Send values to MQTT server for home automation
  client.publish("SolarHeater/1/temp_int", temp_int);
  client.publish("SolarHeater/1/temp_ext", temp_ext);
  client.publish("SolarHeater/1/temp_top", temp_top);
  client.publish("SolarHeater/1/relay_state", relay_state);

  client.loop();
}



