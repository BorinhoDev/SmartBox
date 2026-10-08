# Testes de componentes – ESP32

Sketches de teste dos componentes do SmartBox, feitos com ESP32 DevKit (30 pinos).

## Testes

| Pasta | O que faz |
|---|---|
| [`hello_world_web`](hello_world_web/) | O ESP32 cria a rede Wi-Fi `ESP32-Hello` (senha `12345678`) e serve uma página "Hello World" em `http://192.168.4.1`. |
| [`teste_teclado_3x4`](teste_teclado_3x4/) | Lê um teclado matricial 3x4 e mostra a tecla pressionada no Monitor Serial (115200). |
| [`teste_servo`](teste_servo/) | Faz um servo MG90S (sinal no GPIO 13) varrer de 0° a 180° e voltar. |

## Como usar

1. Abra o `.ino` da pasta na Arduino IDE (placa: **ESP32 Dev Module**).
2. Instale as bibliotecas pelo Gerenciador de Bibliotecas: **Keypad** (Mark Stanley / Alexander Brevig) para o teclado e **ESP32Servo** (Kevin Harrington) para o servo.
3. Faça o upload e abra o Monitor Serial em 115200.

### Ligação do teclado 3x4

| Fio do teclado | Pino do ESP32 |
|---|---|
| 1 (linha 1) | GPIO 14 |
| 2 (linha 2) | GPIO 27 |
| 3 (linha 3) | GPIO 26 |
| 4 (linha 4) | GPIO 25 |
| 5 (coluna 1) | GPIO 33 |
| 6 (coluna 2) | GPIO 32 |
| 7 (coluna 3) | GPIO 13 |

### Ligação do servo MG90S

| Fio do servo | ESP32 |
|---|---|
| Laranja (sinal) | GPIO 13 |
| Vermelho (+) | VIN (5V) |
| Marrom (GND) | GND |

> O teclado e o servo usam o GPIO 13 nos testes. Para usar os dois juntos, mude um deles de pino (ex.: servo no GPIO 18).
