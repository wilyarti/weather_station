# weather_station
This is a simple IOT device that uses a BME280 pressure sensor and a Wemos D1 Mini + Battery Shield to send weather updates via WiFi to ThingSpeak.

I chose ThingSpeak because it was a very easy option, and has Matlab embedded into the website. It allows you to create funky graphs on demand using the inbuild Matlab environment.

The pin out is simple, with A0 -> 100K resistor to the battery, D0 to RST and the BME280 sensor to the data pins.

The housing is 3D printed using a AnyCubic Kossel Linear Plus in PETG. It will need to be screwed together using fasteners readily available at any hardware store.

The ThingSpeak channel is live and public here => https://thingspeak.com/channels/645847
