//
// Передатчик через MQTT json строки с весов
// ESP32 Wemos Lolin32
// V1.0 29.12.2023
//

const char* ssid = "WiFi_name";             // WiFi ssid
const char* password = "password";          // WiFi password
const char* mqtt_server = "192.168.1.100";  // MQTT server IP
const char* topic = "scale";                // MQTT topic



#include <WiFi.h>
#include <PubSubClient.h>

#define ledPin 22 	   // LED pin Lolin32 lite
#define ON_pin 12          // Hold power On pin

WiFiClient espClient;
PubSubClient client(espClient);

#define TOP_BUFFER_SIZE 32  // max topic size
#define MSG_BUFFER_SIZE 128 // max message size
char top[TOP_BUFFER_SIZE];
char msg[MSG_BUFFER_SIZE];
uint32_t ID;

inline void LedOn() { digitalWrite(ledPin, LOW); }  // Led On
inline void LedOff(){ digitalWrite(ledPin, HIGH); } // Led Off

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

void MQTT_connect() {
  Serial.print("MQTT connection...");
  client.setServer(mqtt_server, 1883);
  snprintf (top, TOP_BUFFER_SIZE, "%06X", ID);  // Client ID
  uint32_t time = millis();
  while (!client.connected()) {       // Loop until not connected
    if (!client.connect(msg)) {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(500);   // Wait 0.5 seconds before retrying
      if (millis() - time >= 5000) {  // 5sec timeout
        Serial.println("MQTT not connected!");
        digitalWrite(ON_pin, LOW);    // power OFF
        LedOff();
        esp_deep_sleep_start();       // sleep forever
      }
    }
  }
  Serial.println("OK");
}

uint8_t read_serial2() {
  int s_char = '\0';
  uint i = 0;
  uint32_t time = millis();
  while(millis() - time > 5000){  // wait only 5 sec
    if (Serial2.available() > 0) {
      s_char = Serial2.read();
      i++;
      char InputChar[2] = { char(s_char) };
      if ( s_char == '\r' || s_char == '\n') { msg[i] = 0; return i; }
      else { 
        strcat( msg, (char*)InputChar );
//        Serial.print( (char)s_char );
      }
    }
  }
}


void setup() {
  pinMode(ledPin, OUTPUT);
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
}

void loop() {

  Serial2.flush();
  Serial2.setTimeout(5000);
//  uint8_t len = Serial2.readBytesUntil('\n', msg, MSG_BUFFER_SIZE);

  uint8_t len = read_serial2();
  Serial.print("\r\nRx="); Serial.println(len);

  if ( len > 5 ){
    Serial.println(msg);
    if (msg[0] == '{'){
      wifi_cnnect();
      MQTT_connect();
      snprintf (top, TOP_BUFFER_SIZE, "scale/%06X", ID);  // set topic
      snprintf (msg, MSG_BUFFER_SIZE, msg);               // UART2 received string
      Serial.print("Publish: "); Serial.print(top); Serial.print(" "); Serial.println(msg);
      // Send to server
      if(client.publish(top, msg) == true){
        Serial.println("Send OK.");
      } else {
        Serial.println("MQTT send error!");
      }
    }
  }
  delay(500);
  LedOff();
  digitalWrite(ON_pin, LOW); // power OFF
  esp_deep_sleep_start();
}