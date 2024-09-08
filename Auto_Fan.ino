
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>

//untuk apinya
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// lcd
#include <LiquidCrystal_I2C.h>


#define pinVoltageSensor A0
#define ONE_WIRE_BUS  D3  
#define pinMosfet D4


// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS );

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);


const char *hostname="Automatic Fan Speed";
const char *ssid= "10 Downing Street";
const char *pass= "123456789abcd";

ESP8266WebServer server(80);
String token="d<>[qNGD<Mh-oy9kndU4qLw-$.8*y![T";

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool overRide=false;
bool voltMeter=false;
float vZero=0.0;
// int numberOfSensor=0;



// Serving Hello world
void changeValue(){
  if(!server.hasHeader("Authorization")){
    server.send(401, "text/text", "Not Authorized");
    Serial.println("no auth");
  }else{
    if(!  (server.header("Authorization")==token) ){
      server.send(401, "text/text", "Not Authorized");
      Serial.println("wrong pass");
    }else{
      JsonDocument requestJson;
       DeserializationError error = deserializeJson(requestJson,server.arg("plain"));
       if(error){
        server.send(400, "text/text", "Json not recognize");
       }
      if(!requestJson.containsKey("value")){
        server.send(400, "text/text", "Body contain missing");
      }else{
        float value=requestJson["value"];
        int tipe=requestJson["tipe"];

        overRide=!overRide;
        float counted;
        float displayVoltage;

        if(value > 0){
          overRide=true;
           lcd.setCursor(15, 0);
           lcd.print("R");
          if(tipe==1){ //by voltage
          counted=countValueByVoltage(value); //mosfet value
          analogWrite(pinMosfet, counted);
          displayVoltage=countVoltagebyMosfetValue(counted);




          }else if(tipe==2){ //by mosfet value
            analogWrite(pinMosfet, value);
            displayVoltage=countVoltagebyMosfetValue(value);
          }else if(tipe==3){ //by temperature
            counted=countValuebyTemperature(value);
            analogWrite(pinMosfet, counted);
            displayVoltage=countVoltagebyMosfetValue(counted);

          }else{
            server.send(400, "text/text", "Doesnt have type");
          }

          changeDisplay(displayVoltage);



          
        }else{
          lcd.setCursor(15, 0);
          lcd.print(" ");
          overRide=false;
        }

  








        

        
        
  
        server.send(200, "text/json", "{\"mesage\":\"sucess\"");

      }
    }
  }
}


// Define routing
void restServerRouting() {
    server.on(F("/change"), HTTP_POST, changeValue);
}
 
// Manage not found URL
void handleNotFound() {
  String message = "Not Found\n";
 
  server.send(404, "text/plain", message);
}



float getVoltageAmpereMeter(){
  int raw;
  float newVolt;
  float getVoltage=0.0;
  getVoltage=0.0;
  for (int i=0;i<100;i++){
    raw=analogRead(pinVoltageSensor);
    newVolt=raw*5.0/1024.0;
    getVoltage=0.99*getVoltage + newVolt/100.0;
    delay(5);
  }
  return getVoltage;

}

void setup() {
   // put your setup code here, to run once:

  WiFi.hostname(hostname);
  WiFi.begin(ssid, pass); 
  


  pinMode(pinMosfet, OUTPUT);
  pinMode(pinVoltageSensor, INPUT);

  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("");



  
  sensors.begin();
    
  // numberOfSensor=sensors.getDeviceCount();
  
  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");

  analogWrite(pinMosfet, 0);//9v not too fast or slow

  delay(1000);
  voltMeter=digitalRead(D5);
  if(voltMeter){
    lcd.setCursor(0, 0);
    lcd.print("Connected to");
    lcd.setCursor(0, 1);
    lcd.print("Volt meter");
  }else{
    lcd.setCursor(0, 0);
    lcd.print("Connected to");
    lcd.setCursor(0, 1);
    lcd.print("Ampere Meter");
  }

  delay(1000);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("T:xx.xx");
  
  lcd.print("&xx.xx");
  lcd.write(223);
  lcd.print("C");
  

  lcd.setCursor(0, 1);
  if(voltMeter){
    lcd.print("V:xx.xxv");
  }else{
    lcd.print("V:xx.xxvA:x.xxmA");
  }

  vZero=getVoltageAmpereMeter();
}



void loop() {
  float temperature[2] = {-127, 127};
  float mosfetValue=0;
  float displayVoltage=0;
  server.handleClient();
  sensors.requestTemperatures();

  temperature[0] = sensors.getTempCByIndex(0);
  temperature[1] = sensors.getTempCByIndex(1);

  changeDisplay(temperature);
  if(!overRide){ //kalau sedang tidak dioverride
    mosfetValue=countValuebyTemperature( (temperature[0]+temperature[1])/2.0    )  ; 

    analogWrite(pinMosfet, mosfetValue);

    delay(500);
    if(voltMeter){
    displayVoltage=readVoltage();
    changeDisplay(temperature, displayVoltage);
    }else{
      displayVoltage=countVoltagebyMosfetValue(mosfetValue);
      changeDisplay(temperature, displayVoltage, readAmpere());
    }
  }else{
    changeAmpere(readAmpere());
  }
  


  

  delay(4500);

}

void changeDisplay(float temperature[], float voltage, float ampere){
  changeDisplay(temperature);
  changeDisplay(voltage);
  changeAmpere(ampere);
}

void changeDisplay(float temperature[], float voltage){ //untuk ampere
  changeDisplay(temperature);
  changeDisplay(voltage);
}

void changeAmpere(float ampere){ //untuk ampere
  lcd.setCursor(10,1);
  lcd.print("    ");//buat clear
  lcd.setCursor(10,1);
  lcd.print(ampere);
}

void changeDisplay(float voltage){ //untuk voltage
  lcd.setCursor(2, 1);
  lcd.print("      ");//buat clear
  lcd.setCursor(2, 1);
  lcd.print(voltage);
  lcd.print("V");
}

void changeDisplay(float temperature[]){ //untuk temperature
  lcd.setCursor(2, 0);
  if(temperature[0]==-127){
    lcd.print("00,00");
  }else{
    lcd.print(temperature[0]);
  }

  lcd.setCursor(8, 0);
  if(temperature[1]==-127){
    lcd.print("00,00");
  }else{
    lcd.print(temperature[1]);
  }
}






int countValueByVoltage(float voltage){ //untuk kalau override
  // bye count turun 1 value turun 0.05v
    if(voltage>=12){
      return 225;
    }else{
      float multiplier = (12.0-voltage)/0.05;
    return 240 - multiplier;
    }
}
int countValuebyTemperature(float temperature){
  //expected
  //temp voltage
  //25 3.3
  //45 12
  if(temperature<25){
    return 65; //tested manual voltage  will be 3.37 -> the close point to 3.33
  }else{
    return ((12.6*temperature)-250);//more precomputed
  }
  //line1
  // float voltage=(0.348*temperature)-3.66; //precomputed
  // float res=(255*voltage)/12.0;
  // return res;

  //new method more efficient
}

float countVoltagebyMosfetValue(int mosfetValue){
  if(mosfetValue >240){
    return 12.00;
  }else{
    return 12.0-( (240-mosfetValue)*0.05); //based on manual test
  }
}






float readVoltage(){
  float tempVoltage=0;
int analogVoltage=0.0;

  for(int i=0;i<10;i++){
    analogVoltage=analogRead(pinVoltageSensor);
    tempVoltage+=((analogVoltage*16.5)/1024.0);
    delay(100);
  }

  return (tempVoltage/10.0);
}

float readAmpere(){
  Serial.print("Vzero:");
  Serial.print(vZero);
  float currVoltage=getVoltageAmpereMeter();
  Serial.print(" current: ");
  Serial.println(currVoltage);
  return (currVoltage-vZero)*10;
}




  






