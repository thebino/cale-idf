# CALE ESP-IDF beta

This is the beginning, and a very raw try, to make CALE compile in the Espressif IOT Development Framework. At the moment to explore how difficult it can be to pass an existing ESP32 Arduino framework project to a ESP-IDF based one and to measure how far we can go compiling this with Espressif's own dev framework. 
It will take some weeks to have a working example. The reason is that we would like to explore alternative libraries like [ESP32 IDF Epaper example](https://github.com/loboris/ESP32_ePaper_example) and to make a version that is compatible with the recently released ESP32-S2

## Branches

**master**...    -> stable version - v.0.9.1 First testeable version with a 2.13" b/w epaper display
**refactor/oop** -> Making the components base, most actual branch

tft_test         -> Original SPI master example from ESP-IDF 4 just refactored as a C++ class. WIll be kept for historic reasons


The aim is to learn good how to code and link classes as git submodules in order to program the epaper display driver the same way. The goal is to have a tiny "human readable" code in cale.cpp main file and that the rest is encapsulated in classes.

### Submodules

ESP-IDF uses relative locations as its submodules URLs (.gitmodules). So they link to GitHub. To update the submodules once you **git clone** this repository:

    git submodule update --init --recursive
    
to download the submodules (components) for this project.
Reminder for myself, in case you update the module library just execute:

    # pull all changes for the submodules
    git submodule update --remote

### Compile this 

If it's an ESP32

    idf.py -D IDF_TARGET=esp32 menuconfig

If you have ESP32S2BETA (The ones that Espressif sent before official release)

    idf.py -D IDF_TARGET=esp32s2beta menuconfig

If it's an ESP32S2

    idf.py -D IDF_TARGET=esp32s2 menuconfig

Make sure to edit "CALE configuration" in the Kconfig menuoptions.

And then just build and flash

    idf.py build
    idf.py flash

To clean and start again in case you change target

    idf.py fullclean

To open the serial monitor

    idf.py monitor

Please note that to exit the monitor in Espressif documentation says Ctrl+] but in Linux this key combination is:

    Ctrl+5


## References

https://github.com/krzychb/esp-epaper-29-ws (2 Years ago, probably ESP-IDF 3.0)

[esp32.com "epaper" search](https://esp32.com/search.php?keywords=epaper&fid%5B0%5D=13)

[GxEPD Epaper library](https://CALE.es) GxEPD is to use with Espressif Arduino Framework. 

[CALE.es Web-service](https://CALE.es) a Web-Service that prepares BMP & JPG Screens with the right size for your displays

[CALE.es Arduino-espressif32 firmware](https://github.com/martinberlin/eink-calendar)

### Credits 

GxEPD has been a great resource to start with. For CalEPD component, we mantain same Constants and use the same driver nomenclature as GxEPD, just in small case.  Hats off to Jean-Marc Zingg that was the first one to make such a great resource supporting so many Eink displays.