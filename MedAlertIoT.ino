/******************************************************************************************************************************
   DESENVOLVEDOR:           Edgard Oliveira
   PROJETO:                 TCC Dispensador IoT
   CODENAME:                MedAlertIoT
   DATA DE CRIAÇÃO:         04/12/2021
   DATA DA ÚLTIMA REVISÃO:  05/12/2021
   OBJETIVO:                Comunicar-se com a MedAlertAPI e verificar se está na hora de liberar a medicação

 ******************************************************************************************************************************/
// Bibliotecas
#include <WiFi.h>                     //Biblioteca para gerenciar o Wifi
#include <HTTPClient.h>               //Biblioteca para trabalhar com HTTP
#include <ArduinoJson.h>              //Biblioteca para trabalhar com JSON no arduino
#include <Servo.h>                    //Biblioteca para controle do microservo

// Definição dos pinos utilizados
#define SERVO_MOTOR01 13             //GPIO 13 MICRO SERVO MOTOR
#define SERVO_MOTOR02 12             //GPIO 12 MICRO SERVO MOTOR
#define SERVO_MOTOR03 26             //GPIO 26 MICRO SERVO MOTOR

//Constantes
const char    *SSID           = "sua_rede_wiif";                    //Rede Wifi
const char    *PASSWORD       = "sua_senha_do_wif";                 //Senha da rede wifi
const String  BASE_URL        = "https://medalert.vercel.app/api/"; //endereço do backend
const String  DISPENSADOR_ID  = "1";                                //Identificador do dispensador
const String  PACIENTE_EMAIL  = "email_paciente";                   //Email do paciente
const String  PACIENTE_SENHA  = "senha_email";                      //Senha do paciente
const String  RECEITA_ID      = "identificador_receita";            //Receita do paciente

//Variáveis Globais
String        token                 = "";                           //token de acesso
unsigned long ultimaAtualizacao     = 0;                            //ultima vez que atualizou dados vindos da API
unsigned long intervaloAtualizacao  = 5000;                         //intervalo de atualização de dados da API (5 segundos)
unsigned long ultimoLogin           = 0;                            //ultima vez que fez login na APO
unsigned long intervaloLogin        = 3600000;                      //intervalo de login (1 hora convertida em milisegundos)

//Instanciando os servo motores
Servo         servoMotor01, servoMotor02, servoMotor03;             // instância chamada meuservo

/**
   Objetivo: Função responsável por configurar a velocidade da porta serial
   Recebe: n/a
   Retorna: n/a
*/
void configurarPortaSerial() {
  Serial.begin(115200);
}

/**
   Objetivo: Função responsável por configurar o wifi
   Recebe: n/a
   Retorna: n/a
*/
void configurarWifi() {
  //Configurando o wifi para uso
  WiFi.begin ( SSID, PASSWORD );

  //Aguarda até que mude o status para conectado
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 10 );
  }

  Serial.println(" conectado");
}

/**
   Objetivo: Função responsável por configurar os pinos de entrada e saída
   Recebe: n/a
   Retorna: n/a
*/
void configurarIO() {
  //Configurando entradas digitais


  //Configurando entradas analógicas


  //Configurando saídas digitais
  pinMode(SERVO_MOTOR01, OUTPUT);
  pinMode(SERVO_MOTOR02, OUTPUT);
  pinMode(SERVO_MOTOR03, OUTPUT);


  //Configurando saídas analógicas

}

/**
   Objetivo: Função responsável por anexar os pinos reservados aos servos ao objeto que irá controla-los
   Recebe: n/a
   Retorna: n/a
*/
void configurarServoMotor() {
  servoMotor01.attach(SERVO_MOTOR01);  // anexando o pino do servo do NodeMcu ao meuservo
  servoMotor02.attach(SERVO_MOTOR02);  // anexando o pino do servo do NodeMcu ao meuservo
  servoMotor03.attach(SERVO_MOTOR03);  // anexando o pino do servo do NodeMcu ao meuservo
}

/**
   Objetivo: função responsável por configurar o arduino antes de inicializar a utilização
   Recebe: n/a
   Retorna: n/a
*/
void setup() {
  configurarPortaSerial();

  configurarWifi();

  configurarIO();

  configurarServoMotor();

  fazerLogin(BASE_URL + "autenticacao/login");
}

/**
   Objetivo: Função responsável por movimentar os micro servo motores
   Recebe: o identificador do servo e a posição que ele deve se movimentar
   Retorna: n/a
*/
void movimentarServo(int servoMotor, int posicao) {
  switch (servoMotor) {
    case 1:
      servoMotor01.write(posicao);
      break;
    case 2:
      servoMotor02.write(posicao);
      break;
    case 3:
      servoMotor03.write(posicao);
      break;
  }

  Serial.printf("Servo Motor: %d | Posição: %d\n", servoMotor, posicao);
}


/**
   Objetivo: Função responsável por requisitar dados do servidor backend
   Recebe: o endereço do recurso para onde os dados serão requisitados (rota)
   Retorna: n/a
*/
void lerDados(String recurso) {
  HTTPClient http;

  String rota = recurso + "/" + RECEITA_ID;
  http.begin(rota.c_str());
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", token);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);

    tratarResposta(payload);
  }
  else {
    Serial.print("Código de erro: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

/**
   Objetivo: Função responsável por tratar as respostas vindas do backend
   Recebe: o payload (resposta) e desserializa em variáveis
   Retorna: n/a
*/
void tratarResposta(String payload) {
  StaticJsonDocument<1536> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("A deserializeJson() falhou: ");
    Serial.println(error.c_str());
    return;
  }

  bool sucesso = doc["sucesso"];
  const String mensagem = doc["mensagem"];

  if (sucesso) {
    Serial.print("Mensagem: ");
    Serial.println(mensagem);
    for (JsonObject gerenciamento : doc["gerenciamentos"].as<JsonArray>()) {
      const String  gerenciamento_id                  = gerenciamento["id"];
      const String  gerenciamento_dispensadorId       = gerenciamento["dispensadorId"];
      const String  gerenciamento_pacienteId          = gerenciamento["pacienteId"];
      const String  gerenciamento_receitaId           = gerenciamento["receitaId"];
      const String  gerenciamento_dataHora            = gerenciamento["dataHora"];
      const int     gerenciamento_recipiente          = gerenciamento["recipiente"];
      const String  gerenciamento_status              = gerenciamento["status"];
      const String  gerenciamento_dataHoraConfirmacao = gerenciamento["dataHoraConfirmacao"];
      Serial.print("Gerenciamento Id:");
      Serial.println(gerenciamento_id);
      Serial.print("Dispensador Id:");
      Serial.println(gerenciamento_dispensadorId);
      Serial.print("Paciente Id:");
      Serial.println(gerenciamento_pacienteId);
      Serial.print("Receita Id:");
      Serial.println(gerenciamento_receitaId);
      Serial.print("Data e Hora:");
      Serial.println(gerenciamento_dataHora);
      Serial.print("Recipiente:");
      Serial.println(gerenciamento_recipiente);
      Serial.print("Status:");
      Serial.println(gerenciamento_status);
      Serial.print("Data e Hora da Confirmação:");
      Serial.println(gerenciamento_dataHoraConfirmacao);

      if (DISPENSADOR_ID.equals(gerenciamento_dispensadorId)) {
        comandarDispensador(gerenciamento_recipiente, gerenciamento_id);
      }
    }
  } else {
    Serial.print("Ocorreu um problema. Erro: ");
    Serial.println(mensagem);
  }
}

/**
   Objetivo: Função responsável por comandar o dispensador, indicando o servo que tem que ser acionado
   Recebe: o identificador do servo (recipiente) e o identificador do gerenciamento para atualizá-lo
   Retorna: n/a
*/
void comandarDispensador(int recipiente, String gerenciamentoId) {
  Serial.printf("Acionando recipiente: %d\n", recipiente);
  movimentarServo(recipiente, 180);
  atualizarDados(BASE_URL + "gerenciamentos", gerenciamentoId);
  movimentarServo(recipiente, 0);
}

/**
   Objetivo: Função responsável por enviar atualizações de dados para o servidor backend
   Recebe: o endereço do recurso para onde os dados serão requisitados (rota) e o identificador do gerenciamento
   Retorna: n/a
*/
void atualizarDados(String recurso, String gerenciamentoId) {
  Serial.println("Definindo objetos...");
  StaticJsonDocument<128> doc;
  doc["gerenciamentoId"] = gerenciamentoId;
  doc["dataHoraConfirmacao"] = "2021-12-04T08:00:10.000Z";
  doc["status"] = "OK";

  String corpo = "";

  Serial.println("Serializando...");
  serializeJson(doc, corpo);

  Serial.print("Corpo: ");
  Serial.println(corpo);

  HTTPClient http;

  String serverPath = recurso;

  Serial.print("Rota: ");
  Serial.println(serverPath);
  http.begin(serverPath.c_str());
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", token);

  Serial.println("Disparando requisição de atualização...");
  int httpResponseCode = http.PUT(corpo);

  Serial.print("Codigo de resposta: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);

    tratarResposta(payload);
  }
  else {
    Serial.print("Código de erro: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

/**
   Objetivo: Função responsável por tratar as respostas do processo de login
   Recebe: o payload (resposta) vinda do backend
   Retorna: n/a
*/
void tratarRespostaLogin(String payload) {

  Serial.println("Tratando a resposta do processo de login...");

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("A deserializeJson() falhou: ");
    Serial.println(error.c_str());
    return;
  }

  bool sucesso = doc["sucesso"];
  const String mensagem = doc["mensagem"];

  if (sucesso) {
    Serial.print("Mensagem: ");
    Serial.println(mensagem);
    const String tokenRecebido = doc["token"];
    token = tokenRecebido;
    Serial.print("Token:");
    Serial.println(token);
  } else {
    token = "";
    Serial.print("Ocorreu um problema. Erro: ");
    Serial.println(mensagem);
  }
}

/**
   Objetivo: Função responsável por fazer a autenticação no backend para pegar a autorizaçào de acesso (token)
   Recebe: o endereço do recurso para onde os dados serão requisitados (rota)
   Retorna: n/a
*/
void fazerLogin(String recurso) {

  Serial.println("Fazendo processo de login...");

  StaticJsonDocument<96> doc;

  doc["email"] = PACIENTE_EMAIL;
  doc["senha"] = PACIENTE_SENHA;

  String corpo = "";

  serializeJson(doc, corpo);

  Serial.print("Corpo: ");
  Serial.println(corpo);

  HTTPClient http;

  Serial.print("Rota: ");
  Serial.println(recurso);
  http.begin(recurso.c_str());
  http.addHeader("Content-Type", "application/json");

  Serial.println("Disparando requisição de login...");
  int httpResponseCode = http.POST(corpo);

  Serial.print("Codigo de resposta: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);

    tratarRespostaLogin(payload);
  }
  else {
    Serial.print("Código de erro: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

/**
   Objetivo: Função responsável por fazer o loop (main)
   Recebe: n/a
   Retorna: n/a
*/
void loop() {
  if ((millis() - ultimoLogin) > intervaloLogin) {
    fazerLogin(BASE_URL + "autenticacao/login");
    ultimoLogin = millis();
  }
  if (not token.equals("")) {
    if ((millis() - ultimaAtualizacao) > intervaloAtualizacao) {
      lerDados(BASE_URL + "gerenciamentos");
      ultimaAtualizacao = millis();
    }
  }
}
