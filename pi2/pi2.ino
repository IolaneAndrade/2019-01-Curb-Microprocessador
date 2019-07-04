#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

const int NIVEL_1 = 4;
const int NIVEL_2 = 22;
const int NIVEL_3 = 23;
const int RELE = 16;
const int TIMEZONE = -3;
const char* ssid = "MrVictor";
const char* password = "2334445555";

WiFiUDP udp;

static double nivel_tinta_atual = 100.0;
static double nivel_bateria = 100.0;
static int contador_bateria = 0;

struct Date {
    
    int diaDaSemana;
    int dia;
    int mes;
    int ano;
    int hora;
    int minutos;
    int segundos;
};

NTPClient ntpClient(
    
    udp,
    "0.br.pool.ntp.org",
    TIMEZONE * 3600,
    60000
);

Date getDate() {

    char* strDate = (char*)ntpClient.getFormattedDate().c_str();

    Date date;
    sscanf(strDate, "%d-%d-%dT%d:%d:%dZ", &date.ano, &date.mes, &date.dia, &date.hora, &date.minutos, &date.segundos);

    return date;
}

String dataMonitoramento(Date date) {

    String diaDaSemana = "";

    String dia = String(date.dia);
    String mes = String(date.mes);
    String ano = String(date.ano);

    diaDaSemana = dia + "/";
    diaDaSemana += mes + "/";
    diaDaSemana += ano;
    
    return diaDaSemana;                
}

String horaMonitoramento(Date date) {

    String horario = "";

    String hora = String(date.hora);
    String minutos = String(date.minutos);
    String segundos = String(date.segundos);

    horario = hora + ":";
    horario += minutos;

    return horario;
}


int nivel_tinta() {

    int nivel_atual_tinta = 0;
//    int nivel_1 = digitalRead(NIVEL_1);
//    int nivel_2 = digitalRead(NIVEL_2);
//    int nivel_3 = digitalRead(NIVEL_3);
      int nivel_1 = 1;
      int nivel_2 = 1;
      int nivel_3 = 1;


    if(nivel_1 == 0 && nivel_2 == 0 && nivel_3 == 0) {
      
        nivel_atual_tinta = 0;
    } else if(nivel_1 == 1 && nivel_2 == 0 && nivel_3 == 0) {
      
        nivel_atual_tinta = 33;
    } else if(nivel_1 == 1 && nivel_2 == 1 && nivel_3 == 0) {
      
        nivel_atual_tinta = 63;
    } else if(nivel_1 == 1 && nivel_2 == 1 && nivel_3 == 1) {
      
        nivel_atual_tinta = 100;
    }

   return nivel_atual_tinta;
}

void setup() {

    Serial.begin(115200);
    
    Serial.println();
    Serial.println();
    Serial.print("Conectando a: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi conectado.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    ntpClient.begin();

    while(!ntpClient.update()) {

        Serial.println(".");
        ntpClient.forceUpdate();
        delay(500);
    }

    pinMode(NIVEL_1 , INPUT);
    pinMode(NIVEL_2 , INPUT);
    pinMode(NIVEL_3 , INPUT);
    pinMode(RELE, OUTPUT);    
}

void loop() {

    if ((WiFi.status() == WL_CONNECTED)) { 

        int status_curb = 0;
        HTTPClient httpGet;
     
        httpGet.begin("https://www.jsonstore.io/6ab2d2053ab011dea0384adc74c574ac48fd77f06bcd69b8f7e321fc902fcca8"); 
        int httpCode = httpGet.GET();  
     
        if (httpCode > 0) {
     
            String payload = httpGet.getString();
            StaticJsonDocument<256> json;

            DeserializationError error = deserializeJson(json, payload);
            
            if (error) {
              
                Serial.print(F("deserializeJson() falhou: "));
                Serial.println(error.c_str());
                return;
            }

            status_curb = json["result"]["status_carrinho"].as<int>();
            Serial.println(status_curb);

            if(status_curb == 1) {

                digitalWrite(RELE, HIGH);

                String dados;                
                StaticJsonDocument<256> json;
                Date date = getDate();                          

                String id = json["curbAtivo"] = "1"; 
                String tinta = json["tinta"] = nivel_tinta();
                String bateria = json["bateria"] = nivel_bateria;
                String dia = json["data"] = dataMonitoramento(date);
                String hora = json["hora"] = horaMonitoramento(date);
                String latitude_inicial = json["latitudeInicial"] = "-15.98930198";
                String longitude_inicial = json["logitudeInicial"] = "-48.0446291";
                String latitude_final = json["latitudeFinal"] = "-15.99231874";
                String longitude_final = json["logitudeFinal"] = "-48.04943562";
                String status_carrinho = json["status"] = "true";
            
                serializeJson(json, dados);
                Serial.println(dados);
         
                HTTPClient httpPost;
                contador_bateria += 1;

                if(contador_bateria == 8) {

                    httpPost.begin("http://gustavo2795.pythonanywhere.com/monitoramentos/");      
                    httpPost.addHeader("Content-Type", "application/json");  

                    nivel_bateria -=20;
                    int bateria = json["bateria"] = nivel_bateria;
                    int httpCode = httpPost.POST(dados);   
                 
                    httpPost.end();
                    delay(15000);
                    contador_bateria = 0; 
                } else {

                    httpPost.begin("http://gustavo2795.pythonanywhere.com/monitoramentos/");      
                    httpPost.addHeader("Content-Type", "application/json");  

                    int httpCode = httpPost.POST(dados);   
                 
                    httpPost.end();
                    delay(15000); 
                }  
            } else {

                digitalWrite(RELE, LOW);

                String dados;                
                StaticJsonDocument<256> json;
        
                String id = json["curbAtivo"] = "0"; 
                String tinta = json["tinta"] = "";
                String bateria = json["bateria"] = "";
                String dia = json["data"] = "";
                String hora = json["hora"] = "";
                String latitude_inicial = json["latitudeInicial"] = "";
                String longitude_inicial = json["logitudeInicial"] = "";
                String latitude_final = json["latitudeFinal"] = "";
                String longitude_final = json["logitudeFinal"] = "";
                String status_carrinho = json["status"] = "false";
            
                serializeJson(json, dados);
                Serial.println(dados);
         
                HTTPClient httpPost;    
             
                httpPost.begin("http://gustavo2795.pythonanywhere.com/monitoramentos/");      
                httpPost.addHeader("Content-Type", "application/json");  
             
                int httpCode = httpPost.POST(dados);   
             
                httpPost.end();
                delay(2000); 
            }
        }
        else {
            Serial.println("WiFi Desconectada");
        }
     
        httpGet.end(); 
    }
     
    delay(1000);
}
