// TFT_eSPI configuration for the Wokwi simulator.
// Emulates an ILI9341 wired to the Wokwi ESP32-DevKitC.
// This file is force-included via -include sim/User_Setup.h in the [env:wokwi]
// build_flags, which shadows the real TFT_eSPI User_Setup.h at build time.
#ifndef USER_SETUP_SIM_WOKWI
#define USER_SETUP_SIM_WOKWI

#define USER_SETUP_INFO "Wokwi sim - ILI9341"

#define ILI9341_DRIVER

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000

#endif
