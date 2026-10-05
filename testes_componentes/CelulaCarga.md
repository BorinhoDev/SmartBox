# Validacao da celula de carga

**Projeto:** SmartBox  
**Plataforma:** ESP32 Dev Module + HX711  

## 1. Objetivo

Documentar a montagem e verificar a leitura de peso da celula de carga usada para identificar encomendas depositadas na SmartBox. Este roteiro cobre inicializacao, tara, estabilidade, repetibilidade e resposta a massas conhecidas.


## 2. Componentes e ligacoes

- Celula de carga de tres fios
- Modulo conversor HX711
- ESP32 Dev Module
- Dois resistores de 1 kOhm para completar a ponte
- Massa de referencia conhecida para calibracao
- Protoboard

### Celula de carga e ponte

```text
Celula de carga                     HX711
Branco                              E+
Preto                               E-
Vermelho (sinal)                    B+

E+ ----[ resistor 1 kOhm ]----+---- B-
                              |
E- ----[ resistor 1 kOhm ]----+
```

### HX711 e ESP32

| HX711 | ESP32 Dev Module |
| ---  | --- |
|  VCC | VIN (5 V) |
|  GND | GND |
|  DT  | GPIO 19 |
|  SCK | GPIO 18 |


## 3. Configuracao e calibracao

1.
  Instalar as Placas ESP32:Acesse Ferramentas (Tools) > Placa (Board) > Gerenciador de Placas... (ou clique no segundo ícone na barra lateral esquerda).
  Digite esp32 na caixa de pesquisa.Localize o pacote esp32 desenvolvido pela Espressif Systems e clique em Instalar.
  Verificação: Ao finalizar o download, a tag INSTALLED (ou Instalado) deve aparecer ao lado do pacote.

2.
  Instalar a Biblioteca do HX711:Acesse Ferramentas (Tools) > Gerenciar Bibliotecas... (atalho Ctrl + Shift + I ou terceiro ícone na barra lateral).
  Digite HX711 na busca.Localize HX711 Arduino Library desenvolvida por Bogdan Necula e clique em Instalar.
  Verificação: A biblioteca constará com a etiqueta INSTALLED, e a linha #include "HX711.h" não apresentará erros de compilação.

## 4. Sketch de leitura

```cpp
#include "HX711.h"

const int PINO_DT = 19;
const int PINO_SCK = 18;

// Substitua pelo fator obtido e validado durante a calibracao.
const float FATOR_ESCALA = 420.0;

HX711 escala;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n--- SmartBox: leitura de peso ---");

  escala.begin(PINO_DT, PINO_SCK);
  escala.set_gain(32);  // Canal B
  escala.set_scale(FATOR_ESCALA);
  escala.tare(10);      // Zera a balanca ao ligar

  Serial.println("Pronto para receber encomendas!\n");
}

void loop() {
  if (escala.is_ready()) {
    float peso_g = escala.get_units(5);

    // Remove pequenas oscilacoes sem ocultar leituras negativas maiores.
    if (peso_g > -2.0 && peso_g < 2.0) {
      peso_g = 0.0;
    }

    Serial.print("Peso na caixa: ");
    Serial.print(peso_g, 1);
    Serial.println(" g");
  } else {
    Serial.println("Aguardando HX711...");
  }

  delay(500);
}
```
