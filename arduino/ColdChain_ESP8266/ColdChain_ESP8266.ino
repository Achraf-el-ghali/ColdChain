// ColdChain - ESP8266 NodeMCU + DHT11 -> API Django REST
// Carte : NodeMCU 1.0 (ESP-12E Module) - Moniteur serie : 115200 bauds
// Bibliotheques : "DHT sensor library" (Adafruit) + "Adafruit Unified Sensor"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>   // connexion HTTPS (site en ligne PythonAnywhere)
#include <DHT.h>

#define DHTPIN D2        // DATA du DHT11 sur D2 (GPIO4)
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// ====== A MODIFIER ======
const char* WIFI_SSID     = "NOM_DU_WIFI";
const char* WIFI_PASSWORD = "MOT_DE_PASSE";
// URL de l'API en ligne (PythonAnywhere). Garder le "/" final apres add.
const char* SERVER_URL    = "https://achraf.pythonanywhere.com/api/add/";
// Ancienne URL (serveur Django sur le PC, meme Wi-Fi) :
// const char* SERVER_URL = "http://10.9.11.141:8000/api/post";

// true  = si le DHT11 ne repond pas, on envoie des valeurs de TEST (demo)
// false = si le DHT11 ne repond pas, on n'envoie rien
const bool MODE_TEST = true;
const float TEMP_TEST = 25.0;
const float HUM_TEST  = 50.0;
// ========================

const unsigned long INTERVALLE_MS = 10000;  // une mesure toutes les 10 s

void connecterWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connexion Wi-Fi a ");
  Serial.print(WIFI_SSID);

  unsigned long debut = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Message d'aide si la connexion prend plus de 20 secondes
    if (millis() - debut > 20000) {
      Serial.println("\nWi-Fi introuvable : verifier nom, mot de passe, reseau 2,4 GHz");
      debut = millis();
    }
  }
  Serial.println("\nWi-Fi connecte");
  Serial.print("IP de l'ESP8266 : ");
  Serial.println(WiFi.localIP());
  Serial.print("Serveur Django   : ");
  Serial.println(SERVER_URL);
  Serial.println("------------------------------");
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("=== ColdChain ESP8266 + DHT11 ===");
  dht.begin();
  connecterWiFi();
}

void loop() {
  float temperature = dht.readTemperature();
  float humidite    = dht.readHumidity();

  // Lecture invalide : NaN, ou 0 / 0 (capteur qui ne repond pas)
  bool lectureInvalide = isnan(temperature) || isnan(humidite) ||
                         (temperature == 0 && humidite == 0);

  if (lectureInvalide) {
    if (MODE_TEST) {
      Serial.println("Erreur lecture DHT11 -> envoi de valeurs de TEST");
      temperature = TEMP_TEST;
      humidite    = HUM_TEST;
    } else {
      Serial.println("Erreur lecture DHT11 (verifier 3V3, GND, DATA sur D2)");
      delay(5000);
      return;
    }
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

  // HTTPS : client securise. setInsecure() = on ne verifie pas le certificat
  // du site (suffisant pour un TP ; la connexion reste chiffree).
  WiFiClientSecure client;
  client.setInsecure();
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
