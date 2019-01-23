![alt text](https://raw.githubusercontent.com/wilyarti/weather_station/master/img_20190122_173911.jpg)
![alt text](https://raw.githubusercontent.com/wilyarti/weather_station/master/FireShot%20Capture%20002%20-%20Emerald%20Weather%20Station%20-%20ThingSpea_%20-%20https___thingspeak.com_channels_645847.png)

# Introduction
This weather station has evolved over time. I originally undersized the roof and solar panels and created about 4 different versions. I started with a 0.1W solar panel (110mmx60mm) but it was under powered and only reached charge in the later afternoon.

My latest version uses a 165mm square solar panel and allows for the device to measure more frequently (every minute vs 5 minutes), I think it would be sufficient for low lit areas.

The latest firmware will create a WiFi access point that you can connect if the device is unable to connect to the internet via WiFi. This allows you to enter your ThingSpeak credentials, the sampling interval in seconds, and the WiFi access point credentials you would like to connect to. To access this page connect to the the ESP access point (named ESP... something) and navigate to http://192.168.4.1 and click on the relevent buttons.

After configuring the ESP's WiFi access point will disappear if it was configured correctly.

# Building
First select the roof based on the solar panel you are using and print all the parts. The parts are have all the holes necessary so all you need to do is screw it together.

# Configuring
Create a ThingSpeak account and a channel. Enter this channel number and your credentials into the ESP8266 configuration page that appears when you connect to the access point (as discussed above).

You can the code for a cool 3D bar graph in my GitHub repository, simple copy this and replace my ThingSpeak channel number with your own.

# Issues
Solar radiation is a big problem and will cause the inside of the structure to heat up. You will need to paint any surface that comes in contact with the sun with many layers of exterior UV resistant paint (I used high gloss and about 6 coates).

The latest design uses a large gap below the roof (which holds the solar panel) to allow for air flow. The roof is designed so that water will accumulate and drip off areas away from the electronics - although I am yet to prove it is sufficiently water resistant.

If your WiFi goes down the ESP8266 will consume quite a lot of battery as it will not go into deep sleep but will keep awake running the WiFi access point. This can not be avoided as it is necessary to run the access point to configure the device initially or at a later date.

# OTA Update
To configure Over The Air updates simple add this to your metadata field on your ThingSpeak channel:
~~~~
{
      "publishInterval" : 30,
      "firmwareVersion" : "11",
      "firmwareURL" : "link to firmware url"
}
~~~~
Don't forget to increment your firmware version in the source code as well (to prevent continuous upgrading to the same firmware):
~~~~
const String VERSION = "11";

~~~~

To generate the firmware "compilation" under "Show verbose output during: " in the Arduino settings dialog. Then copy the firmware listed in the compilation status window after clicking compile:
~~~~
<snip>
/home/undef/.arduino15/packages/esp8266/tools/xtensa-lx106-elf-gcc/1.20.0-26-gb404fb9-2/bin/xtensa-lx106-elf-size -A /tmp/arduino_build_27952/thingspeak_ota_weatherstation.ino.elf
Sketch uses 324156 bytes (31%) of program storage space. Maximum is 1044464 bytes.
Global variables use 32228 bytes (39%) of dynamic memory, leaving 49692 bytes for local variables. Maximum is 81920 bytes.

~~~~

In this case the firmware is located here: /tmp/arduino_build_27952/thingspeak_ota_weatherstation.ino.elf
# Parts list
- 18650 battery holder
- Diode for the solar panel
- Wemos D1 Mini
- Wemos Battery Shield (or DC-DC Boost Converter Step Up Module)
- TP4056 Battery Charger (or MCP73871 solar board)
- 6v Solar panel (165mmx165mm recommended)
- BME280 sensor
- Cabling
- Hot Glue
- Silicon Sealant (optional)
- Exterior UV resistant paint

[View source code.](https://github.com/wilyarti/weather_station "Source code.")
[View live data.](https://thingspeak.com/channels/645847 "View live data.")
[View Thingiverse entry](https://www.thingiverse.com/thing:3280265)
