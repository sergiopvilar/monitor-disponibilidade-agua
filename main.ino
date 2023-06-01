// Informações de comunicação com o Blynk
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""
#define BLYNK_PRINT Serial

// Pinos de comunicação
#define VALVULA_ENTRADA 25
#define VALVULA_SAIDA 26
#define BOIA 34

// Bibliotecas
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Credenciais wifi
char ssid[] = "Vilar";
char pass[] = "gaiapeste";

// Variáveis de estado
int estado = 0;
int inicia_medicao = 0;
int contador_medicao = 0;
int counter = 0;
int tempo_enchimento = 15;
int leitura = 0;
char* nome_medidor = "Casa de Sergio";

// Timer do Blynk
BlynkTimer timer;

// Evento de mudança do Pino Virtual
// Pino V5: Estado da disponibilidade de água
BLYNK_WRITE(V5)
{
  // Parse dos parâmetros
  int value = param.asInt();

  if(value != estado) notify(value);
}

// Evento de mudança do Pino Virtual
// Pino V3: Estado da válvula de entrada
BLYNK_WRITE(V3)
{
  // Parse dos parâmetros
  int value = param.asInt();

  Serial.println("Entrada :");
  Serial.println(value);

  // Pela particularidade da montagem, enviamos LOW para ligar o relé
  digitalWrite(VALVULA_ENTRADA, value > 0 ? LOW : HIGH);
}

// Evento de mudança do Pino Virtual
// Pino V4: Estado da válvula de saída
BLYNK_WRITE(V4)
{
  // Parse dos parâmetros
  int value = param.asInt();

  Serial.println("Saida :");
  Serial.println(value);

  // Pela particularidade da montagem, enviamos LOW para ligar o relé
  digitalWrite(VALVULA_SAIDA, value > 0 ? LOW : HIGH);
}

// Evento de mudança do Pino Virtual
// Pino V1: Estado da medição
BLYNK_WRITE(V1)
{
  // Parse dos parâmetros
  int value = param.asInt();

  inicia_medicao = value;
}

// Evento de mudança do Pino Virtual
// Pino V1: Tempo de enchimento
BLYNK_WRITE(V7)
{
  // Parse dos parâmetros
  int value = param.asInt();

  Serial.println("Tempo de enchimento:");
  Serial.println(value);

  tempo_enchimento = value;
}

// Sincronizar todos pinos virtuais ao conectar no Wifi
BLYNK_CONNECTED() {
    Blynk.syncAll();
}

// Notifica falta ou chegada de água
void notify(int value) {
  Serial.println("Notificando mudanca" + value);
  if(value == 1) Blynk.logEvent("chegou_agua", nome_medidor);
  if(value == 0) Blynk.logEvent("faltou_agua", nome_medidor);
}

// Realimenta o estado no Blynk a cada iteração do timer
void setValue(int value) {
  estado = value;
  Blynk.virtualWrite(V5, value);
  Blynk.virtualWrite(V6, value > 0 ? "Tem água" : "Está faltando água");
}

// Liga válvula de entrada
// Pela particularidade da montagem, enviamos LOW para ligar o relé
void ligaEntrada() {
  Serial.println("Ligando válvula de entrada");
  Blynk.virtualWrite(V3, 1);
  digitalWrite(VALVULA_ENTRADA, LOW);
  desligaSaida();

}

// Desliga válvula de entrada
// Pela particularidade da montagem, enviamos HIGH para desligar o relé
void desligaEntrada() {
  Serial.println("Desligando válvula de entrada");
  Blynk.virtualWrite(V3, 0);
  digitalWrite(VALVULA_ENTRADA, HIGH);
}

// Liga válvula de saída
// Pela particularidade da montagem, enviamos LOW para ligar o relé
void ligaSaida() {  
  Serial.println("Ligando válvula de saída");
  Blynk.virtualWrite(V4, 1);
  digitalWrite(VALVULA_SAIDA, LOW);
  desligaEntrada();
}

// Desliga válvula de saída
// Pela particularidade da montagem, enviamos HIGH para desligar o relé
void desligaSaida() {
  Serial.println("Desligando válvula de saída");
  Blynk.virtualWrite(V4, 0);
  digitalWrite(VALVULA_SAIDA, HIGH);
}

// Lê a bóia e notifica o Blynk
void leSensor() {
  Serial.println("Lendo valor do sensor");
  leitura = digitalRead(BOIA);
  if(leitura != estado) notify(leitura);
  estado = leitura;
}

// Lógica da sequência de medição
void medicaoFacade() {
  if(contador_medicao == 1){
    Serial.println("ENCHENDO MEDIDOR");
    Serial.println("----------------");
    ligaEntrada();
    Blynk.virtualWrite(V2, "Enchendo medidor");
  }
  
  if(contador_medicao == tempo_enchimento) {
    Serial.println("VERIFICANDO NIVEL");
    Serial.println("-----------------");
    Blynk.virtualWrite(V2, "Verificando nível");
    desligaEntrada();
  }
  
  if(contador_medicao == tempo_enchimento + 1) leSensor();

  if(contador_medicao == tempo_enchimento + 2) {
    Serial.println("ESVAZIANDO MEDIDOR");
    Serial.println("------------------");
    Blynk.virtualWrite(V2, "Esvaziando medidor");
    ligaSaida();
  }

  if(contador_medicao == 3 + (tempo_enchimento * 2)) desligaSaida();
  if(contador_medicao == 4 + (tempo_enchimento * 2)) {
    contador_medicao = 0;
    inicia_medicao = 0;
    Blynk.virtualWrite(V1, 0);
    Blynk.virtualWrite(V2, "Medição finalizada");
  }
}

// Inicia lógica do medidor
void myTimerEvent()
{
  if(inicia_medicao > 0){
    contador_medicao = contador_medicao + 1;
    medicaoFacade();
  }
  
  // Alimenta o estado a cada iteração
  setValue(estado);
}

void setup()
{
  // Debug console
  Serial.begin(115200);

  // Conecta no Blynk e no Wifi
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  // Como não podemos usar delay() devido ao uso do Blynk
  // usamos um timer que roda a cada 1s
  timer.setInterval(1000L, myTimerEvent);

  // Define modo da pinagem
  pinMode(BOIA, INPUT);
  pinMode(VALVULA_ENTRADA, OUTPUT);
  pinMode(VALVULA_SAIDA, OUTPUT);
}

void loop()
{
  Blynk.run();
  timer.run();  
}
