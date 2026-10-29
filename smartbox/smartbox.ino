#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Keypad.h>
#include <ESP32Servo.h>

// ---- Wi-Fi (coloque o nome e a senha da sua rede 2.4 GHz) ----
const char* WIFI_NOME  = "NOME_DA_REDE";
const char* WIFI_SENHA = "SENHA_AQUI";

// ---- Teclado 4x3 (ordem dos pinos descoberta no teste) ----
const byte LINHAS = 4, COLUNAS = 3;
char teclas[LINHAS][COLUNAS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte pinosLinhas[LINHAS]   = {32, 33, 25, 26};
byte pinosColunas[COLUNAS] = {27, 14, 13};
Keypad teclado = Keypad(makeKeymap(teclas), pinosLinhas, pinosColunas, LINHAS, COLUNAS);

// ---- Servo, botão e LED ----
Servo trava;
const int PINO_SERVO = 18;
const int PINO_BOTAO = 21;
const int PINO_LED   = 2;
const int TRAVADO = 0, DESTRAVADO = 90;

WebServer server(80);
Preferences prefs;

String senha = "1234";
String digitado = "";
bool destravado = false;
bool portaFoiAberta = false;
unsigned long inicioAperto = 0;

// ---- Histórico de eventos (últimos 15) ----
const int MAX_EV = 15;
String eventos[MAX_EV];
int totalEv = 0;

void registrar(String msg) {
  unsigned long s = millis() / 1000;
  char tempo[12];
  sprintf(tempo, "%02lu:%02lu", s / 60, s % 60);
  String linha = String(tempo) + " - " + msg;
  eventos[totalEv % MAX_EV] = linha;
  totalEv++;
  Serial.println(linha);
}

bool portaFechada() {
  return digitalRead(PINO_BOTAO) == LOW;
}

void travar(String origem) {
  trava.write(TRAVADO);
  destravado = false;
  portaFoiAberta = false;
  digitalWrite(PINO_LED, LOW);
  registrar("Travado (" + origem + ")");
}

void destravar(String origem) {
  trava.write(DESTRAVADO);
  destravado = true;
  portaFoiAberta = false;
  digitalWrite(PINO_LED, HIGH);
  registrar("Destravado (" + origem + ")");
}

void verificarPorta() {
  if (!destravado) return;

  if (!portaFechada()) {
    if (!portaFoiAberta) registrar("Porta aberta");
    portaFoiAberta = true;
    inicioAperto = 0;
  }
  else if (portaFoiAberta) {
    if (inicioAperto == 0) inicioAperto = millis();
    if (millis() - inicioAperto > 500) {
      registrar("Porta fechada");
      travar("porta");
    }
  }
}

// ---- Página web ----
const char PAGINA[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>SmartBox</title>
<style>
  body { font-family: Arial, sans-serif; background:#111827; color:#f3f4f6; margin:0; padding:16px; }
  .card { background:#1f2937; border-radius:12px; padding:16px; max-width:420px; margin:0 auto 14px; }
  h1 { text-align:center; margin:8px 0 16px; }
  h2 { font-size:16px; margin:0 0 10px; color:#9ca3af; }
  .status { font-size:26px; font-weight:bold; text-align:center; padding:12px; border-radius:8px; }
  .travada { background:#7f1d1d; }
  .aberta  { background:#14532d; }
  .info { text-align:center; margin-top:8px; color:#d1d5db; }
  .pontos { font-size:30px; text-align:center; letter-spacing:6px; min-height:40px; }
  .botoes { display:flex; gap:10px; }
  button { flex:1; padding:14px; font-size:16px; border:0; border-radius:8px; cursor:pointer; color:white; }
  .verde { background:#16a34a; } .vermelho { background:#dc2626; } .azul { background:#2563eb; }
  input { width:100%; box-sizing:border-box; padding:12px; font-size:18px; border-radius:8px; border:0; margin-bottom:10px; }
  ul { list-style:none; padding:0; margin:0; font-family:monospace; font-size:14px; }
  li { padding:4px 0; border-bottom:1px solid #374151; }
  #msg { text-align:center; margin-top:8px; min-height:20px; }
</style>
</head>
<body>
<h1>SmartBox</h1>

<div class="card">
  <div id="status" class="status travada">...</div>
  <div class="info">Porta: <b id="porta">...</b></div>
</div>

<div class="card">
  <h2>Digitando no teclado</h2>
  <div class="pontos" id="pontos"></div>
</div>

<div class="card">
  <h2>Controle</h2>
  <div class="botoes">
    <button class="verde" onclick="enviar('/abrir')">Destravar</button>
    <button class="vermelho" onclick="enviar('/travar')">Travar</button>
  </div>
</div>

<div class="card">
  <h2>Nova senha (só números, 1 a 8 dígitos)</h2>
  <input id="nova" type="password" inputmode="numeric" maxlength="8" placeholder="Nova senha">
  <button class="azul" style="width:100%" onclick="mudarSenha()">Salvar senha</button>
  <div id="msg"></div>
</div>

<div class="card">
  <h2>Histórico</h2>
  <ul id="eventos"></ul>
</div>

<script>
function atualizar() {
  fetch('/estado').then(r => r.json()).then(d => {
    const st = document.getElementById('status');
    st.textContent = d.destravado ? 'DESTRAVADA' : 'TRAVADA';
    st.className = 'status ' + (d.destravado ? 'aberta' : 'travada');
    document.getElementById('porta').textContent = d.porta;
    document.getElementById('pontos').textContent = '●'.repeat(d.digitando);
    document.getElementById('eventos').innerHTML =
      d.eventos.map(e => '<li>' + e + '</li>').join('');
  }).catch(() => {});
}

function enviar(rota) {
  fetch(rota, { method: 'POST' }).then(atualizar);
}

function mudarSenha() {
  const nova = document.getElementById('nova').value;
  fetch('/senha', { method: 'POST', body: new URLSearchParams({ nova: nova }) })
    .then(r => r.text())
    .then(t => {
      document.getElementById('msg').textContent = t;
      document.getElementById('nova').value = '';
      atualizar();
    });
}

setInterval(atualizar, 500);
atualizar();
</script>
</body>
</html>
)rawliteral";

void rotaPagina() {
  server.send_P(200, "text/html", PAGINA);
}

void rotaEstado() {
  String j = "{";
  j += "\"destravado\":" + String(destravado ? "true" : "false") + ",";
  j += "\"porta\":\"" + String(portaFechada() ? "fechada" : "aberta") + "\",";
  j += "\"digitando\":" + String(digitado.length()) + ",";
  j += "\"eventos\":[";
  int n = min(totalEv, MAX_EV);
  for (int i = 0; i < n; i++) {
    int k = (totalEv - 1 - i) % MAX_EV;   // mais novo primeiro
    if (i > 0) j += ",";
    j += "\"" + eventos[k] + "\"";
  }
  j += "]}";
  server.send(200, "application/json", j);
}

void rotaAbrir() {
  destravar("site");
  server.send(200, "text/plain", "ok");
}

void rotaTravar() {
  travar("site");
  server.send(200, "text/plain", "ok");
}

void rotaSenha() {
  String nova = server.arg("nova");
  bool valida = nova.length() >= 1 && nova.length() <= 8;
  for (unsigned int i = 0; i < nova.length(); i++) {
    if (!isDigit(nova[i])) valida = false;
  }
  if (!valida) {
    server.send(400, "text/plain", "Senha inválida: use só números (1 a 8).");
    return;
  }
  senha = nova;
  prefs.putString("senha", senha);
  registrar("Senha alterada pelo site");
  server.send(200, "text/plain", "Senha salva!");
}

void setup() {
  Serial.begin(115200);
  pinMode(PINO_BOTAO, INPUT_PULLUP);
  pinMode(PINO_LED, OUTPUT);
  trava.attach(PINO_SERVO, 500, 2400);

  prefs.begin("smartbox", false);
  senha = prefs.getString("senha", "1234");   // padrão 1234

  // Conecta no Wi-Fi (LED pisca enquanto tenta)
  WiFi.begin(WIFI_NOME, WIFI_SENHA);
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 15000) {
    digitalWrite(PINO_LED, !digitalRead(PINO_LED));
    delay(300);
  }

  if (WiFi.status() == WL_CONNECTED) {
    MDNS.begin("smartbox");
    Serial.print("Abra: http://smartbox.local  ou  http://");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.softAP("SmartBox", "12345678");   // plano B
    Serial.println("Sem Wi-Fi: conecte em SmartBox e abra http://192.168.4.1");
  }

  travar("início");   // também apaga o LED

  server.on("/", HTTP_GET, rotaPagina);
  server.on("/estado", HTTP_GET, rotaEstado);
  server.on("/abrir", HTTP_POST, rotaAbrir);
  server.on("/travar", HTTP_POST, rotaTravar);
  server.on("/senha", HTTP_POST, rotaSenha);
  server.begin();
}

void loop() {
  server.handleClient();
  verificarPorta();

  char t = teclado.getKey();
  if (!t) return;

  if (t == '*') {
    digitado = "";
    registrar("Tecla * (apagou)");
  }
  else if (t == '#') {
    if (digitado == senha) {
      registrar("Senha correta no teclado");
      destravar("teclado");
    } else if (digitado != "") {
      registrar("Senha ERRADA no teclado");
    }
    digitado = "";
  }
  else {
    digitado += t;
    registrar("Tecla " + String(t));
  }
}