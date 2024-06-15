#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "driver/dac.h"

const char* ssid = "EcoExplorer";         // Nombre de tu red WiFi
const char* password = "12345678"; // Contraseña de tu red WiFi

const int DAC_CHANNEL = 25;  

void setup() {
  Serial.begin(115200);

  // Conexión a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a WiFi");

  dac_output_enable(DAC_CHANNEL);

  // Llamar a la función para reproducir audio
  reproducirAudio();
}

void loop()
{

}

void reproducirAudio() {
  // Realizar la solicitud HTTP al servidor
  HTTPClient http;
  http.begin("https://text-to-speech-lake.vercel.app/tts");  // Reemplaza con la URL de tu servidor
  
  http.addHeader("Content-Type", "application/json");

  // Datos JSON para el POST
  String jsonData = "{\"text\":\"Hola amigos\"}";

  int httpCode = http.POST(jsonData);

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      Serial.println("Descargando audio...");
      int contentLength = http.getSize();
      Serial.printf("Tamaño del audio: %d bytes\n", contentLength);

      // Allocar buffer para el audio
      uint8_t* audioData = (uint8_t*)malloc(contentLength);
      if (audioData == nullptr) {
        Serial.println("Error al asignar memoria");
        http.end();
        return;
      }

      int bytesRead = http.getStream().readBytes(audioData, contentLength);
      Serial.printf("Bytes leídos: %d\n", bytesRead);
      
      // Reproducir audio a través del DAC con un pequeño retraso entre escrituras
      for (int i = 0; i < bytesRead; ++i) {
        dacWrite(DAC_CHANNEL, audioData[i]); // Ajustar según la amplitud y formato del WAV
        delayMicroseconds(10);  // Añadir un pequeño retraso (ajustar según sea necesario)
      }

      free(audioData);
    } else {
      Serial.printf("Error al realizar la solicitud HTTP: %d\n", httpCode);
    }
  } else {
    Serial.println("Error en la conexión al servidor");
  }

  http.end();
}
