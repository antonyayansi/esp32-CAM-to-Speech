
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPIFFS.h>

const char* ssid = "AYANSI";
const char* password = "antonielio";

// Función para calcular el tamaño del buffer necesario para Base64
size_t calculateBase64Length(size_t inputLength) {
    return 4 * ((inputLength + 2) / 3);
}

#include <stdint.h>
#include <string.h>

void encodeBase64(const uint8_t *input, size_t inputLength, char *output) {
    static const char encodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    size_t j = 0;
    for (size_t i = 0; i < inputLength; ) {
        uint32_t octet_a = i < inputLength ? input[i++] : 0;
        uint32_t octet_b = i < inputLength ? input[i++] : 0;
        uint32_t octet_c = i < inputLength ? input[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        output[j++] = encodeTable[(triple >> 3 * 6) & 0x3F];
        output[j++] = encodeTable[(triple >> 2 * 6) & 0x3F];
        output[j++] = encodeTable[(triple >> 1 * 6) & 0x3F];
        output[j++] = encodeTable[(triple >> 0 * 6) & 0x3F];
    }

    // Agregar caracteres de relleno '=' si es necesario
    switch (inputLength % 3) {
        case 1:
            output[j - 1] = '=';
            // no break
        case 2:
            output[j - 2] = '=';
            // no break
        default:
            break;
    }

    output[j] = '\0'; // Asegurar que el arreglo de salida esté terminado con '\0'
}

void setup() {
    Serial.begin(115200);
    delay(100);

    // Inicialización de SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Error al montar el sistema de archivos SPIFFS");
        return;
    }

    // Conexión WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Descargar archivo MP3
    HTTPClient http;
    http.begin("https://youtube.ccore.store/0001.mp3");
    int httpResponseCode = http.GET();

    if (httpResponseCode == HTTP_CODE_OK) {
        // Abrir el archivo para escritura
        File file = SPIFFS.open("/0001.mp3", FILE_WRITE);
        if (!file) {
            Serial.println("Error opening file for writing");
            return;
        }

        // Escribir el cuerpo de la respuesta HTTP en el archivo
        file.write((uint8_t *)http.getStreamPtr(), http.getSize());
        file.close();
        Serial.println("Archivo descargado exitosamente");

        // Leer el archivo MP3
        file = SPIFFS.open("/0001.mp3", "r");
        if (!file) {
            Serial.println("Error al abrir el archivo MP3");
            return;
        }

        // Leer el contenido del archivo en un buffer
        size_t fileSize = file.size();
        uint8_t* fileBuffer = (uint8_t*)malloc(fileSize);
        if (!fileBuffer) {
            Serial.println("Error al asignar memoria para el archivo MP3");
            file.close();
            return;
        }
        file.read(fileBuffer, fileSize);
        file.close();

        // Calcular el tamaño necesario para Base64 y asignar memoria
        size_t base64Size = calculateBase64Length(fileSize) + 1; // +1 para el carácter nulo
        char* base64Buffer = (char*)malloc(base64Size);
        if (!base64Buffer) {
            Serial.println("Error al asignar memoria para Base64");
            free(fileBuffer);
            return;
        }

        // Codificar en Base64
        encodeBase64(fileBuffer, fileSize, base64Buffer);

        // Mostrar el resultado Base64 en la consola serial
        Serial.println(base64Buffer);

        // Liberar la memoria utilizada
        free(fileBuffer);
        free(base64Buffer);

    } else {
        Serial.print("Error en la solicitud HTTP: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}

void loop() {
    // Tu código de bucle aquí
}
