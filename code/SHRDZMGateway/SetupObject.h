class SetupObject
{
  public:
  class SetupItem
  {
    public:
    String m_deviceName;
    String m_parameterName;
    String m_parameterValue;

    public:
    SetupItem(String deviceName, String parameterName, String parameterValue)
    {
      m_deviceName = deviceName;
      m_parameterName = parameterName;
      m_parameterValue = parameterValue;
    };
    SetupItem(String deviceName, String command)
    {
      m_deviceName = deviceName;
      m_parameterName = command;
      m_parameterValue = "";
    };
  };

  public:
  SetupItem *items[20];
  
  void AddItem( String deviceName, String parameterName, String parameterValue )
  {
    for(int i = 0; i<20; i++)
    {
      if(items[i] == NULL)
      {
        items[i] = new SetupItem(deviceName, parameterName, parameterValue);
        break;
      }
    }
  };
  
  void AddItem( String deviceName, String command )
  {
    for(int i = 0; i<20; i++)
    {
      if(items[i] == NULL)
      {
        items[i] = new SetupItem(deviceName, command);
        break;
      }
    }
  };
  
  void AddInitItem( String deviceName )
  {
    for(int i = 0; i<20; i++)
    {
      if(items[i] == NULL)
      {
        items[i] = new SetupItem(deviceName, "init", "");
        break;
      }
    }
  };
  
  SetupItem* GetItem( String deviceName )
  {
    SetupItem *it = NULL;
    
    for(int i = 0;i<20; i++)
    {
      if(items[i] != NULL)
      {
        if(items[i]->m_deviceName == deviceName)
        {
          it = items[i];
        }
        break;
      }
    }

    return it;
  };  

  void RemoveItem(SetupItem *it)
  {
    for(int i = 0;i<20; i++)
    {
      if(items[i] != NULL && items[i] == it)
      {
        delete items[i];
        items[i] = NULL;
        break;
      }
    }
  };
};