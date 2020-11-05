#ifndef Temperature_H
#define Temperature_H

#include <DallasTemperature.h>

class Temperature
{
private:
    DallasTemperature *_sensor;
    float currentTemp = -127;
    float previousTemp = -127;
    bool tempRequested = false;
    

public:
    Temperature(DallasTemperature *);
    void requestOnBus();
    float getTemp();
    String getTempString(String suffix = "");
    void setWaitForResults(bool wait = false);
    bool getIsTempRequested();
    bool setIsTempRequested();
    bool hasTempChanged();
};

#endif