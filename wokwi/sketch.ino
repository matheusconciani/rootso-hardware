#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>

// ----- OLED -----
const int LARGURA_TELA = 128;
const int ALTURA_TELA = 64;
const int PINO_SDA_OLED = 21;
const int PINO_SCL_OLED = 22;
const int ENDERECO_OLED = 0x3C;

Adafruit_SSD1306 tela(
  LARGURA_TELA,
  ALTURA_TELA,
  &Wire,
  -1
);

// ----- Pinos do circuito -----
const int PINO_DHT = 15;
const int PINO_LDR = 34;
const int PINO_BOTAO_N = 32;
const int PINO_BOTAO_P = 25;
const int PINO_BOTAO_K = 26;
const int PINO_RELE = 27;

// ----- Sensor DHT22 -----
DHTesp sensorDHT;

// ----- Irrigação -----
// Valores didáticos para a simulação, não são recomendações agronômicas.
const float UMIDADE_LIGA = 40.0;
const float UMIDADE_DESLIGA = 45.0;

// O módulo de relé Wokwi normalmente é acionado com HIGH.
// Se o seu funcionar invertido, troque HIGH por LOW nestas constantes.
const int RELE_LIGADO = HIGH;
const int RELE_DESLIGADO = LOW;

// Fazemos a leitura do DHT22 em intervalos, em vez de a cada passagem do loop.
const unsigned long INTERVALO_LEITURA = 2500;
unsigned long ultimaLeitura = 0;

// Últimas leituras
float temperatura = NAN;
float umidade = NAN;
float phSimulado = 0.0;
int leituraLDR = 0;

bool bombaLigada = false;

// Botão pressionado será interpretado como nutriente presente/adequado.
// Com INPUT_PULLUP, pressionado = LOW; solto = HIGH.
bool nitrogenioAdequado = false;
bool fosforoAdequado = false;
bool potassioAdequado = false;

void atualizarSensores() {
  TempAndHumidity dados = sensorDHT.getTempAndHumidity();

  temperatura = dados.temperature;
  umidade = dados.humidity;

  leituraLDR = analogRead(PINO_LDR);

  // Conversão didática do valor do LDR para uma escala de 0 a 14.
  // Não é uma medição real de pH: o LDR mede intensidade de luz.
  phSimulado = (leituraLDR / 4095.0) * 14.0;

  nitrogenioAdequado = (digitalRead(PINO_BOTAO_N) == LOW);
  fosforoAdequado = (digitalRead(PINO_BOTAO_P) == LOW);
  potassioAdequado = (digitalRead(PINO_BOTAO_K) == LOW);

  Serial.println("----- ROOTSO | Milho -----");

  if (isnan(umidade) || isnan(temperatura)) {
    Serial.println("Erro na leitura do DHT22.");
  } else {
    Serial.print("Umidade simulada: ");
    Serial.print(umidade, 1);
    Serial.println("%");

    Serial.print("Temperatura: ");
    Serial.print(temperatura, 1);
    Serial.println(" C");
  }

  Serial.print("LDR: ");
  Serial.print(leituraLDR);
  Serial.print(" | pH simulado: ");
  Serial.println(phSimulado, 1);

  Serial.print("N: ");
  Serial.print(nitrogenioAdequado ? "OK" : "baixo");
  Serial.print(" | P: ");
  Serial.print(fosforoAdequado ? "OK" : "baixo");
  Serial.print(" | K: ");
  Serial.println(potassioAdequado ? "OK" : "baixo");
}

void controlarIrrigacao() {
  // Sem leitura válida, a bomba fica desligada por segurança.
  if (isnan(umidade)) {
    bombaLigada = false;
  } else if (!bombaLigada && umidade < UMIDADE_LIGA) {
    bombaLigada = true;
  } else if (bombaLigada && umidade >= UMIDADE_DESLIGA) {
    bombaLigada = false;
  }

  digitalWrite(
    PINO_RELE,
    bombaLigada ? RELE_LIGADO : RELE_DESLIGADO
  );

  Serial.print("Irrigacao: ");
  Serial.println(bombaLigada ? "LIGADA" : "DESLIGADA");

}

void atualizarTela() {
  tela.clearDisplay();
  tela.setTextColor(SSD1306_WHITE);
  tela.setTextSize(1);

  tela.setCursor(0, 0);
  tela.println("ROOTSO | MILHO");

  tela.setCursor(0, 10);
  if (isnan(umidade)) {
    tela.println("Umidade: erro DHT22");
  } else {
    tela.print("Umidade: ");
    tela.print(umidade, 1);
    tela.println("%");
  }

  tela.setCursor(0, 20);
  tela.print("pH simulado: ");
  tela.println(phSimulado, 1);

  tela.setCursor(0, 30);
  tela.print("N:");
  tela.print(nitrogenioAdequado ? "OK " : "-- ");
  tela.print("P:");
  tela.print(fosforoAdequado ? "OK " : "-- ");
  tela.print("K:");
  tela.println(potassioAdequado ? "OK" : "--");

  tela.setCursor(0, 40);
  tela.print("Bomba: ");
  tela.println(bombaLigada ? "LIGADA" : "DESLIGADA");

  tela.setCursor(0, 50);
  if (isnan(temperatura)) {
    tela.println("Temperatura: --");
  } else {
    tela.print("Temperatura: ");
    tela.print(temperatura, 1);
    tela.println(" C");
  }

  tela.display();
}

void setup() {
  Serial.begin(115200);

  pinMode(PINO_BOTAO_N, INPUT_PULLUP);
  pinMode(PINO_BOTAO_P, INPUT_PULLUP);
  pinMode(PINO_BOTAO_K, INPUT_PULLUP);

  pinMode(PINO_RELE, OUTPUT);
  digitalWrite(PINO_RELE, RELE_DESLIGADO);

  Wire.begin(PINO_SDA_OLED, PINO_SCL_OLED);

  if (!tela.begin(SSD1306_SWITCHCAPVCC, ENDERECO_OLED)) {
    Serial.println("Erro ao iniciar o OLED.");
    while (true) {
      delay(10);
    }
  }

  sensorDHT.setup(PINO_DHT, DHTesp::DHT22);

  tela.clearDisplay();
  tela.setTextColor(SSD1306_WHITE);
  tela.setTextSize(1);
  tela.setCursor(50, 0);
  tela.println("ROOTSO");
  tela.println();
  tela.println();
  tela.println();
  tela.println("Iniciando sensores...");
  tela.display();

  delay(5000);
}

void loop() {
  unsigned long agora = millis();

  if (agora - ultimaLeitura >= INTERVALO_LEITURA) {
    ultimaLeitura = agora;

    atualizarSensores();
    controlarIrrigacao();
    atualizarTela();
  }
}