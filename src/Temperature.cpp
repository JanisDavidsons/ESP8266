#include <DallasTemperature.h>
#include <OneWire.h>
#include "Temperature.h"

Temperature::Temperature(DallasTemperature *_dallasTmp) : currentTemp(-127), previousTemp(-127)
{
    _sensor = _dallasTmp;
};

/**
 * Get temperature in float format.
 * 
 * @return Temperature in float.
 */
float Temperature::getTemp()
{
    return currentTemp;
};

/**
 * Get temperature in string format.
 * 
 * @param  Optional suffix (example "ºC").
 * @return Temperature in string type with optional suffix if provided.
 */
String Temperature::getTempString(String suffix)
{
    String toString = String(currentTemp, 2);
    return toString + suffix;
};

/**
 * Request temperature from sensor. Result available thought getter methods.
 *
 * @return true if temperature has changed since last request, otherwise returns false.
 */
bool Temperature::requestOnBus()
{
    _sensor->requestTemperatures();
    currentTemp = _sensor->getTempCByIndex(0);

    if (currentTemp != previousTemp)
    {
        previousTemp = currentTemp;
        return true;
    }
    return false;
};