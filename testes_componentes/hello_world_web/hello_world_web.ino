// ESP32 servindo uma página "Hello World"
// Conecte o celular na rede "ESP32-Hello" (senha 12345678) e abra http://192.168.4.1

#include <WiFi.h>
#include <WebServer.h>

WebServer servidor(80);

void paginaInicial() {
  servidor.send(200, "text/html; charset=utf-8",
    "<!doctype html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>ESP32</title></head>"
    "<body style='font-family:sans-serif;display:grid;place-items:center;height:100vh;margin:0'>"
    "<h1>Hello World</h1></body></html>");
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP("ESP32-Hello", "12345678");
  Serial.print("Abra no navegador: http://");
  Serial.println(WiFi.softAPIP());

  servidor.on("/", paginaInicial);
  servidor.begin();
}

void loop() {
  servidor.handleClient();
}
