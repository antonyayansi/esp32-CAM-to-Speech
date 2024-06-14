#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "AYANSI";
const char* password = "antonielio";
const char* serverAddress = "http://192.168.18.31/capture";

void setup() {
    Serial.begin(115200);
    delay(100);

    // Conectar a la red WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Conectando a WiFi...");
    }
    Serial.println("Conectado a WiFi");

    // Realizar solicitud HTTP
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverAddress);

        int httpResponseCode = http.GET();

        if (httpResponseCode > 0) {
            String jsonResponse = http.getString();
            // Parsear el JSON
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, jsonResponse);

            if (!error) {
                const char* content = doc["choices"][0]["message"]["content"];
                Serial.println("Contenido del campo 'content':");
                Serial.println(content);
            } else {
                Serial.print("Error al parsear JSON: ");
                Serial.println(error.c_str());
            }
        } else {
            Serial.print("Error en la solicitud HTTP: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    } else {
        Serial.println("Error al conectar a WiFi");
    }
}

void loop() {
    // Aquí puedes añadir más funcionalidad si es necesario
}
