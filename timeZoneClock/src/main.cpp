#include <ESP8266WiFi.h>
//#include <WiFiClient.h>    //representa la conexión a internet
//#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>


const char* ssid = "****";
const char* password =  "****";
const char* mqttServer = "broker.mqtt-dashboard.com";
const int mqttPort = 1883;
const char* mqttUser = "****";
const char* mqttPassword = "****";


const char* input_topic = "input";
const char* output_topic = "output";
const char* alive_topic = "alive";
const char* status_request = "StatusRequest";
const char* Json = "JsonStatus";


//const String api= "http://worldtimeapi.org/api/timezone/Europe/England";
const String api= "http://worldtimeapi.org/api/timezone/";
String zonaHoraria;


//inicializa el cliente wifi
WiFiClient espClient;                      // espClient. El constructor se utiliza para inicializar los valores de las propiedades del objeto
//inicializa el cliente MQTT
PubSubClient client(espClient);            // objeto + nombre. que se utiliza para conectarse y comunicarse con un broker MQTT
WiFiClient clientHttp;





/********* Setup wifi ***************************
   setup wifi connect to wifi with the constants
   defined up
   while does not connect print a "."
   if connect then print the local ip
************************************************/
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print(F("Connecting to ")) ;
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());     //WiFi es la clase y localIP es el método
}


String obtenerDiaLetras(int numero){
  switch (numero) {
  case 0:
    return "Domingo";
    break;
  case 1:
    return "Lunes";
    break;
  case 2:
    return "Martes";
    break;
  case 3:
    return "Miercoles";
    break;
  case 4:
    return "Jueves";
    break;
  case 5:
    return "Viernes";
    break;
  case 6:
    return "Sabado";
    break;
  default:
    return "Dia";
    break;
}
}
String obtenerMesLetras(String numero){
  if(numero=="01"){
    return "Enero";
  }else if (numero=="02"){
    return "Febrero";
  }else if (numero=="03"){
    return "Marzo";
  }else if (numero=="04"){
    return "Abril";
  }else if (numero=="05"){
    return "Mayo";
  }else if (numero=="06"){
    return "Junio";
  }else if (numero=="07"){
    return "Julio";
  }else if (numero=="08"){
    return "Agosto";
  }else if (numero=="09"){
    return "Septiembre";
  }else if (numero=="10"){
    return "Octubre";
  }else if (numero=="11"){
    return "Noviembre";
  }else if (numero=="12"){
    return "Diciembre";
  }else{
    return "Mes";
  }
}


String obtenerAnio(String cadena){
   char str[cadena.length()+1];
   unsigned int i=0;
   for (i=0;i<cadena.length();i++) {
      //Serial.println((char)cadena[i]);
      if((char)cadena[i] != '-'){
        str[i]=(char)cadena[i];
      }else {
        break;
      }

    }
    str[i] = 0; // Null termination
    Serial.println(str);
    return str;
}

String obtenerMes(String cadena){
  return cadena.substring(5,7);;
}

String obtenerDiaNumero(String cadena){
  return cadena.substring(8,10);
}

String obtenerHora(String cadena){
  return cadena.substring(11,16);
}

void adjuntarOutput(String diaLetras, String anio, String diaNumero, String mes, String hora){

String cadena;

  cadena = diaLetras + ", " + diaNumero + " de " + mes + " de " + anio + " -- " + hora;
  
  Serial.println(cadena);
  client.publish(output_topic, cadena.c_str());
  
}


void http_Api(String api, String zonaHoraria){

    HTTPClient http;
    if (http.begin(clientHttp, api+zonaHoraria)) { 
    
    int httpCode = http.GET();  //Realizamos la petición
    String codigoStr = String(httpCode);
    
         
        if (httpCode > 0) { //código de retorno

           Serial.println(httpCode); // esperamos que sea 200
           if(httpCode==200){
            client.publish(status_request,"HTTP OK");
           }
           //client.publish(TopicStatusRequest,codigoStr.c_str());
           String data = http.getString();
            client.publish(Json,"JSON OK");
           
            
            Serial.println(data);
    
            StaticJsonDocument <256> doc;
            deserializeJson(doc,data);
    
            // deserializeJson(doc,str); can use string instead of payload
            const char* datetime = doc["datetime"];
            int day_of_week = doc["day_of_week"];
          
            Serial.print("datetime: ");
            Serial.println(datetime);
            
            Serial.println("day_of_week: "+day_of_week);
            adjuntarOutput(obtenerDiaLetras(day_of_week),obtenerAnio(datetime),obtenerDiaNumero(datetime),obtenerMesLetras(obtenerMes(datetime)),obtenerHora(datetime));
    
    
            
            //client.publish(TopicOutput,data);
          }
          else {
            Serial.println("Error en la petición HTTP");
          }
 
    http.end(); //cerramos conexión y liberamos recursos
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  
    Serial.println("callback se esta ejecutando: ");
    

    char str[length+1];
    Serial.print("topico: ");
    Serial.println(topic);
    Serial.print("mensaje: ");
    unsigned int i=0;
    for (i=0;i<length;i++) {
      Serial.print((char)payload[i]);
      str[i]=(char)payload[i];
    }
    Serial.println();
   
   
    str[i] = 0; // Null termination
    zonaHoraria = str;
    Serial.println("zona horaria: "+zonaHoraria);
    http_Api(api,zonaHoraria);

}


void setup() {
 
  //Start Serial Communication
  Serial.begin(115200);
  
  //Connect to WiFi
  setup_wifi();

  //Connect to MQTT Broker
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  //MQTT Connection Validation
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("uco")) {
 
      Serial.println("connected");  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
  
  //Publish to desired topic and subscribe for messages
  client.publish(alive_topic, "Es un Exito");
  client.subscribe(input_topic);
 
}

 
void loop() {
  //MQTT client loop
  client.loop();
}