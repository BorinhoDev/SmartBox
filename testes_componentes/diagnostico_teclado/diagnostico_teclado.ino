// Diagnóstico do teclado 4x3: descobre quais GPIOs se ligam ao apertar cada tecla.
// Não importa a ordem dos fios: aperte uma tecla de cada vez e anote o par que aparece.
// Se uma tecla não mostra nada (ou some e volta ao mexer no fio), o problema é contato/fio.

const byte PINOS[] = {32, 33, 25, 26, 27, 14, 13};  // os 7 GPIOs ligados na fita
const byte N = sizeof(PINOS);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Aperte as teclas uma de cada vez...");
}

void loop() {
  for (byte a = 0; a < N; a++) {
    for (byte i = 0; i < N; i++) pinMode(PINOS[i], INPUT_PULLUP);
    pinMode(PINOS[a], OUTPUT);
    digitalWrite(PINOS[a], LOW);
    delayMicroseconds(50);
    for (byte b = a + 1; b < N; b++) {
      if (digitalRead(PINOS[b]) == LOW) {
        Serial.printf("Contato entre GPIO %d e GPIO %d\n", PINOS[a], PINOS[b]);
        delay(250);
      }
    }
  }
}
