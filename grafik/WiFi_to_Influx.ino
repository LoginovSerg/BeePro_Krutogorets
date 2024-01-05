//
// json строку с весов сохраняем в Influxdb
// ESP32 Wemos Lolin32
// V1.0 03.01.2024
//

const char* ssid = "";             // WiFi ssid
const char* password = "";         // WiFi password
const char* serverIP = "192.168.1.100";     // Server IP
const char* baseName = "Base1";             // Influxdb base name

const char* tag = "Scales";

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define ON_pin 12          // Hold power On pin

WiFiClient espClient;

#define TOP_BUFFER_SIZE 64  // max topic size
#define MSG_BUFFER_SIZE 256 // max message size
char top[TOP_BUFFER_SIZE];
char msg[MSG_BUFFER_SIZE];
uint32_t ID;

inline void LedOn() { digitalWrite(LED_BUILTIN, LOW); }  // Led On
inline void LedOff(){ digitalWrite(LED_BUILTIN, HIGH); } // Led Off

void wifi_cnnect() {
  WiFi.mode(WIFI_STA);
  snprintf (top, TOP_BUFFER_SIZE, "ESP32_%06X", ID); // define hostname
  WiFi.setHostname(top);
  WiFi.begin(ssid, password);
  uint32_t time = millis();
  while (WiFi.status() != WL_CONNECTED) {
    LedOff(); delay(50);
    LedOn();  delay(50);
    if (millis() - time >= 5000) {  // 5sec timeout
      Serial.println("\n\rWiFi not connected!");
      digitalWrite(ON_pin, LOW);    // power OFF
      LedOff();
      esp_deep_sleep_start();       // sleep forever
    }
  }
  Serial.print("IP: "); Serial.print(WiFi.localIP());
  Serial.print(", RSSI: "); Serial.println(WiFi.RSSI());
}

uint8_t read_serial2() {
int s_char;
uint8_t i = 0;
uint32_t time = millis();
Serial.print("Wait...");

  while(millis()-time < 5000){  // wait only 5 sec
    if (Serial2.available() > 0) {
      s_char = Serial2.read();
      if ( s_char == '\r' || s_char == '\n') { msg[i] = 0; return i; }
      else { 
        msg[i] = (uint8_t)s_char; 
        i++;
      }
    }
  }
Serial.println("end.");
  return i;
}



void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  LedOn();
  pinMode(ON_pin, OUTPUT);
  digitalWrite(ON_pin, HIGH);        // Hold power On
  Serial.begin(115200);
  Serial.println("\r\nStart...");
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  uint64_t macAddress = ESP.getEfuseMac();
  Serial.printf("MAC:%012llX\n", macAddress);
  ID = macAddress >> 24;
  ID = ((ID&0x0000FF)<<16)|(ID&0x00FF00)|((ID&0xFF0000)>>16); // swap bytes. MAC записан в обратном порядке
  Serial.printf("ID:%06X\n", ID);

  snprintf (top, TOP_BUFFER_SIZE, "http://%s:8086/write?db=%s",serverIP ,baseName);
  Serial.print("serverName=");
  Serial.println(top);

}

void loop() {
  StaticJsonDocument<200> doc;

  uint8_t len = read_serial2();
//  strcpy(msg, "{\"TIME\":\"13:17\",\"T\":21.9,\"RH\":29,\"WEIGHT\":0.018,\"W2\":0.018,\"V\":3.948}");
//  int len = strlen(msg);
  Serial.print("Rx="); Serial.println(len);

  if ( len > 5 ){
    Serial.println(msg);
    DeserializationError error = deserializeJson(doc, msg); // parse json
    if (error) {  // Test if parsing succeeds.
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    float T = doc["T"];
    int RH = doc["RH"];
    float WEIGHT = doc["WEIGHT"];
    float W2 = doc["W2"];
    float V = doc["V"];

    Serial.print("T="); Serial.println(T);
    Serial.print("RH="); Serial.println(RH);
    Serial.print("WEIGHT="); Serial.println(WEIGHT);
    Serial.print("W2="); Serial.println(W2);
    Serial.print("V="); Serial.println(V);

    len = snprintf (msg, MSG_BUFFER_SIZE, "%s,DEV=%06X T=%.1f\n", tag, ID, T);
    len += snprintf (msg+len, MSG_BUFFER_SIZE, "%s,DEV=%06X RH=%d\n", tag, ID, RH);
    len += snprintf (msg+len, MSG_BUFFER_SIZE, "%s,DEV=%06X WEIGHT=%.3f\n", tag, ID, WEIGHT);
    len += snprintf (msg+len, MSG_BUFFER_SIZE, "%s,DEV=%06X RH=%.3f\n", tag, ID, W2);
    len += snprintf (msg+len, MSG_BUFFER_SIZE, "%s,DEV=%06X V=%.3f", tag, ID, V);
    Serial.print("msg="); Serial.println(msg);

    // Send to server
    wifi_cnnect();
    if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;
      snprintf (top, TOP_BUFFER_SIZE, "http://%s:8086/write?db=%s", serverIP, baseName);
      Serial.print("serverName="); Serial.println(top);
      http.begin(client, top);
      http.addHeader("Content-Type", "text/plain");         // Specify content-type header
      int httpResponseCode = http.POST((uint8_t*)msg, len); // Send HTTP POST request
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      http.end(); // Free resources
    }
  }
Serial.print("Finish:"); Serial.println( millis() );
  delay(500);
  LedOff();
  digitalWrite(ON_pin, LOW); // power OFF
  esp_deep_sleep_start();
}
