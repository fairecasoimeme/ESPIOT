/**
 * ESPIOT
 *
 *  Created on: 21.05.2016
 *
 * VERSION 1.01a
 * CHANGELOG
 * 2017-01-29
 */
#include <stdint.h>
#include <HardwareSerial.h>
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

//Library allows either I2C or SPI, so include both.
#include "SparkFunBME280.h"
#include "Wire.h"
#include "SPI.h"

#include <ArduinoJson.h>
#include "FS.h"

extern "C" {
#include <user_interface.h>
}

#define FPM_SLEEP_MAX_TIME 0xFFFFFFF

/* DEFINES
 *  
 */

#define DONEPIN 14
/* 
 *  VARIABLES GLOBALES
 */
  String VERSION="1.01a";
  const char* WIFISSIDValue;
  const char* WIFIpassword;
  String urlServer ;
  const char* HTTPUser;
  const char* HTTPPass ;
  const char* jsonIp;
  char ip1,ip2,ip3,ip4;
  char mask1,mask2,mask3,mask4;
  char gw1,gw2,gw3,gw4;
  
  bool connexionOK=false;
  bool configOK=false;

  bool cmdComplete=false;
  String SerialCMD;

  int retryConnexionOk,retryWifi;
  int countConnexion=0;
  bool EndCheck=false;
  bool ToReboot=false;
  char wificonfig[512];
  
  IPAddress ip,gateway,subnet;
  
  uint8_t MAC_array[6];
  char MAC_char[18];
  
  BME280 mySensor;
  ESP8266WebServer server(80);
  char serverIndex[512]="<h1>ESPIOT Config</h1><ul><li><a href='params'>Config ESPIOT</a></li><li><a href='update'>Flash ESPIOT</a></li></ul><br><br>Version: 1.01a<br><a href='https://github.com/fairecasoimeme/' target='_blank'>Documentation</a>";
 
  char serverIndexUpdate[256] = "<h1>ESPIOT Config</h1><h2>Update Firmware</h2><form method='POST' action='/updateFile' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
  char serverIndexConfig[1024] = "<h1>ESPIOT Config</h1><h2>Config ESPIOT</h2><form method='POST' action='/config'>SSID : <br><input type='text' name='ssid'><br>Pass : <br><input type='password' name='pass'><br>@IP : <br><input type='text' name='ip'><br>Mask : <br><input type='text' name='mask'><br>@GW : <br><input type='text' name='gw'><br>Http user : <br><input type='text' name='userhttp'><br>Http pass : <br><input type='text' name='passhttp'><br>URL : <br><input type='text' name='url'><br><input type='submit' value='OK'></form>";
 
  const char* Style = "<style>body {  text-align: center; font-family:Arial, Tahoma;  background-color:#f0f0f0;}ul li { border:1px solid gray;  height:30px;  padding:3px;  list-style: none;}ul li a { text-decoration: none;}input{ border:1px solid gray;  height:25px;}input[type=text] { width:150px;  padding:5px;  font-size:10px;}#url {  width:300px;}</style>";


/* FONCTION POUR CHARGER LA CONFIG
 *  
 *  
 */
bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }
  
  
  if (json.containsKey("WIFISSID")) 
  {
    WIFISSIDValue = json["WIFISSID"];
  }
  if (json.containsKey("WIFIpass")) 
  {
    WIFIpassword = json["WIFIpass"];
  }
  if (json.containsKey("Ip")) 
  {
    jsonIp = json["Ip"];
    ip.fromString(jsonIp);
  }
  if (json.containsKey("Mask")) 
  {
    jsonIp = json["Mask"];
    subnet.fromString(jsonIp);
  }
  if (json.containsKey("GW")) 
  {
    jsonIp = json["GW"];
    gateway.fromString(jsonIp);
  }
   if (json.containsKey("urlServer")) 
  {
     urlServer = json["urlServer"].asString();
  }
  if (json.containsKey("HTTPUser")) 
  {
     HTTPUser = json["HTTPUser"].asString();
  }
  if (json.containsKey("HTTPPass")) 
  {
     HTTPPass = json["HTTPPass"].asString();
  }

  uint8_t tmp[6];
  char tmpMAC[18];
  WiFi.macAddress(tmp);
  for (int i = 0; i < sizeof(tmp); ++i){
    sprintf(tmpMAC,"%s%02x",tmpMAC,tmp[i]);
  }
  json["ID"] =  String(tmpMAC);
  json.printTo(wificonfig,sizeof(wificonfig));
  //sprintf(serverIndexConfig,"<h1>ESPIOT Config</h1><h2>Config ESPIOT : %s</h2><form method='POST' action='/config'>SSID : <br><input type='text' name='ssid' value='%s'><br>Pass : <br><input type='password' name='pass'><br>@IP : <br><input type='text' name='ip' value='%s'><br>Mask : <br><input type='text' name='mask' value='%s'><br>@GW : <br><input type='text' name='gw' value='%s'><br>Http user : <br><input type='text' name='userhttp' value='%s'><br>Http pass : <br><input type='text' name='passhttp' value='%s'><br>URL : <br><input type='text' name='url' value='%s'><br><input type='submit' value='OK'></form>",json["ID"].asString(),json["WIFISSID"].asString(),json["Ip"].asString(),json["Mask"].asString(),json["GW"].asString(),json["HTTPUser"].asString(),json["HTTPPass"].asString(),json["urlServer"].asString());
 sprintf(serverIndexConfig,"<html><head></head><body><h1>ESPIOT Config</h1><h2>@MAC : %s</h2><form method='POST' action='/config'>SSID : <br><input type='text' name='ssid' value='%s'><br>Pass : <br><input type='password' name='pass'><br>@IP : <br><input type='text' name='ip' value='%s'><br>Mask : <br><input type='text' name='mask' value='%s'><br>@GW : <br><input type='text' name='gw' value='%s'><br>URL : <br><input type='text' name='url' value='%s' style='width:300px;'><br>Http user : <br><input type='text' name='userhttp' value='%s'><br>Http pass : <br><input type='text' name='passhttp' value='%s'><br><br><input type='submit' value='OK'></form></body></html>",json["ID"].asString(),json["WIFISSID"].asString(),json["Ip"].asString(),json["Mask"].asString(),json["GW"].asString(),json["urlServer"].asString(),json["HTTPUser"].asString(),json["HTTPPass"].asString());

  return true;
}

void PowerDown()
{
  digitalWrite(DONEPIN,HIGH);
  delay(1);
  digitalWrite(DONEPIN,LOW);
  serverWebCfg();

}

void early_init() {
  //WiFi.persistent(false);
 // WiFi.mode(WIFI_OFF);
 // WiFi.forceSleepBegin();
//  wifi_station_disconnect();
//  wifi_set_opmode(NULL_MODE);
//  wifi_set_sleep_type(MODEM_SLEEP_T);
//  wifi_fpm_open();
//  wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
}

float readADC_avg()
{
  int battery=0;
  for (int i=0;i<10;i++)
  {
      battery = battery + analogRead(A0);
  }
  return (((battery/10)*4.2)/1024);
  
}

void setup() {
    //WiFi.mode(WIFI_OFF);
    //WiFi.forceSleepBegin();
    pinMode(DONEPIN,OUTPUT);
    digitalWrite(DONEPIN,LOW);
   /* WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();*/
    Serial.begin(9600);
    Serial.setDebugOutput(true);
  
    if (!SPIFFS.begin()) {
      Serial.println("Failed to mount file system");
      return;
    }

    if (!loadConfig()) {
      Serial.println("Failed to load config");
    } else {
      configOK=true;
      Serial.println("Config loaded");
    }

    
    if (configOK)
    {
     // WiFi.forceSleepWake();
     // wifi_fpm_do_wakeup();
     // wifi_fpm_close();
     // wifi_set_opmode(STATION_MODE);
     // wifi_station_connect();
     WiFi.mode(WIFI_STA);
     if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(WIFISSIDValue, WIFIpassword);
       // WiFi.config(ip, gateway, subnet); 
     }
     
      WiFi.macAddress(MAC_array);
      for (int i = 0; i < sizeof(MAC_array); ++i){
        sprintf(MAC_char,"%s%02x:",MAC_char,MAC_array[i]);
      }
      Serial.println(WiFi.localIP());

      Wire.begin(4,5);
      mySensor.settings.commInterface = I2C_MODE;
      mySensor.settings.I2CAddress = 0x76;
      mySensor.settings.runMode = 3;
      mySensor.settings.tStandby = 0;
      mySensor.settings.filter = 0;
      mySensor.settings.tempOverSample = 1;
      mySensor.settings.humidOverSample = 1;
      mySensor.settings.pressOverSample = 1;
      delay(10);
      mySensor.begin();
      
      
    }else{
      setupWifiAP();
      Serial.println("Config not ok");
      
    }
   retryWifi=0;  
   retryConnexionOk=0;
   countConnexion=0;
   EndCheck=false;
   ToReboot=false;

}



void loop() 
{
   server.handleClient();
   if (ToReboot)
   {
     ESP.restart();
   }
    // wait for WiFi connection
   if (!EndCheck)
   {
      if (WiFi.status() != WL_CONNECTED) 
      {
       countConnexion=0;
        if (retryWifi <40)
        {
          delay(500);
           Serial.print(".");
          retryWifi++;
        }else{
          EndCheck=true;
          PowerDown();
          setupWifiAP();
          serverWebCfg();
          Serial.println("délai dépassé");
        }
      }else{
        connexionOK=true;
        EndCheck=true;
        Serial.println("Connexion OK"); 
      }
   }
   
   if ((connexionOK) && (countConnexion==0))
   {

       float humidity, temp, pression, tension;   
        for (int i = 0; i < 3; ++i){
        
          humidity=mySensor.readFloatHumidity();
          temp=mySensor.readTempC();
          pression=mySensor.readFloatPressure();
          tension=readADC_avg();
          Serial.print("temp:");
          Serial.print(temp);
          Serial.print("hum:");
          Serial.print(humidity);
          Serial.print("tension:");
          Serial.print(tension);
          if (String(temp)!="nan")
          {
            break;  
          }
          delay(2);
        }
        
        if (retryConnexionOk<3)
        {
            connexionOK=false;
            HTTPClient http;
           
            // configure traged server and url
            http.begin(urlServer); //HTTP
            Serial.print("[HTTP] begin...\n");
            //Serial.print(urlServer);
            
            if ((HTTPUser != "") || (HTTPPass != ""))
            {
              http.setAuthorization(HTTPUser, HTTPPass);
            }
            Serial.print("[HTTP] GET...\n");
            // start connection and send HTTP header
            http.addHeader("Content-Type", "application/x-www-form-urlencoded");
            int httpCode = http.POST("id="+String(MAC_char)+"&temp="+String(temp)+"&pression="+String(pression)+"&hum="+String(humidity)+"&v="+String(tension));
        
            // httpCode will be negative on error
            if(httpCode > 0) {
                if(httpCode == HTTP_CODE_OK) {

                    
                }
                Serial.println("HTTP OK");
                PowerDown();
            } else {
                Serial.println("HTTP NOK");
                PowerDown();
            }
            
            http.end();
            retryConnexionOk++;
        }else{
          PowerDown();
        }
        countConnexion++;
    }

    cmdComplete=false;
    while (Serial.available()>0 && cmdComplete ==false)
    {
       char car;
        car=Serial.read();
        if (car==0x02)
        {
          SerialCMD="";
        }else if (car==0x03)
        {
          cmdComplete=true;
        }else{
           SerialCMD +=car;
        }     
    }

    if (cmdComplete)
    {
      cmdComplete=false;
      if (SerialCMD=="reset")
      {
         ESP.restart();
      }else if (SerialCMD=="loadconfig")
      {  
        Serial.print(wificonfig);
      }
      else{
        StaticJsonBuffer<512> jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(SerialCMD);
        if (!json.success()) {
          Serial.println("Failed to parse config file");
        }else{
          Serial.println("OK");
          File configFile = SPIFFS.open("/config.json", "w");
          if (!configFile) {
            Serial.println("Failed to open config file for writing");
          }else{
            json.printTo(configFile);
            delay(2000);
            
          }
          
        }
      }
    }
}

void setupWifiAP()
{
  WiFi.mode(WIFI_AP);

  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String AP_NameString = "ESPIOT-" + macID;

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  String WIFIPASSSTR = "admin"+macID;
  char WIFIPASS[WIFIPASSSTR.length()+1];
  memset(WIFIPASS,0,WIFIPASSSTR.length()+1);
  for (int i=0; i<WIFIPASSSTR.length(); i++)
    WIFIPASS[i] = WIFIPASSSTR.charAt(i);

  WiFi.softAP(AP_NameChar,WIFIPASS );
}

void serverWebCfg()
{
  char* host = "espiot";
  Serial.println("HTTP server started");
    MDNS.begin(host);
    server.on("/", HTTP_GET, [](){
      server.sendHeader("Connection", "close");
       server.send(200, "text/html", strcat(serverIndex,Style));
    });
    server.on("/update", HTTP_GET, [](){
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", strcat(serverIndexUpdate,Style));
    });
    server.on("/params", HTTP_GET, [](){
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", strcat(serverIndexConfig,Style));
    });
     server.on("/config", HTTP_POST, [](){
      
       String StringConfig;
       String ssid=server.arg("ssid");
       String pass=server.arg("pass");
       String ip=server.arg("ip");

       String mask=server.arg("mask");

       String gw=server.arg("gw");

       String userhttp=server.arg("userhttp");
       String passhttp=server.arg("passhttp");
       String url=server.arg("url");
       uint8_t tmp[6];
       char tmpMAC[18];
       WiFi.macAddress(tmp);
       for (int i = 1; i < sizeof(tmp); ++i){
        sprintf(tmpMAC,"%s%02x:",tmpMAC,tmp[i]);
       }

       StringConfig = "{\"ID\":\""+String(tmpMAC)+"\",\"WIFISSID\":\""+ssid+"\",\"WIFIpass\":\""+pass+"\",\"Ip\":\""+ip+"\",\"Mask\":\""+mask+"\",\"GW\":\""+gw+"\",\"urlServer\":\""+url+"\",\"HTTPUser\":\""+userhttp+"\",\"HTTPPass\":\""+passhttp+"\"}";       
       
       StaticJsonBuffer<512> jsonBuffer;
       JsonObject& json = jsonBuffer.parseObject(StringConfig);
       File configFile = SPIFFS.open("/config.json", "w");
       if (!configFile) {
        Serial.println("Failed to open config file for writing");
       }else{
         json.printTo(configFile);
         //delay(6000);
         //ToReboot=true;
      }
      
      server.send(200, "text/plain", "OK");
    });
   
    server.on("/updateFile", HTTP_POST, [](){
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      ESP.restart();
    },[](){
      HTTPUpload& upload = server.upload();
      if(upload.status == UPLOAD_FILE_START){
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if(!Update.begin(maxSketchSpace)){//start with max available size
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_WRITE){
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_END){
        if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      yield();
    });
    server.begin();
    MDNS.addService("http", "tcp", 80);

}
