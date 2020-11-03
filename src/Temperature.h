#ifndef Temperature_H
#define Temperature_H

#include <DallasTemperature.h>

class Temperature
{
private:
    DallasTemperature *_sensor;
    float currentTemp;
    float previousTemp;

public:
    Temperature(DallasTemperature *);
    bool requestOnBus();
    float getTemp();
    String getTempString(String suffix = "");
};

#endif