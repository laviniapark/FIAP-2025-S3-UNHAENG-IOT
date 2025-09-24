#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define DEVICE_ID "esp01"

const char* mqtt_server = "broker.hivemq.com"
const int   mqtt_port   = 1883;

String topic_events  = String("unhaeng/iot/") + DEVICE_ID + "/events";   // ENTRY/EXIT (ou LEFT/RIGHT)
String topic_cmd     = String("unhaeng/iot/") + DEVICE_ID + "/cmd";      // LED_ON/OFF
String topic_status  = String("unhaeng/iot/") + DEVICE_ID + "/status";   // online/offline (LWT)
String topic_metrics = String("unhaeng/iot/") + DEVICE_ID + "/metrics";  // heartbeat, rssi, led

WiFiClient espClient;
PubSubClient client(espClient);

#define BTN_LEFT  32
#define BTN_RIGHT 22
#define LED       15

// Debounce
bool lastLeft  = true, lastRight = true;
unsigned long lastMsLeft = 0, lastMsRight = 0;
const unsigned DEBOUNCE_MS = 150;

// Heartbeat
unsigned long lastBeat = 0;

void setup_wifi() {
  delay(10);
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    String cid = String("unhaeng-") + DEVICE_ID + "-" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF), HEX);
    Serial.print("Tentando conexão MQTT como "); Serial.println(cid);

    // LWT: se cair, broker mantém "offline" retido em topic_status
    if (client.connect(
          cid.c_str(),
          nullptr, nullptr,
          topic_status.c_str(), // <-- precisa .c_str()
          0, true,
          "offline"
        )) {
      Serial.println("Conectado ao broker!");
      // publica "online" retido
      client.publish(topic_status.c_str(), "online", true);
      // assina comandos após conectar
      client.subscribe(topic_cmd.c_str()); // <-- precisa .c_str()
    } else {
      Serial.print("Falhou (rc="); Serial.print(client.state()); Serial.println("). Retentando em 5s...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (unsigned i = 0; i < length; i++) msg += (char)message[i];

  Serial.print("Mensagem recebida ["); Serial.print(topic); Serial.print("]: ");
  Serial.println(msg);

  if (String(topic) == topic_cmd) {
    if (msg == "LED_ON")  { digitalWrite(LED, HIGH); client.publish(topic_events.c_str(), "LED_ON_ACK"); }
    if (msg == "LED_OFF") { digitalWrite(LED, LOW);  client.publish(topic_events.c_str(), "LED_OFF_ACK"); }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(BTN_LEFT,  INPUT_PULLUP); // botão -> GND
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  // captura estado inicial para não gerar evento falso
  lastLeft  = digitalRead(BTN_LEFT);
  lastRight = digitalRead(BTN_RIGHT);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  Serial.println("ESP32 iniciado. Aguardando conexão MQTT...");
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  //HEARTBEAT a cada 5s
  if (millis() - lastBeat > 5000) {
    lastBeat = millis();
    long rssi = WiFi.RSSI();
    String json = String("{\"uptime\":") + (millis()/1000) +
                  ",\"rssi\":" + rssi +
                  ",\"led\":"  + (digitalRead(LED) == HIGH ? 1 : 0) + "}";
    client.publish(topic_metrics.c_str(), json.c_str());
    Serial.print("HB -> "); Serial.println(json);
  }

  //LEFT (ENTRY)
  bool nowLeft = digitalRead(BTN_LEFT);
  if (nowLeft != lastLeft && (millis() - lastMsLeft) > DEBOUNCE_MS) {
    lastMsLeft = millis();
    if (nowLeft == LOW) { // pressionado (PULLUP)
      client.publish(topic_events.c_str(), "LEFT"); // ou "ENTRY"
      Serial.println("LEFT");
    }
    lastLeft = nowLeft;
  }

  //RIGHT (EXIT)
  bool nowRight = digitalRead(BTN_RIGHT);
  if (nowRight != lastRight && (millis() - lastMsRight) > DEBOUNCE_MS) {
    lastMsRight = millis();
    if (nowRight == LOW) {
      client.publish(topic_events.c_str(), "RIGHT"); // ou "EXIT"
      Serial.println("RIGHT");
    }
    lastRight = nowRight;
  }
}
