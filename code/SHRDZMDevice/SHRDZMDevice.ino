/*
  SHRDZMDevice

  Created 20 Jul 2020
  By Erich O. Pintar
  Modified 15 December 2020
  By Erich O. Pintar

  https://github.com/saghonfly

*/

#define DEBUG_SHRDZM

#include "config/config.h"

Configuration configuration;
SimpleEspNowConnection simpleEspConnection(SimpleEspNowRole::CLIENT);
DeviceBase* dev;
String ver, nam;
bool firmwareUpdate = false;
bool avoidSleeping = false;
bool batterycheckDone = false;
bool canGoDown = false;
bool forceSleep = false;
bool loopDone = false;
bool initReboot = false;
bool sendBufferFilled = false;
bool isDeviceInitialized = false;
bool pairingOngoing = false;
bool finishSent = false;
bool processendSet = false;
bool processendReached = false;
bool configurationMode = false;
bool gatewayMode = false;
String SSID;
String password;
String host;
String url;
unsigned long clockmillis = 0;
unsigned long prepareend = 0;
unsigned long processend = 0;
bool finalMeasurementDone = false;
bool setNewDeviceType = false;
String newDeviceType = "";
String deviceName;
bool writeConfiguration = false;
Ticker configurationBlinker;
ESP8266WebServer *server;

/// Configuration Webserver
char* getWebsite(char* content)
{  
  int len = strlen(content);
  
  char *temp = (char *) malloc (1400+len);

#ifdef DEBUG
  Serial.println("Handle Root");
  Serial.println("Content len = "+String(len));
#endif

  snprintf(temp, 1400+len,  
"<!DOCTYPE html>\
<html>\
<head>\
<style>\
body {\
  font-family: Arial, Helvetica, sans-serif;\
}\
\
hr\
{ \
  display: block;\
  margin-top: 0.5em;\
  margin-bottom: 0.5em;\
  margin-left: auto;\
  margin-right: auto;\
  border-style: inset;\
  border-width: 1px;\
}\
ul \
{\
list-style-type: none;\
  margin: 0;\
  padding: 0;\
  width: 150px;\
  background-color: #f1f1f1;\
  position: fixed;\
  height: 100%;\
  overflow: auto;\
}\
\
li a {\
  display: block;\
  color: #000;\
  padding: 8px 16px;\
  text-decoration: none;\
}\
\
li a.active {\
  background-color: #4CAF50;\
  color: white;\
}\
\
li a:hover:not(.active) {\
  background-color: #555;\
  color: white;\
}\
.main {\
  margin-left: 200px;\
  margin-bottom: 30px;\
}\
</style>\
<title>SHRDZMGateway - %s</title>\
</head>\
<body>\
\
<ul>\
  <li>\
    <a class='active' href='#home'>SHRDZMDevice<br/>\
      <font size='2'>%s</font>\
    </a></li>\
  <li><a href='./general'>General</a></li>\
  <li><a href='./settings'>Settings</a></li>\
  <li><a href='./about'>About</a></li>\
  <li><a href='./reboot'>Reboot</a></li>\
  <br/>\
  <li><a href='./deleteconfig'>Delete Config</a></li>\  
  <br/><br/><br/>\
  <li><center>&copy;&nbsp;<font size='2' color='darkgray'>Erich O. Pintar</font></center></li>\  
  <br/><br/>\
</ul>\
\
<div class='main'>\
  %s\
</div>\
</body>\
</html>\
  ", deviceName.c_str(), deviceName.c_str(), content);

  return temp;
}

void handleRoot() 
{
  char * temp = getWebsite("<h1>General</h1>General Information");

  server->send(200, "text/html", temp);

  free(temp); 
}

void handleNotFound() 
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server->uri();
  message += "\nMethod: ";
  message += (server->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server->args();
  message += "\n";

  for (uint8_t i = 0; i < server->args(); i++) {
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }

  server->send(404, "text/plain", message);
}

void handleReboot() 
{
  char temp[300];
  
  snprintf(temp, 300,
  "<!DOCTYPE html>\
  <html>\
  <head>\
  <meta http-equiv='refresh' content='20; url=/'>\
  </head>\
  <body>\
  <h1>Please wait. Will reboot in 20 seconds...</h1>\
  </body>\
  </html>\
  ");

  server->send(200, "text/html", temp);

  delay(2000);
  
  ESP.reset();  
}

void handleSettings()
{
  char content[2600];

  if(server->args() != 0)
  {
    if( server->arg("wlanenabled") == "1")
      configuration.setWlanParameter("enabled", "true");
    else
      configuration.setWlanParameter("enabled", "false");

    if(server->hasArg("ssid"))
      configuration.setWlanParameter("ssid", server->arg("ssid").c_str());
    else
      configuration.setWlanParameter("ssid", "");
        
    if(server->hasArg("password"))
      configuration.setWlanParameter("password", server->arg("password").c_str());
    else
      configuration.setWlanParameter("password", "");    
    if(server->hasArg("MQTTbroker"))
      configuration.setWlanParameter("MQTTbroker", server->arg("MQTTbroker").c_str());
    if(server->hasArg("MQTTport"))
      configuration.setWlanParameter("MQTTport", server->arg("MQTTport").c_str());
    if(server->hasArg("MQTTuser"))
      configuration.setWlanParameter("MQTTuser", server->arg("MQTTuser").c_str());
    if(server->hasArg("MQTTpassword"))
      configuration.setWlanParameter("MQTTpassword", server->arg("MQTTpassword").c_str());

    writeConfiguration = true;    
  }
  
  snprintf(content, 2600,  
      "<h1>Settings</h1><p><strong>Configuration</strong><br /><br />\
      <p>WLAN Settings if Device acts as it's own gateway.</p>\
      </p>\
      <br/><br/>\
      <form method='post'>\
      <input type='checkbox' id='wlanenabled' name='wlanenabled' value='1' %s/>\
      <input type='hidden' name='wlanenabled' value='0' />\
      <label for='wlanenabled'>Device should act as it's own gateway</label><br/>\
      <br/>\
      <hr/>\
      <input type='text' id='ssid' name='ssid' placeholder='SSID' size='50' value='%s'>\
      <label for='ssid'>SSID</label><br/>\
      <br/>\
      <input type='password' id='password' name='password' placeholder='Password' size='50' value='%s'>\
      <label for='password'>Password</label><br/>\
      <input type='checkbox' onclick='showWLANPassword()'>Show Password\
      <br/>\
      <input type='text' id='MQTTbroker' name='MQTTbroker' placeholder='MQTT Broker' size='50' value='%s'>\
      <label for='MQTTbroker'>MQTT Broker</label><br/>\
      <br/>\
      <input type='text' id='MQTTport' name='MQTTport' placeholder='MQTT Port' size='50' value='%s'>\
      <label for='MQTTbroker'>MQTT Port</label><br/>\
      <br/>\
      <input type='text' id='MQTTuser' name='MQTTuser' placeholder='MQTT User' size='50' value='%s'>\
      <label for='MQTTuser'>MQTT User</label><br/>\
      <br/>\
      <input type='text' id='MQTTpassword' name='MQTTpassword' placeholder='MQTT Password' size='50' value='%s'>\
      <label for='MQTTuser'>MQTT Password</label><br/>\
      <br/><br /> <input type='submit' value='Save Configuration!' />\
      <script>\
      function showWLANPassword() {\
        var x = document.getElementById('password');\
        if (x.type === 'password') {\
          x.type = 'text';\
        } else {\
          x.type = 'password';\
        }\
      }\
      </script>\ 
      </form>\
      "
      ,
      String(configuration.getWlanParameter("enabled")) == "true" ? "checked" : "",
      configuration.getWlanParameter("ssid"),
      configuration.getWlanParameter("password"),
      configuration.getWlanParameter("MQTTbroker"),
      configuration.getWlanParameter("MQTTport"),
      configuration.getWlanParameter("MQTTuser"),
      configuration.getWlanParameter("MQTTpassword") 
  );  

  Serial.println("configuration.getWlanParameter(\"enabled\") = "+String(configuration.getWlanParameter("enabled")));

  char * temp = getWebsite(content);

  server->send(200, "text/html", temp);

  free(temp); 
}

///////////////////////////


/// Gateway Webserver
void startGatewayWebserver()
{
  WiFi.mode(WIFI_STA);
  DLN("after WIFI_STA ");

  uint8_t pmac[6];
  WiFi.macAddress(pmac);
  deviceName = macToStr(pmac);

  deviceName.replace(":", "");
  deviceName.toUpperCase();

  String APName = "SHRDZMDevice-"+deviceName;
  WiFi.hostname(APName.c_str());
  WiFi.begin(configuration.getWlanParameter("ssid"), configuration.getWlanParameter("password"));
  DLN("after Wifi.begin");


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }  
}


///////////////////////////
String macToStr(const uint8_t* mac)
{
  char mac_addr[13];
  mac_addr[12] = 0;
  
  sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

  return String(mac_addr);
}

void changeConfigurationBlinker()
{
#ifdef LEDPIN
  digitalWrite(LEDPIN, !(digitalRead(LEDPIN)));
#endif
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++)
  {
    if(data.charAt(i)==separator || i==maxIndex)
    {
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void sendSetup()
{
  if(!configuration.containsKey("gateway"))
    return;
  
  simpleEspConnection.sendMessage((char *)"$I$");
  
  configuration.sendSetup(&simpleEspConnection);

  if(dev != NULL)
  {
    SensorData* sd = dev->readParameterTypes();

    if(sd != NULL)
    {
      String reply = "";
      
      reply = "$SD$";

      for(int i = 0; i<sd->size; i++)
      {
        reply += sd->di[i].nameI;
        if(i < sd->size-1)
          reply += "|";
      }

      simpleEspConnection.sendMessage((char *)reply.c_str());

      delete sd; 

      JsonObject ap = dev->getActionParameter();
      if(!ap.isNull())
      {
        reply = "$AP$";
    
        for (JsonPair kv : ap) 
        {
          reply += kv.key().c_str()+String(":")+kv.value().as<char*>()+"|";
        }
    
        reply.remove(reply.length()-1);
    
        simpleEspConnection.sendMessage((char *)reply.c_str());    
      }      
    }        
  }

  String s = String("$V$")+ver+"-"+ESP.getSketchMD5();
  simpleEspConnection.sendMessage((char *)s.c_str());

  // send supported devices
  s = String("$X$")+String(SUPPORTED_DEVICES);
  simpleEspConnection.sendMessage((char *)s.c_str());  
}

bool updateFirmware(String message)
{
  if(message.indexOf('|') == -1)
  {
    DLN("firmware update not possible ! "+message);
    return false;
  }

  SSID = getValue(message, '|', 0);
  password = getValue(message, '|', 1);
  host = getValue(message, '|', 2);

  DLN("SSID:"+SSID+" password:"+password+" host:"+host);

  if(host.substring(0,7) != "http://")
  {
    DLN("Upgrade : only http address supported!");
    return false;    
  }

  host = host.substring(7);

  if(host.substring(host.length()-4) != ".php")
  {
    DLN("Upgrade : only php update script supported");
    return false;    
  }

  if(host.indexOf('/') == -1)
  {
    DLN("Upgrade : host string not valid");
    return false;    
  }

  url = host.substring(host.indexOf('/'));
  host = host.substring(0,host.indexOf('/'));

  DLN("before simpleEspConnection.end");
  DLN("after simpleEspConnection.end");
  WiFi.disconnect(true);
  
  WiFi.mode(WIFI_STA);
  DLN("after WIFI_STA ");

  WiFi.begin(SSID.c_str(), password.c_str());
  DLN("after Wifi.begin");

  return true;
}

void setConfig(String cmd)
{
  if(cmd == "configuration")
  {
    DLN("need to send the setup...");
    sendSetup();     

    return;
  }
  
  if(cmd.indexOf(':') == -1)
    return;

  String pname = getValue(cmd, ':', 0);
  String pvalue = getValue(cmd, ':', 1);

  DLN("setConfig "+pvalue);

  if( pname == "interval" || 
      pname == "sensorpowerpin" || 
      pname == "devicetype" || 
      pname == "preparetime" || 
      pname == "processtime" || 
      pname == "batterycheck" || 
      pname == "gateway")
  {
    if(pname == "devicetype")
    {
      newDeviceType = pvalue;
      setNewDeviceType = true;
    }
    else
    {
      configuration.set((char *)pname.c_str(), (char *)pvalue.c_str());
    }
  }
  else
  {
    if(configuration.containsDeviceKey((char *)pname.c_str()))
    {
      configuration.setDeviceParameter((char *)pname.c_str(), (char *)pvalue.c_str());
    }
  }
       
  configuration.store();    
  sendSetup();       
}

void OnMessage(uint8_t* ad, const uint8_t* message, size_t len)
{
  DLN("MESSAGE:"+String((char *)message));

  if(String((char *)message) == "$SLEEP$") // force to go sleep
  {
    canGoDown = true;
    DLN("FORCE SLEEP MODE");
    forceSleep= true;
    return;
  }
  
  if(String((char *)message) == "$S$") // ask for settings
  {
    sendSetup();
  }
  else if(String((char *)message) == "$PING$") // ping
  {
    if(configuration.containsKey("gateway"))
    {
      simpleEspConnection.sendMessage("$PING$");
    }
      
    return;
  }
  else if(String((char *)message).substring(0,5) == "$SDT$") // set device type
  {
    newDeviceType = String((char *)message).substring(5);
    setNewDeviceType = true;
    
    configuration.store();        
    sendSetup();
  }
  else if(String((char *)message).substring(0,4) == "$SC$") // set configuration
  {
    if(dev != NULL)
    {
      JsonObject ap = dev->getActionParameter();
      if(!ap.isNull())
      {
        String pname = getValue(String((char *)message).substring(4), ':', 0);      
        if(ap.containsKey(pname))
        {
          dev->setAction(String((char *)message).substring(4));
        }
        else
        {
          setConfig(String((char *)message).substring(4));
        }
      }
      else
      {
        setConfig(String((char *)message).substring(4));
      }      
    }       
    else
    {
      setConfig(String((char *)message).substring(4));
    }
  }  
  else if(String((char *)message).substring(0,3) == "$U$") // update firmware
  {
    firmwareUpdate = true;   

    if(!updateFirmware(String((char *)message).substring(3)))
    {
      delay(100);    
      ESP.restart();            
    }
  }
}

void OnPairingFinished()
{
  clockmillis = millis();  

  DLN("OnPairingFinished");

  sendSetup();
  avoidSleeping = false;
  pairingOngoing = false;

  clockmillis = millis();  

  initReboot = true;
}

void OnNewGatewayAddress(uint8_t *ga, String ad)
{
  simpleEspConnection.setServerMac(ga);
  configuration.set("gateway", (char *)ad.c_str());  

  configuration.store();   
}

void OnSendError(uint8_t* ad)
{
  
}

void OnSendDone(uint8_t* ad)
{
 // avoidSleeping = false;
}

// for firmware upgrade
void update_started() 
{
  DLN("CALLBACK:  HTTP update process started");
}

void update_finished() 
{
  DLN("CALLBACK:  HTTP update process finished ");
}

void update_progress(int cur, int total) 
{
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes..\n", cur, total);
}

void update_error(int err) 
{
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}

void upgradeFirmware()
{
  avoidSleeping = true;
  
  if ((WiFi.status() == WL_CONNECTED)) 
  {     
    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);
  
    String versionStr = nam+" "+ver+" "+ESP.getSketchMD5();
    DLN("WLAN connected!");
  
    WiFiClient client; 
    Serial.printf("host:%s, url:%s, versionString:%s \n", host.c_str(), url.c_str(), versionStr.c_str());
    t_httpUpdate_return ret = ESPhttpUpdate.update(host, 80, url, versionStr);    
    
    switch (ret) 
    {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d):  sendUpdatedVersion %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        delay(100);
        ESP.restart();
        break;
  
      case HTTP_UPDATE_NO_UPDATES:
        DLN("HTTP_UPDATE_NO_UPDATES ");
        delay(100);
        ESP.restart();
        break;
  
      case HTTP_UPDATE_OK:
        DLN("HTTP_UPDATE_OK");
        break;
    }
  }  
}

void initDeviceType(const char *deviceType, bool firstInit)
{
  delete dev;

  if(strcmp(deviceType, "DHT22") == 0)
  {
    dev = new Device_DHT22();
  }
  else if(strcmp(deviceType, "BH1750") == 0)
  {
    dev = new Device_BH1750();
  }
  else if(strcmp(deviceType, "BMP280") == 0)
  {
    dev = new Device_BMP280();
  }
  else if(strcmp(deviceType, "BME280") == 0)
  {
    dev = new Device_BME280();
  }
  else if(strcmp(deviceType, "DS18B20") == 0)
  {
    dev = new Device_DS18B20();
  }
  else if(strcmp(deviceType, "HTU21D") == 0 || strcmp(deviceType, "HTU21") == 0 ||
          strcmp(deviceType, "SI7021") == 0 || strcmp(deviceType, "SHT21") == 0)
  {
    dev = new Device_HTU21D();
  }
  else if(strcmp(deviceType, "MQ135") == 0)
  {
    dev = new Device_MQ135();
  }
  else if(strcmp(deviceType, "WATER") == 0)
  {
    dev = new Device_WATER();
  }
  else if(strcmp(deviceType, "ANALOG") == 0)
  {
    dev = new Device_ANALOG();
  }
  else if(strcmp(deviceType, "DIGITAL") == 0)
  {
    dev = new Device_DIGITAL();
  }
  else if(strcmp(deviceType, "SDS011") == 0)
  {
    dev = new Device_SDS011();
  }
  else if(strcmp(deviceType, "IM350") == 0)
  {
    dev = new Device_IM350();
  }
  else if(strcmp(deviceType, "SDS011_BMP280") == 0)
  {
    dev = new Device_SDS011_BMP280();
  }  
  else if(strcmp(deviceType, "SDS011_BME280") == 0)
  {
    dev = new Device_SDS011_BME280();
  }  
  else if(strcmp(deviceType, "DIGITALGROUND") == 0)
  {
    dev = new Device_DIGITALGROUND();
  }
  else if(strcmp(deviceType, "RELAYTIMER") == 0)
  {
    dev = new Device_RELAYTIMER();
  }
  else if(strcmp(deviceType, "GW60") == 0)
  {
    dev = new Device_GW60();
  }
  else
  {
    return;
  }
  
  if(dev != NULL)
  {
    configuration.set("devicetype", (char *)deviceType);
    
    if(firstInit)
    {     
      dev->initialize();
      
      JsonObject dc = dev->getDeviceParameter();

      configuration.setDeviceParameter(dc);

      SensorData *initParam = dev->readInitialSetupParameter();
  
      if(initParam)
      {
        for(int i = 0; i<initParam->size; i++)
        {
          if(configuration.containsKey((char *)initParam->di[i].nameI.c_str()))
          {
            configuration.set((char *)initParam->di[i].nameI.c_str(), (char *)initParam->di[i].valueI.c_str());
          }
        }
        
        delete initParam;
      }      

      initReboot = true;
    }

    dev->setDeviceParameter(configuration.getDeviceParameter());    
  }
}

void setup() 
{
#ifdef DEBUG_SHRDZM
Serial.begin(9600); Serial.println();
#endif
  bool writeConfigAndReboot = false;
  
#ifdef VERSION
  ver = String(VERSION);
#else
  ver = "0.0.0";  
#endif

#ifdef NAME
  nam = String(NAME);
#else
  nam = "SHRDZMDevice";  
#endif

  DV(nam);

  if(!SPIFFS.begin())
  {
    DLN("First use. I need to format file system. This will take a few seconds. Please wait...");
    SPIFFS.format();
  }
  else
  {
    DLN("SPIFFS accessed...");
  }

  DLN("configuration loading...");    
  if(!configuration.load())
  {
    DLN("configuration.initialize...");    
    configuration.initialize();
  }

  DLN("Configuration loaded...");    

  if(configuration.migrateToNewConfigurationStyle())
  {
    DLN("Migration to new config style needed. Will reboot now...");    

    configuration.store();    

    delay(100);    
    ESP.restart();      
  }
  
  if(!configuration.containsKey("preparetime"))
  {
    configuration.set("preparetime", "0");
    writeConfigAndReboot = true;
  }

  if(!configuration.containsKey("processtime"))
  {
    configuration.set("processtime", "0");
    writeConfigAndReboot = true;
  }

  if(!configuration.containsKey("batterycheck"))
  {
    configuration.set("batterycheck", "OFF");
    writeConfigAndReboot = true;
  }  


  if(writeConfigAndReboot)
  {
    configuration.store();    

    delay(100);    
    ESP.restart();      
  }

  String lastVersionNumber = configuration.readLastVersionNumber();
  String currVersion = ESP.getSketchMD5();

  DLN("'"+lastVersionNumber+"':'"+currVersion+"'");

  if(configuration.get("pairingpin") != NULL)
  {    
    pinMode(atoi(configuration.get("pairingpin")), INPUT_PULLUP);    
    DLN("PairingPin = "+String(configuration.get("pairingpin")));    
  }
  else
  {
    int s = 13;
#ifdef PAIRING_PIN    
    s = PAIRING_PIN;
#endif
    configuration.set("pairingpin", (char *)(String(s).c_str()));
    configuration.store();    

    delay(100);    
    ESP.restart();      
  }

  /// check whether to start in gateway mode
  if( String(configuration.getWlanParameter("enabled")) == "true" )
  {
    gatewayMode = true;
  }

  // check if pairing button pressed
  pairingOngoing = !digitalRead(atoi(configuration.get("pairingpin")));
  
  if(!gatewayMode || pairingOngoing)
  {
    simpleEspConnection.begin();
  
    DV(simpleEspConnection.myAddress);
    
    simpleEspConnection.onPairingFinished(&OnPairingFinished);  
    simpleEspConnection.setPairingBlinkPort(LEDPIN);  
    simpleEspConnection.onSendError(&OnSendError);    
    simpleEspConnection.onSendDone(&OnSendDone);
    if(configuration.containsKey("gateway"))
    {
      simpleEspConnection.setServerMac(configuration.get("gateway"));  
    }
  
    simpleEspConnection.onNewGatewayAddress(&OnNewGatewayAddress);    
    simpleEspConnection.onMessage(&OnMessage);  
  }
  
  if(pairingOngoing)
  {
    avoidSleeping = true;
    DLN("Start pairing");    
//    pairingOngoing = true;
    simpleEspConnection.startPairing(300);
  }
  else
  {
    // enable sensor power if configured
    if(atoi(configuration.get("sensorpowerpin")) != 99)
    {
      pinMode(atoi(configuration.get("sensorpowerpin")), OUTPUT);
      digitalWrite(atoi(configuration.get("sensorpowerpin")),HIGH);          
    }
    
    // check if preparation is needed
    prepareend = 1000 * atoi(configuration.get("preparetime"));
    DV(prepareend);

    if(strcmp(lastVersionNumber.c_str(), currVersion.c_str()) != 0)
    {    
      sendSetup();
      configuration.storeVersionNumber();
    }      
  }

  if(gatewayMode)
  {
    startGatewayWebserver();
  }

  clockmillis = millis();  
}

void getMeasurementData()
{
  if(configuration.containsKey("gateway"))
  {      
    if(dev != NULL)
    {
      SensorData* sd = dev->readParameter();
  
      if(sd != NULL)
      {
        String reply;
        
        for(int i = 0; i<sd->size; i++)
        {
          reply = "$D$";
    
          reply += sd->di[i].nameI+":"+sd->di[i].valueI;

          DV(reply);
    
          simpleEspConnection.sendMessage((char *)reply.c_str());  
        }
        delete sd;
        sd = NULL;
      }
    }
  }  
}

void loop() 
{
  if(configurationMode)
  {
    server->handleClient();

    if(writeConfiguration)
    {
      writeConfiguration = false;
      configuration.store();
    }

    return;  
  }

  if(gatewayMode) // start web server
  {

    return;
  }
  
  if(!firmwareUpdate && configuration.containsKey("gateway"))
    sendBufferFilled = simpleEspConnection.loop();

  if(pairingOngoing)
  {
    if(millis() > 2000)
    {
      if(digitalRead(atoi(configuration.get("pairingpin"))) == false)
      {
        DLN("Entering configuration mode...");
        simpleEspConnection.endPairing();

        pairingOngoing = false;
        configurationMode = true;
        simpleEspConnection.end();

        uint8_t pmac[6];
        WiFi.macAddress(pmac);
        deviceName = macToStr(pmac);
      
        deviceName.replace(":", "");
        deviceName.toUpperCase();

        String APName = "SHRDZM-"+deviceName;
        WiFi.hostname(APName.c_str());        
        WiFi.softAP(APName);     
        
        server = new ESP8266WebServer(80);

        server->on("/", handleRoot);
        server->on("/reboot", handleReboot);
        server->on("/general", handleRoot);
        server->on("/settings", handleSettings);
        server->onNotFound(handleNotFound);
        server->begin();
        

        configurationBlinker.attach(0.2, changeConfigurationBlinker);
      }
    }
    return;
  }

  if(firmwareUpdate)
    upgradeFirmware();

  if(!batterycheckDone && configuration.containsKey("gateway"))
  {
    batterycheckDone = String(configuration.get("batterycheck")) == "ON" ? false : true;
    if(!batterycheckDone)
    {      
      String reply = "$D$battery:"+String(analogRead(A0));

      DLN("battery : "+reply);

      simpleEspConnection.sendMessage((char *)reply.c_str());  
      batterycheckDone = true;
    }
  }

  if(!isDeviceInitialized)
  {
    if(configuration.get("devicetype") != "UNKNOWN")
    {
      initDeviceType(configuration.get("devicetype"), false);
      if(dev != NULL)
        dev->prepare();
    }
    
    isDeviceInitialized = true;
  }
  
  // get measurement data
  if(dev != NULL)
  {
    loopDone = dev->loop();
    if(dev->isNewDataAvailable())
    {
      getMeasurementData();
    } 
  }  
  else
    loopDone = true;

  if(millis() >= prepareend && !processendSet)
  {
    if(atof(configuration.get("processtime")) > 0.0f)
    {
      processend = 1000 * atof(configuration.get("processtime")) + millis();      
    }
    else
    {
      processend = 0;
    }    
    
    processendSet = true;
  }

  if(processendSet && !processendReached)
  {
    if(dev != NULL)
    {
      processendReached = dev->hasProcessEarlyEnded();
      DLN("Process early finshed.");
    }
  }

  if(!finalMeasurementDone && millis() >= prepareend)
  {
    getMeasurementData();

    finalMeasurementDone = true;
    DV(finalMeasurementDone);
    clockmillis = millis();
  }

  if(((loopDone && !sendBufferFilled && finalMeasurementDone) || processendReached) && !finishSent)
  {
    // send finish to gateway
    if(configuration.containsKey("gateway"))
    {
      simpleEspConnection.sendMessage("$F$");  
      DLN("Finished process sent");
      finishSent = true;
    }    
  }

  if(millis() > MAXCONTROLWAIT+clockmillis && !sendBufferFilled && loopDone & finalMeasurementDone && millis() >= prepareend)
  {
      canGoDown = true;
  }  

  if(!processendReached && millis() > processend)
  {
    if(dev != NULL)
    {
      dev->setPostAction();
  
      if(dev->isNewDataAvailable())
      {    
        getMeasurementData();
      }
    }
    
    processendReached = true;
  }

  if(setNewDeviceType && finalMeasurementDone)
  {
    initDeviceType(newDeviceType.c_str(), true);
    setNewDeviceType = false;
    newDeviceType = "";

    configuration.store();        
    DLN("vor sendSetup");
    sendSetup();    
    DLN("nach sendSetup");
  }


  if(canGoDown && !avoidSleeping && simpleEspConnection.isSendBufferEmpty() && finalMeasurementDone && processendReached)
  {
    if(initReboot)
    {
      ESP.restart();
      delay(100);

      return;
    }
    if(atoi(configuration.get("interval")) > 0)
    {
      // send uptime
      gotoSleep();    
    }
  }
}

void gotoSleep() 
{  
  delete dev;  
  int sleepSecs;

  if(configuration.get("devicetype") == "UNKNOWN") // goto sleep just for 5 seconds and flash 2 times
  {
    sleepSecs = 5;
#ifdef LEDPIN
    pinMode(LEDPIN, OUTPUT);

    digitalWrite(LEDPIN, LOW);
    delay(500);
    digitalWrite(LEDPIN, HIGH);
    delay(500);
    digitalWrite(LEDPIN, LOW);
    delay(500);
    digitalWrite(LEDPIN, HIGH);
#endif
  }
  else
  {
    sleepSecs = atoi(configuration.get("interval")) - atoi(configuration.get("preparetime"));
  }

  Serial.printf("Up for %i ms, going to sleep for %i secs... \n", millis(), sleepSecs); 

  if(sleepSecs > 0)
  {
    ESP.deepSleep(sleepSecs * 1000000, RF_NO_CAL);
  }
  
  delay(100);
}
