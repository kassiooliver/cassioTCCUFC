#define TINY_GSM_MODEM_SIM800  //define qual modulo sim iremos utilizar
#include <Adafruit_GFX.h>  //biblioteca tela oled
#include <Adafruit_SSD1306.h> //biblioteca tela oled
#include <axp20x.h>  //biblioteca tela oled
#include <TinyGPS++.h>  //biblioteca gps
#include <PubSubClient.h> //biblioteca do mqtt
#include <TinyGsmClient.h> //biblioteca gsm



///Seriais utilizadas
HardwareSerial GPS(1); //Inicializando o modulo GPS do ESP na primeira Hardware Serial
HardwareSerial SIM800(0); //Inicializando o modulo SIM800L do ESP na serial zero

int rele_bomba = 4; //4 //Pino utilizado pelo rele para acionamento da bomba de combustivel


TinyGPSPlus gps; //Instaciamento do objeto GPS
TinyGsm modemGSM(SIM800);//Instaciamento do objeto SIM800L
TinyGsmClient gsmClient(modemGSM); //Instaciamento do modem GSM
PubSubClient mqtt(gsmClient); //Instanciamento do broker mqtt

//Instaciamento do display do ESP32
AXP20X_Class axp;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
/**********/

//configuracoes de conexao a rede 2G da Operadora Oi
const char apn[]  = "oi.com.br"; //APN OI
const char user[] = "oi"; //Usuario
const char pass[] = "oi"; //Senha

// configuracoes do mqtt
const char* broker = "6.tcp.ngrok.io";         //endereco do broker mqtt
const char* topicOut = "mqtt_tcc/rastreio";    //Topico de saida do broker
const char* topicIn = "mqtt_tcc/bloqueio";     //topico de entrada do broker


char buffer[100];
char buffer2[100];

char statusbloqueio[30] = "VEÍCULO HABILITADO A LIGAR" ;
char statusbloqueio2[30] = "VEICULO DESABILITADO A LIGAR" ;




/*************************************************************************************************************/


void setup()
{

  
  pinMode(rele_bomba, OUTPUT); //Inicializamos o pino que acionara o rele da bomba de combustivel

  //Configuracao do display
   Wire.begin(21, 22);
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
  //  Serial.println("AXP192 Begin PASS");
  } else {
  //  Serial.println("AXP192 Begin FAIL");
  }
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
   for(;;);
  }
  delay(2000);
  display.clearDisplay(); 
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.clearDisplay();
/*****************/
  GPS.begin(9600, SERIAL_8N1, 34, 12);   //Inicializacao do GPS
  SIM800.begin(115200, SERIAL_8N1, 1, 3);   //Inicializacao do SIM800L
  
  modemGSM.restart();
  if(!modemGSM.waitForNetwork())
  {
    while(true);
  }
  display.println("Qual do sinal: " + String(modemGSM.getSignalQuality())+"%");
  display.println("Conectando ao servidor");
  
 if (!modemGSM.gprsConnect(apn, user, pass))
  {

    while(true);
  }

  mqtt.setServer(broker, 18059);
  mqtt.setCallback(mqttCallback);

  display.println("End. IP Rede GSM: ");
  display.println(modemGSM.localIP());

  display.println();
  display.display();

}

/*************************************************************************/

void loop()
{ 

  char *valorLat = dtostrf(gps.location.lat(),6,6,buffer); //Armazenamos em um vetor a latitude
  char *valorLon = dtostrf(gps.location.lng(),6,6,buffer2);//Armazenamos em um vetor a longitude
  char *estadoIgnicao;
  
  if (digitalRead(rele_bomba) == LOW)
{
     estadoIgnicao = statusbloqueio;
} else estadoIgnicao = statusbloqueio2;


  char buffer3[100] = "";
  strcat(buffer3, valorLat);
  strcat(buffer3, ",");
  strcat(buffer3, valorLon);
  strcat(buffer3, ",");
  strcat(buffer3, estadoIgnicao);
  
  String message = buffer3;
  
  mqtt.publish(topicOut, message.c_str());
  
  mqttConnect();
  
  smartDelay(1000); 
  
  mqtt.loop();
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (GPS.available())
      gps.encode(GPS.read());
  } while (millis() - start < ms);
} 

void mqttCallback(char* topic, byte* payload, unsigned int len)
{
 payload[len] = '\0'; 
 String message = (char*)payload; 
 if(message == "1"){
    digitalWrite(rele_bomba, HIGH);
    //veículo desligado
  }
  if(message == "0"){
    digitalWrite(rele_bomba, LOW);
    //veículo ligado"
  } 
}


void mqttConnect()
{
   while (!mqtt.connected()) 
    {
        if (mqtt.connect("")) 
        {
             mqtt.subscribe(topicIn);
             mqtt.connected();
        } 
    }
}
