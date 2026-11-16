// ColdChain - ESP8266 NodeMCU + DHT11 -> API Django REST
// Carte : NodeMCU 1.0 (ESP-12E Module) - Moniteur serie : 115200 bauds
// Bibliotheques : "DHT sensor library" (Adafruit) + "Adafruit Unified Sensor"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <DHT.h>

#define DHTPIN D2        // DATA du DHT11 sur D2 (GPIO4)
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// ====== A MODIFIER ======
const char* WIFI_SSID     = "NOM_DU_WIFI";
const char* WIFI_PASSWORD = "MOT_DE_PASSE";
// IPv4 du PC (ipconfig) + port 8000 + /api/post  (jamais 127.0.0.1 ni localhost)
const char* SERVER_URL    = "http://192.168.1.20:8000/api/post";
// ========================

const unsigned long INTERVALLE_MS = 10000;  // une mesure toutes les 10 s

void connecterWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connexion Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connecte");
  Serial.print("IP de l'ESP8266 : ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(100);
  dht.begin();
  connecterWiFi();
}

void loop() {
  float temperature = dht.readTemperature();
  float humidite    = dht.readHumidity();

  if (isnan(temperature) || isnan(humidite)) {
    Serial.println("Erreur lecture DHT11");
    delay(5000);
    return;
  }

  // Les cles doivent etre EXACTEMENT celles du modele Django : temp et hum
  String json = "{\"temp\":" + String(temperature, 1) +
                ",\"hum\":"  + String(humidite, 1) + "}";

  Serial.print("Temperature : "); Serial.print(temperature); Serial.println(" C");
  Serial.print("Humidite    : "); Serial.print(humidite);    Serial.println(" %");
  Serial.println("JSON envoye : " + json);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi deconnecte, reconnexion...");
    connecterWiFi();
  }

  WiFiClient client;
  HTTPClient http;
  http.begin(client, SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(json);
  Serial.print("Code HTTP : ");
  Serial.println(httpCode);           // 201 = mesure enregistree

  if (httpCode > 0) {
    Serial.println("Reponse : " + http.getString());
  } else {
    Serial.println("Erreur connexion : " + http.errorToString(httpCode));
  }
  http.end();

  Serial.println("------------------------------");
  delay(INTERVALLE_MS);
}
