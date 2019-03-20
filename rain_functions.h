/// rain sensor, thanks to https://www.hackster.io/hliang/thingspeak-weather-station-data-analysis-2877b0
#include "config.h"
extern volatile unsigned int rainEventCount = 0;
extern unsigned int lastRainEvent = 0;
extern float rainScaleMM = 0.2794;

void handleRainEvent() {
    // Count rain gauge bucket tips as they occur
    // Activated by the magnet and reed switch in the rain gauge, attached to input D2
    unsigned int timeRainEvent = millis(); // grab current time

    // ignore switch-bounce glitches less than 10mS after initial edge
    if (timeRainEvent - lastRainEvent < 10) {
        return;
    }

    rainEventCount++; //Increase this minute's amount of rain
    lastRainEvent = timeRainEvent; // set up for next event
    Serial.println("Rain event....");
}

void initializeRainGauge() {
    pinMode(RainPin, INPUT_PULLUP);
    attachInterrupt(RainPin, handleRainEvent, FALLING);
    return;
}



float getRainMM()
{
    float result = rainScaleMM * float(rainEventCount);
    return result;
}
