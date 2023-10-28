#include <microDS18B20.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <stdint.h>

const uint8_t DS_SENSOR_AMOUNT = 2;
const char *ssid = "my-fi";
const char *password = "my-fipass";

// создаём двухмерный массив с адресами датчиков
uint8_t addr[][8] = {
    {0x28, 0xFF, 0x18, 0x8C, 0x54, 0x14, 0x0, 0xF3},
    {0x28, 0xFF, 0x7B, 0x9F, 0x61, 0x15, 0x3, 0xD1},
};

float temps[DS_SENSOR_AMOUNT];

MicroDS18B20<D4, DS_ADDR_MODE> sensor[DS_SENSOR_AMOUNT];
ESP8266WebServer server(80);

void wifiConnect();
void initOTA();
void getTemp();
void restServerRouting();
void handleNotFound();

void setup()
{
  pinMode(A0, INPUT);
  Serial.begin(115200);
  // устанавливаем адреса
  for (int i = 0; i < DS_SENSOR_AMOUNT; i++)
  {
    sensor[i].setAddress(addr[i]);
    sensor[i].setResolution(12, *addr[i]);
  }

  wifiConnect();
  initOTA();
  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  wifiConnect();
  ArduinoOTA.handle();
  server.handleClient();

  static uint32_t tmr;
  if (millis() - tmr >= 1000)
  {
    tmr = millis();

    // выводим показания в порт
    for (int i = 0; i < DS_SENSOR_AMOUNT; i++)
    {
      float t = sensor[i].getTemp();
      Serial.print(t);
      temps[i] = t;
      Serial.print(',');
    }
    Serial.println();

    // запрашиваем новые
    for (int i = 0; i < DS_SENSOR_AMOUNT; i++)
    {
      sensor[i].requestTemp();
    }
  }
}

void restServerRouting()
{
  server.on("/", HTTP_GET, [](){ server.send(200, F("text/html"), F("Welcome to the REST Web Server")); });
  server.on(F("/getTemp"), HTTP_GET, getTemp);
}

// Serving Hello world
void getTemp()
{
  server.send(200, "application/json", "{\"kube\":"+String(temps[0])+",\"extractor\":"+String(temps[1])+"}");
}

// Manage not found URL
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

// OTA Initialization
void initOTA()
{
  ArduinoOTA.setPassword("100500");
  ArduinoOTA.setPort(8266);
  ArduinoOTA.begin();
}

// Wi-Fi
void wifiConnect()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return;
  }

  WiFi.hostname("ESP Distiller Temp");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.printf("Connection status: %d\n", WiFi.status());
  while (WiFi.status() != WL_CONNECTED)
  {
    uint32_t uptime = millis();
    // restart ESP if could not connect during 5 minutes
    if (WiFi.status() != WL_CONNECTED && (uptime > 1000 * 60 * 5))
    {
      ESP.restart();
    }
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, LOW);
  }

  Serial.printf("\nConnection status: %d\n", WiFi.status());
  Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
  Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
  Serial.printf("Hostname: %s\n", WiFi.hostname().c_str());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}