//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// IceCube LED model
// Made to use Arduino Due, 4800 NeoPixels, Adafruit 2.8" Capacative Touch Screen, and 3.3V -> 5V logic level converter
// Authors: John Evans, Elim Cheung
// Version: v1.1.0-20170612
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include <Adafruit_GFX.h>     // Core graphics library
#include <SPI.h>              // this is needed for display
#include <Adafruit_ILI9341.h> // Display library
#include <Wire.h>             // this is needed for FT6206
#include <Adafruit_FT6206.h>  // Touch library
#include <SD.h>               // SD card library
#include <FastLED.h>          // FastLED library
#include <StandardCplusplus.h>
#include <vector>

#include "LED.h"
#include "Event.h"
#include "timeBin.h"
#include "Directory.h"

#include "i3Model.h"

void setup () {
  Serial.begin(250000); // Set baud rate to 250000
  while (!Serial) ; // wait for Arduino Serial Monitor

  delay(500); // needed to properly boot on first tr

  i3Model.setup();
}

void loop () {
  i3Model.run();
}
