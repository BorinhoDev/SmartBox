#include <Keypad.h>

// Dimensões do teclado
const byte LINHAS = 4;
const byte COLUNAS = 3;

// Mapeamento das teclas
char mapaTeclas[LINHAS][COLUNAS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

// Pinos correspondentes no ESP32
byte pinosLinhas[LINHAS]   = {14, 27, 26, 25}; // Pinos 1, 2, 3 e 4 da fita
byte pinosColunas[COLUNAS] = {33, 32, 13};     // Pinos 5, 6 e 7 da fita

// Inicializa a instância do teclado
Keypad teclado = Keypad(makeKeymap(mapaTeclas), pinosLinhas, pinosColunas, LINHAS, COLUNAS);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("======================================");
  Serial.println("   TESTE DE TECLADO MATRICIAL 3X4     ");
  Serial.println("   Pressione qualquer tecla...        ");
  Serial.println("======================================");
}

void loop() {
  char teclaPressionada = teclado.getKey();

  if (teclaPressionada) {
    Serial.print("Tecla detectada: [ ");
    Serial.print(teclaPressionada);
    Serial.println(" ]");
  }
}
