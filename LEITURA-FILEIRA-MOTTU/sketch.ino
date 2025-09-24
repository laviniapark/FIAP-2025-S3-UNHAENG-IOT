#include <WiFi.h>
#include <PubSubClient.h>

// CONFIGURAÇÕES WI-FI
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// CONFIGURAÇÕES MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* topic_events = "fiap/iot/buttons";      // LEFT/RIGHT
const char* topic_cmd    = "fiap/iot/cmd";          // LED_ON / LED_OFF
const char* topic_status = "fiap/iot/status";       // online/offline (LWT)

WiFiClient espClient;
PubSubClient client(espClient);

// PINOS
#define BTN_LEFT 32
#define BTN_RIGHT 22
#define LED 15

bool lastLeft  = true, lastRight = true;
unsigned long lastMsLeft = 0, lastMsRight = 0;
const unsigned DEBOUNCE_MS = 150;

void setup_wifi() {
  delay(10);
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    String cid = "esp32-client-fiap-" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF), HEX);
    Serial.print("Tentando conexão MQTT como "); Serial.println(cid);

    if (client.connect(cid.c_str(), nullptr, nullptr, topic_status, 0, true, "offline")) {
      Serial.println("Conectado ao broker!");
      client.subscribe(topic_cmd);
      client.publish(topic_status, "online", true); // retain
    } else {
      Serial.print("Falhou (rc="); Serial.print(client.state()); Serial.println("), 5s…");
      delay(5000);
    }
  }
}


void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (unsigned i = 0; i < length; i++) msg += (char)message[i];

  if (String(topic) == topic_cmd) {
    if (msg == "LED_ON")  { digitalWrite(LED, HIGH); client.publish(topic_events, "LED_ON_ACK"); }
    if (msg == "LED_OFF") { digitalWrite(LED, LOW);  client.publish(topic_events, "LED_OFF_ACK"); }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // LEFT
  bool nowLeft = digitalRead(BTN_LEFT);
  if (nowLeft != lastLeft && (millis() - lastMsLeft) > DEBOUNCE_MS) {
    lastMsLeft = millis();
    if (nowLeft == LOW) { // borda de descida = pressionado
      client.publish(topic_events, "LEFT");
      Serial.println("LEFT");
    }
    lastLeft = nowLeft;
  }

  // RIGHT
  bool nowRight = digitalRead(BTN_RIGHT);
  if (nowRight != lastRight && (millis() - lastMsRight) > DEBOUNCE_MS) {
    lastMsRight = millis();
    if (nowRight == LOW) {
      client.publish(topic_events, "RIGHT");
      Serial.println("RIGHT");
    }
    lastRight = nowRight;
  }
}
