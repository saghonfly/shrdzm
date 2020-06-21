#ifndef DEVICEBASE_H
#define DEVICEBASE_H

#include <Arduino.h>
#include <ArduinoJson.h>

class ConfigData
{
  public:
    ConfigData(int size);
    ~ConfigData();

    class ConfigDataItem 
    { 
      public:
        String valueI;
        ConfigDataItem(){};
        ConfigDataItem(String value);
    };  
  
    ConfigDataItem *di;
    int size;
};

class SensorData
{
  public:
    SensorData(int size);
    ~SensorData();

    class DataItem 
    { 
      public:
        String nameI;    
        String valueI;
        DataItem(){};
        DataItem(String name, String value);
    };  
  
    DataItem *di;
    int size;
};

class DeviceBase
{
  public:
    DeviceBase(){};

    virtual bool setDeviceParameter(JsonObject obj);
    virtual bool initialize();
    JsonObject getDeviceParameter();
    virtual SensorData* readParameterTypes();
    virtual SensorData* readParameter();
    virtual SensorData* readInitialSetupParameter(){return NULL;};

  protected:
    void PrintText(String text);

    JsonObject deviceParameter;
    StaticJsonDocument<JSON_OBJECT_SIZE(10)> doc;

};

#endif