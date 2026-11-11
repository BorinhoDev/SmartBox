# SmartBox – código principal (ESP32)

Caixa com senha: o teclado 4x3 destrava o servo, o botão da porta trava de novo quando ela fecha, e uma página web mostra tudo ao vivo.

## O que faz

- **Teclado:** a senha tem 4 dígitos. Ao digitar o 4º número, ele confere na hora (destrava ou registra senha errada). `*` ou `#` apagam o que foi digitado.
- **Botão da porta:** depois de destravar, a porta precisa abrir (botão solto). Quando ela fecha e aperta o botão por 0,5 s, o servo trava sozinho.
- **Página web** (`http://smartbox.local`):
  - estado da trava e da porta;
  - os números aparecem enquanto alguém digita;
  - **senha temporária:** gera uma senha aleatória de 4 dígitos (só com 2, 5, 8 e 0) que funciona pelo tempo escolhido (1 a 1440 min) e pode ser cancelada;
  - quadro **Rede**: status da conexão, IP, MAC, sinal (dBm), quedas e hora do ESP32;
  - os 15 eventos mais recentes do log e o link para o log completo (`/log`);
  - botões **Destravar** e **Travar**;
  - troca da senha fixa (exatamente 4 números). A senha fica salva mesmo desligando o ESP32. Padrão: `2580`.
- **LED azul da placa:** pisca enquanto conecta no Wi-Fi; aceso = destravado.
- **Log persistente:** cada evento é gravado na memória flash (LittleFS, arquivo `/log.txt`) como `[INFO]`, `[AVISO]` ou `[ERRO]`, com data e hora pegas por NTP. Acima de 64 KB o arquivo vira `/log_antigo.txt`.
- **Reconexão:** se o Wi-Fi cair, registra `[ERRO]`, tenta reconectar a cada 10 s (`[AVISO]`) e registra `[INFO]` com o IP quando volta.

## Rotas do servidor (HTTP, porta 80)

| Método | Rota | Função |
|---|---|---|
| GET | `/` | Página web |
| GET | `/estado` | JSON com trava, porta, senha temporária, IP, MAC, sinal, Wi-Fi, hora e últimos eventos |
| GET | `/log` | Log completo da flash (texto) |
| POST | `/abrir` | Destrava |
| POST | `/travar` | Trava |
| POST | `/senha` | Troca a senha fixa (`nova`, 4 dígitos) |
| POST | `/temp` | Gera senha temporária (`minutos`, 1 a 1440) |
| POST | `/cancelarTemp` | Cancela a senha temporária |

## Antes de carregar

1. Placa na Arduino IDE: **ESP32 Dev Module**.
2. Bibliotecas: **Keypad** (Mark Stanley / Alexander Brevig) e **ESP32Servo** (Kevin Harrington). WiFi, WebServer, ESPmDNS, LittleFS e Preferences já vêm com o pacote do ESP32.
3. No começo do `smartbox.ino`, troque `NOME_DA_REDE` e `SENHA_AQUI` pelos dados da rede (Wi-Fi **2.4 GHz**).

O ESP32 entra na rede da casa e a página fica em `http://smartbox.local` ou no IP mostrado no Monitor Serial (115200). Os clientes precisam estar na mesma rede.

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

## Referências

Projetos públicos semelhantes, consultados como referência e inspiração para a montagem, a lógica de senha e o fluxo da caixa de entregas. O código deste repositório foi escrito pelo grupo; os links abaixo serviram de base de estudo e comparação.

**Teclado e trava com ESP32**
- [ESP32 – Keypad Door Lock (esp32io)](https://esp32io.com/tutorials/esp32-keypad-door-lock)
- [ESP32 – Door Lock System using Password (esp32io)](https://esp32io.com/tutorials/esp32-door-lock-system-using-password)
- [ESP32 Keypad Database Security System (Hackster)](https://hackster.io/donskytech/esp32-keypad-database-security-system-b4b4af)
- [Exemplo de ESP32 com servo no Wokwi](https://wokwi.com/projects/413054245538808833)

**Trava controlada remotamente**
- [remote-door-lock (GitHub)](https://github.com/vilhelmprytz/remote-door-lock)

**Caixas de recebimento de encomendas**
- [Secure Package Reception Box (Hackster)](https://www.hackster.io/PSoC_Rocks/secure-package-reception-box-d3f382)
- [Automatic Delivery Collection Bot (Hackster)](https://hackster.io/sujata3/automatic-delivery-collection-bot-cd498d)

## Observações

- A página não tem login: qualquer pessoa na mesma rede consegue destravar.
- Se o servo travar/destravar no sentido contrário, troque os valores de `TRAVADO` e `DESTRAVADO`.
