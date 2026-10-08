#include <ESP32Servo.h>

Servo meuServo;      // Cria o objeto para controlar o servo
int pinoServo = 13;  // Pino GPIO onde o fio de sinal (Laranja) está conectado

void setup() {
  Serial.begin(115200);

  // Aloca os timers PWM (necessário para o ESP32 funcionar corretamente com servos)
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // A frequência padrão de servos como o MG90S é 50Hz
  meuServo.setPeriodHertz(50); 

  // Anexa o pino do servo. 
  // Valores 500 e 2400 são as larguras de pulso mín/máx (em microssegundos) comuns para o MG90S.
  meuServo.attach(pinoServo, 500, 2400); 

  Serial.println("Servo iniciado. Testando varredura...");
}

void loop() {
  int pos;

  // Gira o servo de 0 a 180 graus (1 grau por vez)
  for (pos = 0; pos <= 180; pos += 1) {
    meuServo.write(pos);
    delay(15); // Pequena pausa para dar tempo do motor alcançar a posição
  }

  delay(500); // Pausa meio segundo nos extremos

  // Gira o servo de 180 de volta a 0 graus
  for (pos = 180; pos >= 0; pos -= 1) {
    meuServo.write(pos);
    delay(15); 
  }

  delay(500);
}
