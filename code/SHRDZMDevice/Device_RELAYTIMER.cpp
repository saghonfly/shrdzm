#include "Device_RELAYTIMER.h"

Device_RELAYTIMER::Device_RELAYTIMER()
{  
  deviceTypeName = "RELAYTIMER";
  
  actionParameter = docAction.to<JsonObject>();  
  actionParameter["relay12"] = "TRIGGER";
  actionParameter["relay02"] = "TRIGGER";
  actionParameter["relay04"] = "TRIGGER";
  actionParameter["relay05"] = "TRIGGER";

  port = -1;
  state = false;

  dataAvailable = false;
  minWaitTime = millis()+500;
  processFinished = false;

  actionSet = false;
}

Device_RELAYTIMER::~Device_RELAYTIMER()
{
  Serial.println("RELAYTIMER Instance deleted");
}

bool Device_RELAYTIMER::setDeviceParameter(JsonObject obj)
{
  DeviceBase::setDeviceParameter(obj);
}

bool Device_RELAYTIMER::initialize()
{
  // create an object
  deviceParameter = doc.to<JsonObject>();

  return true;
}

bool Device_RELAYTIMER::setAction(String action)
{
  String sPort = getValue(action, ':', 0);
  if(sPort == "relay02")
    port = 2;
  else if(sPort == "relay04")
    port = 4;
  else if(sPort == "relay05")
    port = 5;
  else if(sPort == "relay12")
    port = 12;

  if(port != -1)
  {
    pinMode(port, OUTPUT);
    Serial.printf("Set port %d to LOW\n",port);
    
    setPort(HIGH);
    et = millis() + 1000 * 10;
  }

  actionSet = true;
  dataAvailable = true;

  return true;
}


void Device_RELAYTIMER::setPort(bool high)
{
  digitalWrite(port, high);
  state = high;      
}

bool Device_RELAYTIMER::setPostAction()
{
  if(port != -1)
  {
    setPort(LOW);
  } 

  dataAvailable = true;
  processFinished = true;

  return true;
}

bool Device_RELAYTIMER::loop()
{
  if(actionSet && millis() < et)
    return false;

//  Serial.printf("loop done\n");
//  setPort(HIGH);
    
  return true;
}

bool Device_RELAYTIMER::hasProcessEarlyEnded()
{
  return false;

  if(port == -1 && millis() > minWaitTime)
  {
    processFinished = true;   
    dataAvailable = true; 
    return true;
  }

}

SensorData* Device_RELAYTIMER::readParameterTypes()
{
  SensorData *al = new SensorData(5);

  al->di[0].nameI = "relay02";
  al->di[1].nameI = "relay04";
  al->di[2].nameI = "relay05";
  al->di[3].nameI = "relay12";
  al->di[4].nameI = "lastuptime";

  return al;
}

SensorData* Device_RELAYTIMER::readInitialSetupParameter()
{
  SensorData *al = new SensorData(3);

  al->di[0].nameI = "interval";
  al->di[0].valueI = "15";
  al->di[1].nameI = "preparetime";
  al->di[1].valueI = "0";
  al->di[2].nameI = "processtime";
  al->di[2].valueI = "1.5";

  return al;
}

SensorData* Device_RELAYTIMER::readParameter()
{  
  if(port == -1 && processFinished && dataAvailable)
  {
    SensorData *al = new SensorData(1);

    al->di[0].nameI = "lastuptime";
    al->di[0].valueI = String(millis());  

    dataAvailable = false;
    return al;
  }
  
  if(port == -1)
    return NULL;

  Serial.println("Port = "+String(port));
  
  SensorData *al = new SensorData(processFinished ? 2 : 1);

  String sstate = state ? "ON" : "OFF";

  if(port == 2)
    al->di[0].nameI = "relay02";
  else if(port == 4)
    al->di[0].nameI = "relay04";
  else if(port == 5)
    al->di[0].nameI = "relay05";
  else if(port == 12)
    al->di[0].nameI = "relay12";

  al->di[0].valueI = sstate;  

  if(processFinished)
  {
    al->di[1].nameI = "lastuptime";
    al->di[1].valueI = String(millis());  
  }

  dataAvailable = false;

  return al;
}

String Device_RELAYTIMER::getValue(String data, char separator, int index)
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
