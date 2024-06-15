
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "driver/dac.h"

const char* ssid = "EcoExplorer";         // Nombre de tu red WiFi
const char* password = "12345678";        // Contraseña de tu red WiFi
const char* serverAddress = "http://192.168.116.57/capture";

const int DAC_CHANNEL = 25;               // Canal DAC

void setup() {
  Serial.begin(115200);

  // Conexión a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a WiFi");

  // Llamar a la función para reproducir audio
  reproducirAudio();
}

void loop() {
  // No se necesita hacer nada en el loop
}

String imgDescription() {
  // Realizar la solicitud HTTP al servidor
  HTTPClient http;
  http.begin(serverAddress);

  int httpResponseCode = http.GET();
  String content = "";

  if (httpResponseCode > 0) {
    String jsonResponse = http.getString();
    // Parsear el JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, jsonResponse);

    if (!error) {
      content = doc["choices"][0]["message"]["content"].as<String>();
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

  return content;
}

int16_t muLawDecode(uint8_t muLawByte) {
  const uint16_t MULAW_MAX = 0x1FFF;
  const uint16_t MULAW_BIAS = 33;

  muLawByte = ~muLawByte;
  int16_t sign = (muLawByte & 0x80) ? -1 : 1;
  int16_t exponent = (muLawByte >> 4) & 0x07;
  int16_t mantissa = muLawByte & 0x0F;
  int16_t sample = sign * ((mantissa << (exponent + 3)) + MULAW_BIAS);

  // Suavizar la muestra dividiendo por 4
  sample = sample / 4;

  return sample;
}

// Filtro de paso bajo
int16_t lowPassFilter(int16_t currentSample, int16_t previousSample, float alpha) {
  return (alpha * currentSample) + ((1.0 - alpha) * previousSample);
}

void reproducirAudio() {
  String text = "Techo de oficina con dos luces encendidas y una esquina de una pared blanca.";
  //String text = imgDescription();

  HTTPClient http;
  // Segunda solicitud HTTP para obtener el audio
  http.begin("https://text-to-speech-lake.vercel.app/tts");  // Reemplaza con la URL de tu servidor
  http.addHeader("Content-Type", "application/json");

  // Datos JSON para el POST
  String jsonData = "{\"text\":\"" + text + "\"}";

  int httpCode = http.POST(jsonData);

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      Serial.println("Descargando audio...");
      int contentLength = http.getSize();
      Serial.printf("Tamaño del audio: %d bytes\n", contentLength);

      // Verificar si el tamaño del audio es válido
      if (contentLength > 0) {
        const int bufferSize = 1024; // Tamaño del buffer para leer el audio en partes
        uint8_t buffer[bufferSize];
        WiFiClient* stream = http.getStreamPtr();

        int16_t previousSample = 0; // Almacenar la muestra anterior para suavizado
        float alpha = 0.1; // Parámetro del filtro de paso bajo

        while (contentLength > 0) {
          int bytesToRead = min(bufferSize, contentLength);
          int bytesRead = stream->readBytes(buffer, bytesToRead);

          if (bytesRead > 0) {
            for (int i = 0; i < bytesRead; ++i) {
              int16_t decodedSample = muLawDecode(buffer[i]);

              // Aplicar el filtro de paso bajo
              int16_t filteredSample = lowPassFilter(decodedSample, previousSample, alpha);
              previousSample = filteredSample;

              dacWrite(DAC_CHANNEL, (filteredSample >> 4) + 128); // Convertir a rango 0-255
              delayMicroseconds(35);  // Ajustar según la frecuencia de muestreo del audio
            }
            contentLength -= bytesRead;
          } else {
            Serial.println("Error al leer el stream de audio");
            break;
          }
        }
      } else {
        Serial.println("Tamaño del audio no válido");
      }
    } else {
      Serial.printf("Error al realizar la solicitud HTTP: %d\n", httpCode);
    }
  } else {
    Serial.println("Error en la conexión al servidor");
  }

  http.end();
}

