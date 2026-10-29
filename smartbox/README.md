# SmartBox – código principal (ESP32)

Caixa com senha: o teclado 4x3 destrava o servo, o botão da porta trava de novo quando ela fecha, e uma página web mostra tudo ao vivo.

## O que faz

- **Teclado:** digite a senha e aperte `#` para destravar. `*` apaga o que foi digitado.
- **Botão da porta:** depois de destravar, a porta precisa abrir (botão solto). Quando ela fecha e aperta o botão por 0,5 s, o servo trava sozinho.
- **Página web** (`http://smartbox.local`):
  - estado da trava e da porta;
  - os ●●● aparecem enquanto alguém digita;
  - histórico das últimas 15 teclas/eventos;
  - botões **Destravar** e **Travar**;
  - troca da senha (só números, 1 a 8 dígitos). A senha fica salva mesmo desligando o ESP32. Padrão: `1234`.
- **LED azul da placa:** pisca enquanto conecta no Wi-Fi; aceso = destravado.

## Antes de carregar

1. Placa na Arduino IDE: **ESP32 Dev Module**.
2. Bibliotecas: **Keypad** (Mark Stanley / Alexander Brevig) e **ESP32Servo** (Kevin Harrington).
3. No começo do `smartbox.ino`, troque `NOME_DA_REDE` e `SENHA_AQUI` pelos dados da rede (Wi-Fi **2.4 GHz**).

Se não conseguir conectar em 15 s, o ESP32 cria a rede **SmartBox** (senha `12345678`) e a página fica em `http://192.168.4.1`.

## Ligação

### Teclado 4x3 (7 fios)

| Teclas | Pino do ESP32 |
|---|---|
| Linha 1 (1 2 3) | GPIO 32 |
| Linha 2 (4 5 6) | GPIO 33 |
| Linha 3 (7 8 9) | GPIO 25 |
| Linha 4 (* 0 #) | GPIO 26 |
| Coluna 1 (1 4 7 *) | GPIO 27 |
| Coluna 2 (2 5 8 0) | GPIO 14 |
| Coluna 3 (3 6 9 #) | GPIO 13 |

### Servo

| Fio | ESP32 |
|---|---|
| Laranja (sinal) | GPIO 18 |
| Vermelho (+) | VIN (5V) |
| Marrom (−) | GND |

### Botão da porta (4 pernas)

Use duas pernas em **diagonal**: uma no **GPIO 21** e a outra no **GND**. Não precisa de resistor (o código usa o pull-up interno).

## Observações

- A página não tem login: qualquer pessoa na mesma rede consegue destravar.
- Se o servo travar/destravar no sentido contrário, troque os valores de `TRAVADO` e `DESTRAVADO`.
