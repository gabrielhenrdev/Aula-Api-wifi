#include <Arduino.h>
#include <WiFi.h>
#include <ezTime.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// funções
void conexaoWIFI();
void reconexaoWIFI();
void exibirData();
void exibirLCD();
void atualizarLinhaLCD(int linha, String conteudo);

// WiFi
const char *SSID = "SENAI EDUCACIONAL";
const char *SENHA = "info@134";

const unsigned long tempoConexao = 10000;
const unsigned long tempoReconexao = 5000;
unsigned long tempoAnterior = 0;
const unsigned long intervalo = 10000;

String endAPI = "https://wttr.in/Sao+Caetano+do+Sul?lang=pt&format=";

// Api
float temp, sensacaoterm;
int umi, pressao;
String condicao, nascerSol;

String consAPI(String cod);

JsonDocument doc;

int minuto;
int segundo;
int hora;
int hora12;
int dia;
int diaAno;
int mes;
int ano;

bool am;
bool pm;

// Y -> Ano completo (2026)
// y -> Ano com 2 dígitos (26)
// m -> Mês (01-12)
// d -> Dia (01-31)
// H -> Hora (00-23)
// i -> Minuto (00-59)
// s -> Segundo (00-59)
// l -> Dia da semana por extenso (Thursday)
// D -> Dia da semana abreviado (Thu)
// F -> Nome completo do mês (November)
// M -> Nome abreviado do mês (Nov)
// a/A -> am/pm
// T -> Sigla do fuso (BRT)
// z -> Dia do ano

Timezone sp;

unsigned long tempoultimaConexao = 0;

const char *meses[] = {
    "Janeiro", "Fevereiro", "Março", "Abril",
    "Maio", "Junho", "Julho", "Agosto",
    "Setembro", "Outubro", "Novembro", "Dezembro"
};

const char *diasSemana[] = {
    "domingo",
    "segunda-feira",
    "terça-feira",
    "quarta-feira",
    "quinta-feira",
    "sexta-feira",
    "sábado"
};

LiquidCrystal_I2C lcd(0x27, 20, 4);

byte umidade[8] = {
    B00000,
    B00100,
    B01110,
    B11111,
    B11111,
    B11111,
    B11111,
    B01110
};

String linhasAnteriores[4] = {"", "", "", ""};

void setup() {

    Serial.begin(115200);

    Wire.begin(21, 22);
    lcd.init();
    lcd.backlight();
    lcd.createChar(0, umidade);
    lcd.clear();

    conexaoWIFI();

    
    if (WiFi.status() == WL_CONNECTED) {
        sp.setLocation("America/Sao_Paulo");
        
        waitForSync();

        minuto = sp.minute();
        segundo = sp.second();
        hora = sp.hour();
        hora12 = sp.hourFormat12();
        dia = sp.day();
        diaAno = sp.dayOfYear();
        mes = sp.month();
        ano = sp.year();

        am = sp.isAM();
        pm = sp.isPM();

        exibirData();
    }
}

void loop() {

    reconexaoWIFI();

    unsigned long tempoAtual = millis();

    if (tempoAtual - tempoAnterior >= intervalo) {

        tempoAnterior = tempoAtual;

        temp = consAPI("%t").toFloat();
        sensacaoterm = consAPI("%f").toFloat();
        umi = consAPI("%h").toInt();
        pressao = consAPI("%P").toInt();
        condicao = consAPI("%C");
        nascerSol = consAPI("%S");

        JsonObject clima = doc["clima"].to<JsonObject>();

        clima["temperatura"] = temp;
        clima["sensacao_termica"] = sensacaoterm;
        clima["umidade"] = umi;
        clima["pressao"] = pressao;
        clima["condicao"] = condicao;
        clima["nascer_sol"] = nascerSol;

        Serial.println("Temp: " + String(temp));
        Serial.println("Sensacao: " + String(sensacaoterm));
        Serial.println("Umidade: " + String(umi));
        Serial.println("Pressao: " + String(pressao));
        Serial.println("Condicao: " + condicao);
        Serial.println("Nascer sol: " + nascerSol);
        Serial.println("--------------");

    }

    exibirLCD();
}

void conexaoWIFI() {

    Serial.printf("Conectando ao WiFi: %s\n", SSID);

    WiFi.begin(SSID, SENHA);

    unsigned long tempoWifi = millis();
    unsigned long tempoPonto = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - tempoWifi < tempoConexao) {

        if (millis() - tempoPonto >= 500) {

            tempoPonto = millis();

            Serial.print("-");
        }
    }

    if (WiFi.status() == WL_CONNECTED) {

        Serial.println("\nConectado ao WiFi!");

        Serial.print("Endereco IP: ");
        Serial.println(WiFi.localIP());

    } else {

        Serial.println("\nFalha ao se conectar.");
    }
}

void reconexaoWIFI() {

    unsigned long tempoAtual = millis();

    if (tempoAtual - tempoultimaConexao >= tempoReconexao) {

        tempoultimaConexao = tempoAtual;

        if (WiFi.status() != WL_CONNECTED) {

            Serial.println("Voce perdeu a conexao!");

            conexaoWIFI();

            if (WiFi.status() == WL_CONNECTED) {

                waitForSync();
            }
        }
    }
}

void exibirData() {

    int diaSemana = sp.weekday();

    int diasRestantes = 365 - diaAno;

    JsonObject data = doc["data_coleta"].to<JsonObject>();

    data["dia"] = dia;
    data["mes"] = meses[mes - 1];
    data["ano"] = ano;
    data["dia_semana"] = diasSemana[diaSemana - 1];
    data["dias_restantes"] = diasRestantes;

    Serial.print("Hoje e ");
    Serial.print(dia);
    Serial.print(" de ");
    Serial.print(meses[mes - 1]);
    Serial.print(" de ");
    Serial.print(ano);

    Serial.print(", ");
    Serial.print(diasSemana[diaSemana - 1]);

    Serial.print(" e faltam ");
    Serial.print(diasRestantes);
    Serial.println(" dias para o ano novo.");
}

void atualizarLinhaLCD(int linha, String conteudo) {

    if (conteudo.length() > 20) {
        conteudo = conteudo.substring(0, 20);
    }

    if (linhasAnteriores[linha] != conteudo) {

        lcd.setCursor(0, linha);
        lcd.print("                    ");

        lcd.setCursor(0, linha);

        if (linha == 2) {

            lcd.write(byte(0));
            lcd.print(conteudo.substring(1));

        } else {

            lcd.print(conteudo);
        }

        linhasAnteriores[linha] = conteudo;
    }
}

void exibirLCD() {

    String linha1 =
        String(sp.day()) + "/" +
        String(sp.month()) + "/" +
        String(sp.year()) + " " +
        String(sp.hour()) + ":" +
        (sp.minute() < 10 ? "0" : "") + String(sp.minute()) + ":" +
        (sp.second() < 10 ? "0" : "") + String(sp.second());

    String linha2 =
        "T:" + String(temp, 0) +
        "C ST:" + String(sensacaoterm, 0) + "C";

    String linha3 =
        ":" + String(umi) +
        "% P:" + String(pressao) + "hPa";

    String linha4 = condicao + " " + nascerSol;

    atualizarLinhaLCD(0, linha1);
    atualizarLinhaLCD(1, linha2);
    atualizarLinhaLCD(2, linha3);
    atualizarLinhaLCD(3, linha4);
}

String consAPI(String cod) {

    HTTPClient http;

    http.begin(endAPI + cod);

    int codigohttp = http.GET();

    String resposta = "";

    if (codigohttp == 200) {

        resposta = http.getString();

    } else {

        Serial.print("Erro, Cod: " + String(codigohttp));
    }

    http.end();

    return resposta;
}