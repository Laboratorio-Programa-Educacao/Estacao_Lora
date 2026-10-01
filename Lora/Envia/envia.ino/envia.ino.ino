#include <HardwareSerial.h>

#define RADIO_RX 16   // Ligar ao TXD do E49
#define RADIO_TX 17   // Ligar ao RXD do E49
#define RADIO_BAUD 9600

// true = mostra o pacote bruto antes do quadro (útil para depurar). Depois pode pôr false.
#define DEBUG_RAW true

HardwareSerial Radio(2);
String bufferRadio = "";
unsigned long ultimaRecepcaoByte = 0;
long ultimoSeq = -1;

const char* BORDA = "+-------------------------------------+";

void exibeDados(String dados);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Radio.begin(RADIO_BAUD, SERIAL_8N1, RADIO_RX, RADIO_TX);

  // Limpa ruídos residuais ao iniciar
  while (Radio.available()) {
    Radio.read();
  }

  Serial.println();
  Serial.println(BORDA);
  Serial.println("|   ESTACAO RECEPTORA PRONTA          |");
  Serial.println("|   Aguardando dados do radio...      |");
  Serial.println(BORDA);
}

void loop() {
  // Escuta contínua do rádio
  while (Radio.available()) {
    char c = Radio.read();
    ultimaRecepcaoByte = millis();

    if (c == '\n') {
      bufferRadio.trim();
      if (bufferRadio.length() > 0) {
        if (bufferRadio.startsWith("DADOS:")) {
          exibeDados(bufferRadio.substring(6));
        } else {
          Serial.println("[RX bruto]: " + bufferRadio);
        }
      }
      bufferRadio = "";
    } else if (c != '\r') {
      bufferRadio += c;
    }

    // Trava de segurança 1: texto maior que 200 caracteres (lixo) é descartado
    if (bufferRadio.length() > 200) {
      Serial.println("[AVISO] Buffer excedeu 200 caracteres, descartado.");
      bufferRadio = "";
    }
  }

  // Trava de segurança 2: mais de 2 s sem receber byte, descarta o que ficou pela metade
  if (bufferRadio.length() > 0 && (millis() - ultimaRecepcaoByte > 2000)) {
    Serial.println("[RX incompleto descartado]: " + bufferRadio);
    bufferRadio = "";
  }
}

// Extrai só HH:MM de qualquer formato ("16:30" ou "01/10/2026 16:30:04")
String formataHora(String h) {
  int i = h.indexOf(':');
  if (i >= 2 && h.length() >= (unsigned)(i + 3)) {
    return h.substring(i - 2, i + 3);
  }
  return "--:--";
}

void linha(const char* rotulo, const String& valor) {
  char buf[64];
  snprintf(buf, sizeof(buf), "|  %-12s %-22s|", rotulo, valor.c_str());
  Serial.println(buf);
}

void exibeDados(String dados) {
  float t = NAN, h = NAN, p = NAN;
  int erroTransmissor = -1;
  long seq = -1;
  String horaTransmissao = "";

  int pos = 0;
  while (pos < (int)dados.length()) {
    int fim = dados.indexOf(';', pos);
    if (fim == -1) fim = dados.length();

    String par = dados.substring(pos, fim);
    int igual = par.indexOf('=');

    if (igual > 0) {
      String chave = par.substring(0, igual);
      String valor = par.substring(igual + 1);

      if (chave == "T" && valor != "NAN") t = valor.toFloat();
      else if (chave == "H" && valor != "NAN") h = valor.toFloat();
      else if (chave == "P" && valor != "NAN") p = valor.toFloat();
      else if (chave == "ERR") erroTransmissor = valor.toInt();
      else if (chave == "SEQ") seq = valor.toInt();
      else if (chave == "HORA") horaTransmissao = valor;
    }
    pos = fim + 1;
  }

  // Descarta a repetição do mesmo pacote (a transmissora envia 2 vezes)
  if (seq >= 0 && seq == ultimoSeq) return;
  if (seq >= 0) ultimoSeq = seq;

#if DEBUG_RAW
  Serial.println("[RX bruto]: DADOS:" + dados);
#endif

  char titulo[64];
  snprintf(titulo, sizeof(titulo), "|  %-21s %12s |", "ESTACAO METEOROLOGICA",
           ("Hora " + formataHora(horaTransmissao)).c_str());

  Serial.println();
  Serial.println(BORDA);
  Serial.println(titulo);
  Serial.println(BORDA);
  linha("Temperatura:", isnan(t) ? "[ERRO]" : String(t, 1) + " C");
  linha("Umidade:",     isnan(h) ? "[ERRO]" : String(h, 1) + " %");
  linha("Pressao:",     isnan(p) ? "[ERRO]" : String(p, 1) + " hPa");
  if (erroTransmissor > 0) {
    linha("AVISO:", "sensores c/ erro: " + String(erroTransmissor));
  }
  Serial.println(BORDA);
}