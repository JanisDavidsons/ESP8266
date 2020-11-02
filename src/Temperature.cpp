#include <DallasTemperature.h>
#include <OneWire.h>
#include "Temperature.h"

Temperature::Temperature(DallasTemperature *_dallasTmp)
{
    _sensor = _dallasTmp;
};

int Temperature::getTempInt()
{
    _sensor->requestTemperatures();
    currentTemp = _sensor->getTempCByIndex(0);
    return currentTemp;
};

String Temperature::getTempString()
{
    _sensor->requestTemperatures();
    currentTemp = _sensor->getTempCByIndex(0);
    return String(currentTemp + "ºC");
};