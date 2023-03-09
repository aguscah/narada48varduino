#include "config.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "SimpleTimer.h"
#include <ModbusMaster.h>
#include <PubSubClient.h>
#include<SoftwareSerial.h>


const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWD";
const char* mqtt_server = "MQTT_IP_ADDRESS";
const char* mqtt_user = "MQTT_USER";
const char* mqtt_pass = "MQTT_PASS";

#define OTA_HOSTNAME                    "BMS_NARADA"
#define MQTT_ROOT                       "BMSNARADA"


/* Hardware Serial2 is only available on certain boards.
   For example the Arduino MEGA 2560
*/
SoftwareSerial RS485(12, 14);

bool loadstate;
float ctemp, bvoltage, loadvoltage, battChargeCurrent, btemp,heattemp2 , bremaining, lpower, lcurrent, pvvoltage, pvcurrent, pvpower, battemp, maxpvvoltage, maxbatvoltage, kwhtoday, kwhmonth, rembattemp, htemp,heattemp;
uint8_t charger_mode  = 0;
uint8_t charger_mode1  = 0;
uint8_t temp;
char charger_charging_status[][12] = {
    "Off",
    "Float",
    "Boost",
    "Equlization"
};

int timerTask2;
char buf[12];
uint8_t result;

WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;
ModbusMaster node;
ModbusMaster node1;
ModbusMaster node2;
ModbusMaster node3;
ModbusMaster nodeepever;

void preTransmission() {}
void postTransmission() {}


void setup() {
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();


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

  int addr = 39;


  timerTask2 = timer.setInterval(4000, readBMS);


}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("BMSNARADA", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...

    } else {
      Serial.println("MQQT NOT connected");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void readBMS() {
  RS485.begin(9600);

  node.begin(39, RS485);
  node2.begin(40, RS485);

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  
  node2.preTransmission(preTransmission);
  node2.postTransmission(postTransmission);

  
  float battVoltage,  battCurrent, buff, kwh, allVolt,avg_volt;
  float  batCurrent, batCapAh, batSOC, batSOH,batCC, cel_1, cel_2, cel_3, cel_4, cel_5, cel_6, cel_7, cel_8, cel_9, cel_10, cel_11, cel_12, cel_13, cel_14, cel_15, all_ah, all_soc, all_ampere, avg_soc;
  int j;

  int c = 0;
  ///////////// UNTUK BMS 1  - Tahun 2020 kebawah
  node.clearResponseBuffer();
  delay(100);
  result = node.readInputRegisters(0x0fff, 10 );
  if (result == node.ku8MBSuccess) {
    Serial.println("");
    c++;
    battVoltage = node.getResponseBuffer(0) / 100.0f;
    batCurrent = ((node.getResponseBuffer(1) - 10000) * -1) / 10.0f;
    batCapAh = node.getResponseBuffer(2) / 10.0f ;
    batSOC = node.getResponseBuffer(8) / 100.0f;
    batSOH = node.getResponseBuffer(9) / 100.0f;
    allVolt +=battVoltage;
    all_soc += batSOC;
    all_ah += batCapAh;
    all_ampere += batCurrent;
    dtostrf(battVoltage, 2, 2, buf );
    client.publish("BMSNARADA/1/battVoltage", buf);
    dtostrf(batCurrent, 3, 2, buf );
    client.publish("BMSNARADA/1/batCurrent", buf);
    dtostrf(batCapAh, 2, 2, buf );
    client.publish("BMSNARADA/1/batCapAh", buf);
    dtostrf(batSOC, 2, 2, buf );
    client.publish("BMSNARADA/1/batSOC", buf);
    dtostrf(batSOH, 3, 0, buf );
    client.publish("BMSNARADA/1/batSOH", buf);

    Serial.print("battVoltage_1 : "); Serial.println(battVoltage);
    Serial.print("batCurrent_1 : "); Serial.println(batCurrent);
    Serial.print("batCapAh_1 : "); Serial.println(batCapAh);
    Serial.print("batSOC_1 : "); Serial.println(batSOC);
    Serial.print("batSOH_1 : "); Serial.println(batSOH);
  } else {
    Serial.println("Read register 0x1000 failed!");
  }

  delay(50);
  result = node.readInputRegisters(0x1009, 10 );
  if (result == node.ku8MBSuccess) {
    Serial.println("");
    cel_1 = node.getResponseBuffer(3) / 1000.0f;
    cel_2 = node.getResponseBuffer(4) / 1000.0f;
    cel_3 = node.getResponseBuffer(5) / 1000.0f;
    cel_4 = node.getResponseBuffer(6) / 1000.0f;
    cel_5 = node.getResponseBuffer(7) / 1000.0f;
    cel_6 = node.getResponseBuffer(8) / 1000.0f;
    cel_7 = node.getResponseBuffer(9) / 1000.0f;
    dtostrf(cel_1, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_1", buf);
    dtostrf(cel_2, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_2", buf);
    dtostrf(cel_3, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_3", buf);
    dtostrf(cel_4, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_4", buf);
    dtostrf(cel_5, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_5", buf);
    dtostrf(cel_6, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_6", buf);
    dtostrf(cel_7, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_7", buf);
    Serial.print("cel_1 : "); Serial.println(cel_1);
    Serial.print("cel_2 : "); Serial.println(cel_2);
    Serial.print("cel_3 : "); Serial.println(cel_3);
    Serial.print("cel_4 : "); Serial.println(cel_4);
    Serial.print("cel_5 : "); Serial.println(cel_5);
    Serial.print("cel_6 : "); Serial.println(cel_6);
    Serial.print("cel_7 : "); Serial.println(cel_7);
  } else {
    Serial.println("Read register 0x1009 failed!");
  }

  delay(50);
  result = node.readInputRegisters(0x1013, 10 );
  if (result == node.ku8MBSuccess) {
    cel_8 = node.getResponseBuffer(0) / 1000.0f;
    cel_9 = node.getResponseBuffer(1) / 1000.0f;
    cel_10 = node.getResponseBuffer(2) / 1000.0f;
    cel_11 = node.getResponseBuffer(3) / 1000.0f;
    cel_12 = node.getResponseBuffer(4) / 1000.0f;
    cel_13 = node.getResponseBuffer(5) / 1000.0f;
    cel_14 = node.getResponseBuffer(6) / 1000.0f;
    cel_15 = node.getResponseBuffer(7) / 1000.0f;
    dtostrf(cel_8, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_8", buf);
    dtostrf(cel_9, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_9", buf);
    dtostrf(cel_10, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_10", buf);
    dtostrf(cel_11, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_11", buf);
    dtostrf(cel_12, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_12", buf);
    dtostrf(cel_13, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_13", buf);
    dtostrf(cel_14, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_14", buf);
    dtostrf(cel_15, 1, 3, buf );
    client.publish("BMSNARADA/1/cel_15", buf);

    Serial.print("cel_8 : "); Serial.println(cel_8);
    Serial.print("cel_9 : "); Serial.println(cel_9);
    Serial.print("cel_10 : "); Serial.println(cel_10);
    Serial.print("cel_11 : "); Serial.println(cel_11);
    Serial.print("cel_12 : "); Serial.println(cel_13);
    Serial.print("cel_13 : "); Serial.println(cel_13);
    Serial.print("cel_14 : "); Serial.println(cel_14);
    Serial.print("cel_15 : "); Serial.println(cel_15);
  } else {
    Serial.println("Read register 0x1014 failed!");
  }



  ///////////// UNTUK BMS 3 - Tahun 2020 keatas
  client.publish("EPSolar/info2", "Check NARADA-3");
  node2.clearResponseBuffer();
  delay(100);
  result = node2.readInputRegisters(0x0fff, 32 );
  if (result == node2.ku8MBSuccess) {
    Serial.println("");
    c++;
    battVoltage = node2.getResponseBuffer(0) / 100.0f;
    batCurrent = ((node2.getResponseBuffer(1) - 10000) ) / 10.0f;
    batCapAh = node2.getResponseBuffer(2) / 10.0f ;
    batSOC = node2.getResponseBuffer(8) / 100.0f;
    batCC = node2.getResponseBuffer(9);
    batSOH = node2.getResponseBuffer(10) / 100.0f;
    allVolt +=battVoltage;
    all_soc += batSOC;
    all_ah += batCapAh;
    all_ampere += batCurrent;
    dtostrf(node2.getResponseBuffer(1), 5, 0, buf );
    client.publish("BMSNARADA/3/tes1", buf);
 
    dtostrf(batCC, 3, 0, buf );
    client.publish("BMSNARADA/3/battCycle", buf);
    dtostrf(battVoltage, 2, 2, buf );
    client.publish("BMSNARADA/3/battVoltage", buf);
    dtostrf(batCurrent, 3, 2, buf );
    client.publish("BMSNARADA/3/batCurrent", buf);
    dtostrf(batCapAh, 2, 2, buf );
    client.publish("BMSNARADA/3/batCapAh", buf);
    dtostrf(batSOC, 2, 2, buf );
    client.publish("BMSNARADA/3/batSOC", buf);
    dtostrf(batSOH, 3, 0, buf );
    client.publish("BMSNARADA/3/batSOH", buf);

    Serial.print("battVoltage_2 : "); Serial.println(battVoltage);
    Serial.print("batCurrent_2 : "); Serial.println(batCurrent);
    Serial.print("batCapAh_2 : "); Serial.println(batCapAh);
    Serial.print("batSOC_2 : "); Serial.println(batSOC);
    Serial.print("batSOH_2 : "); Serial.println(batSOH);

    cel_1 = node2.getResponseBuffer(14) / 1000.0f;
    cel_2 = node2.getResponseBuffer(15) / 1000.0f;
    cel_3 = node2.getResponseBuffer(16) / 1000.0f;
    cel_4 = node2.getResponseBuffer(17) / 1000.0f;
    cel_5 = node2.getResponseBuffer(18) / 1000.0f;
    cel_6 = node2.getResponseBuffer(19) / 1000.0f;
    cel_7 = node2.getResponseBuffer(20) / 1000.0f;
    cel_8 = node2.getResponseBuffer(21) / 1000.0f;
    cel_9 = node2.getResponseBuffer(22) / 1000.0f;
    cel_10 = node2.getResponseBuffer(23) / 1000.0f;
    cel_11 = node2.getResponseBuffer(24) / 1000.0f;
    cel_12 = node2.getResponseBuffer(25) / 1000.0f;
    cel_13 = node2.getResponseBuffer(26) / 1000.0f;
    cel_14 = node2.getResponseBuffer(27) / 1000.0f;
    cel_15 = node2.getResponseBuffer(28) / 1000.0f;

    dtostrf(cel_1, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_1", buf);
    dtostrf(cel_2, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_2", buf);
    dtostrf(cel_3, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_3", buf);
    dtostrf(cel_4, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_4", buf);
    dtostrf(cel_5, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_5", buf);
    dtostrf(cel_6, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_6", buf);
    dtostrf(cel_7, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_7", buf);
    dtostrf(cel_8, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_8", buf);
    dtostrf(cel_9, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_9", buf);
    dtostrf(cel_10, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_10", buf);
    dtostrf(cel_11, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_11", buf);
    dtostrf(cel_12, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_12", buf);
    dtostrf(cel_13, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_13", buf);
    dtostrf(cel_14, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_14", buf);
    dtostrf(cel_15, 1, 2, buf );
    client.publish("BMSNARADA/3/cel_15", buf);

    Serial.print("cel_1 : "); Serial.println(cel_1);
    Serial.print("cel_2 : "); Serial.println(cel_2);
    Serial.print("cel_3 : "); Serial.println(cel_3);
    Serial.print("cel_4 : "); Serial.println(cel_4);
    Serial.print("cel_5 : "); Serial.println(cel_5);
    Serial.print("cel_6 : "); Serial.println(cel_6);
    Serial.print("cel_7 : "); Serial.println(cel_7);
    Serial.print("cel_8 : "); Serial.println(cel_8);
    Serial.print("cel_9 : "); Serial.println(cel_9);
    Serial.print("cel_10 : "); Serial.println(cel_10);
    Serial.print("cel_11 : "); Serial.println(cel_11);
    Serial.print("cel_12 : "); Serial.println(cel_13);
    Serial.print("cel_13 : "); Serial.println(cel_13);
    Serial.print("cel_14 : "); Serial.println(cel_14);
    Serial.print("cel_15 : "); Serial.println(cel_15);




  } else {
    Serial.println("Read register BMS-2 0x1000 failed!");
  }



}

void loop() {


  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  timer.run();

}
