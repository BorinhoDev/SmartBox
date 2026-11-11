#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <time.h>
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

WebServer server(80);             // servidor HTTP na porta 80 (TCP)
Preferences prefs;

const int TAM_SENHA = 4;          // senha sempre com 4 dígitos
String senha = "2580";
String digitado = "";
bool destravado = false;
bool portaFoiAberta = false;
unsigned long inicioAperto = 0;

// ---- Senha temporária (aleatória, só com 2, 5, 8 e 0) ----
String senhaTemp = "";
unsigned long tempInicio = 0;
unsigned long tempDuracao = 0;    // em milissegundos

// ---- Wi-Fi: reconexão ----
bool wifiOk = false;
bool mdnsAtivo = false;
unsigned long ultimaTentativa = 0;
int quedas = 0;

// ---- Log: últimos 15 na memória + arquivo na flash ----
const int MAX_EV = 15;
String eventos[MAX_EV];
int totalEv = 0;
bool flashOk = false;
const char* ARQ_LOG = "/log.txt";
const char* ARQ_LOG_ANTIGO = "/log_antigo.txt";
const size_t LIMITE_LOG = 64 * 1024;   // ao passar de 64 KB, o log vira "antigo"

// Data e hora pelo NTP; antes de sincronizar, usa o tempo desde que ligou
String agora() {
  struct tm t;
  if (getLocalTime(&t, 10)) {
    char b[24];
    strftime(b, sizeof(b), "%d/%m/%Y %H:%M:%S", &t);
    return String(b);
  }
  unsigned long s = millis() / 1000;
  char b[24];
  sprintf(b, "sem hora +%02lu:%02lu", s / 60, s % 60);
  return String(b);
}

// nivel: "INFO", "AVISO" ou "ERRO"
void registrar(String nivel, String msg) {
  String linha = "[" + nivel + "] " + agora() + " - " + msg;
  eventos[totalEv % MAX_EV] = linha;
  totalEv++;
  Serial.println(linha);

  if (!flashOk) return;
  File f = LittleFS.open(ARQ_LOG, FILE_APPEND);
  if (!f) return;
  f.println(linha);
  size_t tamanho = f.size();
  f.close();
  if (tamanho > LIMITE_LOG) {
    LittleFS.remove(ARQ_LOG_ANTIGO);
    LittleFS.rename(ARQ_LOG, ARQ_LOG_ANTIGO);
  }
}

bool portaFechada() {
  return digitalRead(PINO_BOTAO) == LOW;
}

void travar(String origem) {
  trava.write(TRAVADO);
  destravado = false;
  portaFoiAberta = false;
  digitalWrite(PINO_LED, LOW);
  registrar("INFO", "Travado (" + origem + ")");
}

void destravar(String origem) {
  trava.write(DESTRAVADO);
  destravado = true;
  portaFoiAberta = false;
  digitalWrite(PINO_LED, HIGH);
  registrar("INFO", "Destravado (" + origem + ")");
}

void verificarPorta() {
  if (!destravado) return;

  if (!portaFechada()) {
    if (!portaFoiAberta) registrar("INFO", "Porta aberta");
    portaFoiAberta = true;
    inicioAperto = 0;
  }
  else if (portaFoiAberta) {
    if (inicioAperto == 0) inicioAperto = millis();
    if (millis() - inicioAperto > 500) {
      registrar("INFO", "Porta fechada");
      travar("porta");
    }
  }
}

// ---- Wi-Fi: detecta queda e reconecta a cada 10 s ----
void verificarWifi() {
  bool conectado = WiFi.status() == WL_CONNECTED;

  if (conectado && !wifiOk) {
    wifiOk = true;
    if (mdnsAtivo) MDNS.end();
    mdnsAtivo = MDNS.begin("smartbox");
    registrar("INFO", "Wi-Fi conectado. IP " + WiFi.localIP().toString() +
                      ", sinal " + String(WiFi.RSSI()) + " dBm");
  }
  else if (!conectado && wifiOk) {
    wifiOk = false;
    quedas++;
    registrar("ERRO", "Conexão Wi-Fi perdida (queda " + String(quedas) + ")");
    ultimaTentativa = millis();
  }

  if (!conectado && millis() - ultimaTentativa > 10000) {
    ultimaTentativa = millis();
    registrar("AVISO", "Tentando reconectar ao Wi-Fi " + String(WIFI_NOME));
    WiFi.disconnect();
    WiFi.begin(WIFI_NOME, WIFI_SENHA);
  }
}

// ---- Senha temporária ----
bool tempAtiva() {
  return senhaTemp != "" && millis() - tempInicio < tempDuracao;
}

unsigned long tempRestante() {   // em segundos
  if (!tempAtiva()) return 0;
  return (tempDuracao - (millis() - tempInicio)) / 1000;
}

void verificarTemp() {
  if (senhaTemp != "" && !tempAtiva()) {
    registrar("INFO", "Senha temporária " + senhaTemp + " expirou");
    senhaTemp = "";
  }
}

String gerarSenhaTemp() {
  const char digitos[] = "2580";
  String s = "";
  for (int i = 0; i < TAM_SENHA; i++) {
    s += digitos[esp_random() % 4];
  }
  return s;
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
  .temp { font-size:34px; font-weight:bold; text-align:center; letter-spacing:8px; color:#facc15; }
  .botoes { display:flex; gap:10px; }
  button { flex:1; padding:14px; font-size:16px; border:0; border-radius:8px; cursor:pointer; color:white; }
  .verde { background:#16a34a; } .vermelho { background:#dc2626; } .azul { background:#2563eb; } .roxo { background:#7c3aed; }
  input { width:100%; box-sizing:border-box; padding:12px; font-size:18px; border-radius:8px; border:0; margin-bottom:10px; }
  label { display:block; margin-bottom:6px; color:#d1d5db; }
  table { width:100%; font-size:14px; } td { padding:3px 0; } td:last-child { text-align:right; font-family:monospace; }
  a { color:#93c5fd; }
  ul { list-style:none; padding:0; margin:0; font-family:monospace; font-size:13px; }
  li { padding:4px 0; border-bottom:1px solid #374151; }
  .ERRO { color:#fca5a5; } .AVISO { color:#fcd34d; }
  #msg, #msgTemp { text-align:center; margin-top:8px; min-height:20px; }
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
  <h2>Senha temporária (aleatória)</h2>
  <div id="tempAtiva" style="display:none">
    <div class="temp" id="tempSenha"></div>
    <div class="info">Expira em <b id="tempTempo"></b></div>
    <br>
    <button class="vermelho" style="width:100%" onclick="cancelarTemp()">Cancelar senha temporária</button>
  </div>
  <div id="tempNova">
    <label for="minutos">Funciona por quantos minutos?</label>
    <input id="minutos" type="number" min="1" max="1440" value="10">
    <button class="roxo" style="width:100%" onclick="gerarTemp()">Gerar senha aleatória</button>
  </div>
  <div id="msgTemp"></div>
</div>

<div class="card">
  <h2>Nova senha fixa (4 números)</h2>
  <input id="nova" type="password" inputmode="numeric" maxlength="4" placeholder="Nova senha">
  <button class="azul" style="width:100%" onclick="mudarSenha()">Salvar senha</button>
  <div id="msg"></div>
</div>

<div class="card">
  <h2>Rede</h2>
  <table>
    <tr><td>Status</td><td id="wifi">...</td></tr>
    <tr><td>Endereço IP</td><td id="ip">...</td></tr>
    <tr><td>MAC</td><td id="mac">...</td></tr>
    <tr><td>Sinal</td><td id="rssi">...</td></tr>
    <tr><td>Quedas desde que ligou</td><td id="quedas">...</td></tr>
    <tr><td>Hora do ESP32</td><td id="hora">...</td></tr>
  </table>
</div>

<div class="card">
  <h2>Log (últimos 15) · <a href="/log" target="_blank">ver log completo</a></h2>
  <ul id="eventos"></ul>
</div>

<script>
function formatarTempo(s) {
  const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), seg = s % 60;
  const mm = String(m).padStart(2, '0'), ss = String(seg).padStart(2, '0');
  return h > 0 ? h + 'h ' + mm + 'min' : mm + ':' + ss;
}

function atualizar() {
  fetch('/estado').then(r => r.json()).then(d => {
    const st = document.getElementById('status');
    st.textContent = d.destravado ? 'DESTRAVADA' : 'TRAVADA';
    st.className = 'status ' + (d.destravado ? 'aberta' : 'travada');
    document.getElementById('porta').textContent = d.porta;
    document.getElementById('pontos').textContent = d.digitado;

    const ativa = d.temp !== '';
    document.getElementById('tempAtiva').style.display = ativa ? 'block' : 'none';
    document.getElementById('tempNova').style.display = ativa ? 'none' : 'block';
    if (ativa) {
      document.getElementById('tempSenha').textContent = d.temp;
      document.getElementById('tempTempo').textContent = formatarTempo(d.tempRestante);
    }

    document.getElementById('wifi').textContent = d.wifi;
    document.getElementById('ip').textContent = d.ip;
    document.getElementById('mac').textContent = d.mac;
    document.getElementById('rssi').textContent = d.rssi + ' dBm';
    document.getElementById('quedas').textContent = d.quedas;
    document.getElementById('hora').textContent = d.hora;

    document.getElementById('eventos').innerHTML = d.eventos.map(e => {
      const nivel = e.startsWith('[ERRO]') ? 'ERRO' : (e.startsWith('[AVISO]') ? 'AVISO' : '');
      return '<li class="' + nivel + '">' + e + '</li>';
    }).join('');
  }).catch(() => {});
}

function enviar(rota) {
  fetch(rota, { method: 'POST' }).then(atualizar);
}

function gerarTemp() {
  const minutos = document.getElementById('minutos').value;
  fetch('/temp', { method: 'POST', body: new URLSearchParams({ minutos: minutos }) })
    .then(r => r.text())
    .then(t => { document.getElementById('msgTemp').textContent = t; atualizar(); });
}

function cancelarTemp() {
  fetch('/cancelarTemp', { method: 'POST' })
    .then(r => r.text())
    .then(t => { document.getElementById('msgTemp').textContent = t; atualizar(); });
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
  j += "\"digitado\":\"" + digitado + "\",";
  j += "\"temp\":\"" + (tempAtiva() ? senhaTemp : String("")) + "\",";
  j += "\"tempRestante\":" + String(tempRestante()) + ",";
  j += "\"wifi\":\"" + String(WiFi.status() == WL_CONNECTED ? "conectado" : "desconectado") + "\",";
  j += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  j += "\"mac\":\"" + WiFi.macAddress() + "\",";
  j += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  j += "\"quedas\":" + String(quedas) + ",";
  j += "\"hora\":\"" + agora() + "\",";
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

// Log completo guardado na flash (o antigo primeiro, depois o atual)
void rotaLog() {
  if (!flashOk) {
    server.send(500, "text/plain; charset=utf-8", "Memória flash indisponível.");
    return;
  }
  String texto = "";
  const char* arquivos[] = { ARQ_LOG_ANTIGO, ARQ_LOG };
  for (const char* nome : arquivos) {
    File f = LittleFS.open(nome, FILE_READ);
    if (f) {
      texto += f.readString();
      f.close();
    }
  }
  if (texto == "") texto = "Log vazio.";
  server.send(200, "text/plain; charset=utf-8", texto);
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
  bool valida = nova.length() == TAM_SENHA;
  for (unsigned int i = 0; i < nova.length(); i++) {
    if (!isDigit(nova[i])) valida = false;
  }
  if (!valida) {
    registrar("AVISO", "Tentativa de salvar senha inválida pelo site");
    server.send(400, "text/plain", "Senha inválida: use exatamente 4 números.");
    return;
  }
  senha = nova;
  prefs.putString("senha", senha);
  registrar("INFO", "Senha fixa alterada pelo site");
  server.send(200, "text/plain", "Senha salva!");
}

void rotaTemp() {
  long minutos = server.arg("minutos").toInt();
  if (minutos < 1 || minutos > 1440) {
    server.send(400, "text/plain", "Tempo inválido: use de 1 a 1440 minutos.");
    return;
  }
  senhaTemp = gerarSenhaTemp();
  tempInicio = millis();
  tempDuracao = (unsigned long)minutos * 60000UL;
  registrar("INFO", "Senha temporária " + senhaTemp + " criada (" + String(minutos) + " min)");
  server.send(200, "text/plain", "Senha temporária criada!");
}

void rotaCancelarTemp() {
  if (senhaTemp != "") {
    registrar("INFO", "Senha temporária " + senhaTemp + " cancelada");
    senhaTemp = "";
  }
  server.send(200, "text/plain", "Senha temporária cancelada.");
}

void setup() {
  Serial.begin(115200);
  pinMode(PINO_BOTAO, INPUT_PULLUP);
  pinMode(PINO_LED, OUTPUT);
  trava.attach(PINO_SERVO, 500, 2400);

  flashOk = LittleFS.begin(true);   // formata na primeira vez
  if (!flashOk) Serial.println("[ERRO] Falha ao montar a memória flash (LittleFS)");

  prefs.begin("smartbox", false);
  senha = prefs.getString("senha", "2580");   // padrão 2580
  if (senha.length() != TAM_SENHA) {          // senha antiga fora do padrão
    senha = "2580";
    prefs.putString("senha", senha);
  }

  // Horário de Brasília pelo NTP (vale assim que o Wi-Fi conectar)
  configTzTime("<-03>3", "a.st1.ntp.br", "pool.ntp.org");

  // Conecta no Wi-Fi da casa (LED pisca enquanto tenta)
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NOME, WIFI_SENHA);
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 15000) {
    digitalWrite(PINO_LED, !digitalRead(PINO_LED));
    delay(300);
  }
  struct tm t;
  getLocalTime(&t, 5000);           // espera a hora chegar (até 5 s)

  registrar("INFO", "SmartBox ligado");
  if (WiFi.status() != WL_CONNECTED) {
    registrar("ERRO", "Não conectou ao Wi-Fi " + String(WIFI_NOME) + " ao ligar");
  }
  verificarWifi();                  // registra IP e liga o mDNS
  travar("início");                 // também apaga o LED

  server.on("/", HTTP_GET, rotaPagina);
  server.on("/estado", HTTP_GET, rotaEstado);
  server.on("/log", HTTP_GET, rotaLog);
  server.on("/abrir", HTTP_POST, rotaAbrir);
  server.on("/travar", HTTP_POST, rotaTravar);
  server.on("/senha", HTTP_POST, rotaSenha);
  server.on("/temp", HTTP_POST, rotaTemp);
  server.on("/cancelarTemp", HTTP_POST, rotaCancelarTemp);
  server.begin();                   // abre a porta 80 e fica escutando
  Serial.println("Abra: http://smartbox.local  ou  http://" + WiFi.localIP().toString());
}

void loop() {
  server.handleClient();
  verificarWifi();
  verificarPorta();
  verificarTemp();

  char t = teclado.getKey();
  if (!t) return;

  if (t == '*' || t == '#') {     // * ou # apagam o que foi digitado
    digitado = "";
    return;
  }

  digitado += t;

  if (digitado.length() == TAM_SENHA) {   // 4 dígitos: confere na hora
    if (digitado == senha) {
      registrar("INFO", "Senha fixa correta no teclado");
      destravar("teclado");
    } else if (tempAtiva() && digitado == senhaTemp) {
      registrar("INFO", "Senha temporária " + senhaTemp + " usada no teclado");
      destravar("senha temporária");
    } else {
      registrar("AVISO", "Senha errada no teclado");
    }
    digitado = "";
  }
}
