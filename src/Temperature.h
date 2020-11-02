#ifndef Temperature_H
#define Temperature_H

#include <DallasTemperature.h>

class Temperature
{
private:
    DallasTemperature* _sensor;

public:
    int currentTemp;
    int previousTemp;

    Temperature(DallasTemperature*);
    int getTempInt();
    String getTempString();
};

#endif