#include <DallasTemperature.h>
#include <OneWire.h>
#include "Temperature.h"

Temperature::Temperature(DallasTemperature *_dallasTmp) : currentTemp(-127), previousTemp(-127)
{
    _sensor = _dallasTmp;
    setWaitForResults();
};

/**
 * Get temperature in float format.
 * 
 * @return Temperature in float.
 */
float Temperature::getTemp()
{
    Temperature::tempRequested = false;
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
    tempRequested = false;
    return toString + suffix;
};

/**
 * Request temperature from sensor. Result available thought getter methods.
 *
 * @return true if temperature has changed since last request, otherwise returns false.
 */
void Temperature::requestOnBus()
{
    _sensor->requestTemperatures();
    currentTemp = _sensor->getTempCByIndex(0);
    tempRequested = true;
};

/**
 * Should program excecution stop and wait until data arrive.
 *
 * @param Optional true to block program while rading temperature, false to carry on with code excecution. (default = false)
 */
void Temperature::setWaitForResults(bool wait)
{
    _sensor->setWaitForConversion(wait);
};

/** 
 * Get if temperature is already been requested and availiable in memory
 * 
 * @param True if request is done and data available, otherwise false. 
 */
bool Temperature::getIsTempRequested()
{
    return tempRequested;
}

/** 
 * Get if temperature has changed since last request
 * 
 * @param True if temperature changed, otherwise false. 
 */
bool Temperature::hasTempChanged()
{
    if (currentTemp != previousTemp)
    {
        previousTemp = currentTemp;
        return true;
    }
    return false;
}