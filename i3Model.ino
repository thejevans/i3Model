#include <Adafruit_GFX.h>     // Core graphics library
#include <SPI.h>              // this is needed for display
#include <Adafruit_ILI9341.h> // Display library
#include <Wire.h>             // this is needed for FT6206
#include <Adafruit_FT6206.h>  // Touch library
#include <SD.h>               // SD card library
#include <Adafruit_NeoPixel.h>// NeoPixel library

#ifdef __AVR__
#include <avr/power.h>
#endif

// SPI pins
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_FT6206 ts = Adafruit_FT6206();

// Set button sizes and locations
#define BOXSIZE 40
#define BOXSTART_X 35
#define BOXSTART_X2 125
#define BOXSTART_Y 220
#define BOXSTART_Y2 270

// Set maximum brightness
#define MAX_BRIGHT 100

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 12

// We have 80 strings x 60 doms/string = 4800 total doms NeoPixels are attached to the Arduino.
#define NUMPIXELS 4800

// Set pin for SD card
const int chipSelect = 4;

// Global variables for file management
int page = 1;
File root;
String fileNames[255];
int lastFile = 0;
int lastPage = 0;
int isEventFile[255];
int isDir[255];

// Switch for pausing events or tests
bool paused = false;

// Switch for stopping events or tests
bool stopped = false;

// True if event or test is playing or paused. False if no event or test running
bool playing = false;

// False if continually pressing touchscreen
bool released = true;

//0 = none; 1 = h_menu; 2 = file_menu
int activeMenu = 0;

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // wait for Arduino Serial Monitor

  delay(500); // needed to properly boot on first try

  tft.begin();
  ts.begin();
  pixels.begin();

  // Clear NeoPixels
  clearPixels();

  // Clear TFT
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  // End program if no SD card detected
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  // Save filenames and directory flags to fileNames for printing in the file menu
  root = SD.open("/");
  File entry = root.openNextFile();
  int i = 0;
  String k = "";
  while(entry) {
    fileNames[i] = "";
    isDir[i] = 0;
    k = entry.name();
    isEventFile[i] = 0;
    if(!k.startsWith("_")) {
      if (!entry.isDirectory()) {
        if (k.substring(k.indexOf('.')) == ".I3R") {
          isEventFile[i] = 1;
          Serial.println(k);
        }
        //k = k + "\t\t\t\t\t\t\t";
        //k = k + entry.size();
      }
      else {
        isDir[i] = 1;
      }
      fileNames[i] = k;
      i++;
      lastFile++;
    }
    entry = root.openNextFile();
  }
  lastPage = (lastFile - (lastFile % 25)) / 25;
  if(lastFile % 25 > 0) {
    lastPage++;
  }
  root.rewindDirectory();

  // Display initial home menu
  make_h_menu(0);
}

// Handles touch events
TS_Point boop() {
  TS_Point p;
  if (ts.touched()) {
    if (released) {
      Serial.println("boop!");
      p = ts.getPoint();
      p.x = -(p.x - 240);
      p.y = -(p.y - 320);
      Serial.println(p.x);
      Serial.println(p.y);
      released = false;
      return p;
    }
  }
  else {
    released = true;
  }
  p.x = 0;
  p.y = 0;
  return p;
}

void wait(int runs) {
  for(int i = 0; i < runs; i++) {
    loop();
  }
  return;
}

// Main loop
void loop() {
  TS_Point p = boop(); // Checks for touch event

  // Prints buttons on display and responds to touch events
  switch (activeMenu) {
    case 1: // Main menu
    if (p.x >= BOXSTART_X && p.x <= BOXSTART_X2 - 10) {
      if (p.y >= BOXSTART_Y && p.y <= BOXSTART_Y2 - 10) {
        make_h_menu(1); // Play button selected
        paused = false; // If paused, play
      }
      else if (p.y >= BOXSTART_Y2 && p.y <= BOXSTART_Y2 + BOXSIZE) {
        make_h_menu(3); // File button selected
        if(playing) { // If playing, do nothing
          break;
        }
        make_file_menu(0, 0); // Switch to file menu (page = 0 to clear screen)
      }
    }
    else if (p.x >= BOXSTART_X2 && p.x <= BOXSTART_X2 + BOXSIZE * 2) {
      if (p.y >= BOXSTART_Y && p.y <= BOXSTART_Y2 - 10) {
        if(paused) {
          paused = false;
          stopped = true;
        }
        else if(playing){
          paused = true; // If playing, pause
        }
        make_h_menu(2); // Pause button selected
      }
      else if (p.y >= BOXSTART_Y2 && p.y <= BOXSTART_Y2 + BOXSIZE) {
        make_h_menu(4); // Test button selected
        led_test(); // Run basic LED strip test
      }
    }
    break;

    case 2: // File menu
    if (p.y >= BOXSTART_Y2 && p.y <= BOXSTART_Y2 + BOXSIZE) {
      if (p.x >= BOXSTART_X && p.x <= BOXSTART_X2 - 10) { // Back button selected
        if (page >= 2) { // If not on first page, display previous page
          page = page - 1; // Move back one page
          make_file_menu(page, 1);
        }
        else { // Else, return to main menu
          tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
          tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
          make_h_menu(0);
        }
      }
      else if (p.x >= BOXSTART_X2 && p.x <= BOXSTART_X2 + BOXSIZE * 2) { // Next button selected
        if (page < lastPage) { //If on last page, do nothing
          page = page + 1; // Move forward one page
          make_file_menu(page, 2);
        }
      }
    }
    else {
      for(int i = 0; i < lastFile; i++) {
        if(isEventFile[i] == 1) {
          int j = 10 + 8 * (i % 25);
          if(p.y >= j && p.y <= j + 8) {
            page = 1;
            Serial.println("EVENT SELECTED");
            displayEvents(fileNames[i]);
          }
        }
      }
    }
    break;

    default:
    break;
  }
}

// Tests each string with all 3 solid colors and a gradient to make sure that all leds are working and to help with mapping
void led_test() {
  while(playing) { // if something is already playing, exit led_test()
    return;
  }
  paused = false;
  playing = true;
  tft.fillRect(0, 0, 240, 218, ILI9341_BLACK); // Clear screen above buttons
  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Running LED Test 1");
  tft.setCursor(10, 10+8);
  int base = 10;
  for(int i = 0; i < NUMPIXELS / 60; i++) {
    loop(); // Run loop to check for touch event
    while (paused) { // If paused, wait until unpaused
      loop();
    }
    if(stopped) { // If stopped, clear pixels, clear screen, exit led_test()
      stopped = false;
      playing = false;
      clearPixels();
      tft.fillRect(0, 0, 240, 218, ILI9341_BLACK); // Clear screen above buttons
      return;
    }
    tft.setCursor(10, base + 8 * (i + 1));
    tft.print("String ");
    tft.print(i);
    tft.print(": RED ");
    setStringColor(i, MAX_BRIGHT, 0, 0); // Sets string to red
    loop(); // Run loop to check for touch event
    tft.print("GREEN ");
    setStringColor(i, 0, MAX_BRIGHT, 0); // Sets string to green
    loop(); // Run loop to check for touch event
    tft.print("BLUE ");
    setStringColor(i, 0, 0, MAX_BRIGHT); // Sets string to blue
    loop(); // Run loop to check for touch event
    tft.print("GRAD");
    setStringGrad(i, MAX_BRIGHT, 0, 0, 0, 0, MAX_BRIGHT); // Displays a gradient from red to blue
    loop(); // Run loop to check for touch event
    wait(500);
    setStringColor(i, 0, 0, 0);
    loop(); // Run loop to check for touch event
    if(i % 25 == 24) { // If output fills screen, clear screen and set cursor to top again
      tft.fillRect(10, 18, 230, 200, ILI9341_BLACK);
      base = base - 200;
    }
  }
  tft.setCursor(10, base + 8 * (NUMPIXELS / 60) + 1);
  tft.print("Done!");
  playing = false; // No longer playing
}

// Displays a solid color on a given string
void setStringColor(int stringNum, byte r, byte g, byte b) {
  int startPos = stringNum * 60; // sets start at beginning of string
  for(int i = 0; i < 60; i++) {
    pixels.setPixelColor(i + startPos, r, g, b);
  }
  Serial.print("string ");
  Serial.print(stringNum);
  Serial.println(": solid");
  pixels.show();
}

// Displays a gradient on a given string
void setStringGrad(int stringNum, byte ir, byte ig, byte ib, byte fr, byte fg, byte fb) {
  int startPos = stringNum * 60; // sets start at beginning of string
  byte r = ir;
  byte g = ig;
  byte b = ib;
  for(int i = 0; i < 60; i++) {
    pixels.setPixelColor(i + startPos, r, g, b);
    r = min(MAX_BRIGHT, r + (int)(((float)fr - (float)ir) / 60.0)); // steps from initial red to final red
    g = min(MAX_BRIGHT, g + (int)(((float)fg - (float)ig) / 60.0)); // steps from initial green to final green
    b = min(MAX_BRIGHT, b + (int)(((float)fb - (float)ib) / 60.0)); // steps from initial blue to final blue
    
  }
  Serial.print("string ");
  Serial.print(stringNum);
  Serial.println(": gradient");
  pixels.show();
}

void clearPixels() {
  for(int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, 0, 0, 0);
  }
  pixels.show();
}

void displayEvents(String filename) {
  while(playing) {
    return;
  }
  paused = false;
  playing = true;

  bool newevent = false;
  int event = 1;
  int frame = 1;
  char val[20];
  int led;
  int r;
  int g;
  int b;
  int pos = 0;
  int i = 0;
  File file = SD.open(filename);

  make_h_menu(0);

  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(filename);
  tft.setCursor(10, 10+8);
  tft.print("event: ");
  tft.print(event);
  tft.setCursor(10, 10+8+8);
  tft.print("frame: ");
  tft.print(frame);

  while(file.available()) {
    loop();
    while (paused) {
      loop();
    }
    if(stopped) {
      stopped = false;
      playing = false;
      clearPixels();
      tft.fillRect(0, 0, 240, 218, ILI9341_BLACK); // Clear screen above buttons
      return;
    }
    if(newevent) {
      clearPixels();
      tft.setCursor(10+8*5, 10+8);
      tft.fillRect(10+8*5, 10+8, 10+9*5, 10+8+8, ILI9341_BLACK);
      tft.print(event);
      pos = 0;
      newevent = false;
    }
    i = 0;
    memset(val, 0, sizeof(val));
    while(val[i-1] != '\n') {
      val[i] = file.read();
      i++;
    }
    if(val[0] == 'n') {
      Serial.println("END OF FRAME");
      frame++;
      pixels.show();
      tft.setCursor(10+8*5, 10+8+8);
      tft.fillRect(10+8*5, 10+8+8, 10+9*5, 10+8+8+8, ILI9341_BLACK);
      tft.print(frame);
      pos = 0;
    }
    else if(val[0] == 'x') {
      Serial.println("END OF EVENT");
      event++;
      paused = true;
      newevent = true;
    }
    else if(pos == 0) {
      led = atoi(val);
      Serial.print(led);
      Serial.print(" ");
      pos++;
    }
    else if(pos == 1) {
      r = atoi(val);
      Serial.print(r);
      Serial.print(" ");
      pos++;
    }
    else if(pos == 2) {
      g = atoi(val);
      Serial.print(g);
      Serial.print(" ");
      pos++;
    }
    else if(pos == 3) {
      b = atoi(val);
      Serial.println(b);
      int ir = min(r, MAX_BRIGHT);
      int ig = min(g, MAX_BRIGHT);
      int ib = min(b, MAX_BRIGHT);
      pixels.setPixelColor(led, ir, ig, ib);
      pos = 0;
    }
  }
  while (paused) {
    loop();
  }
  if(stopped) {
    stopped = false;
    playing = false;
    clearPixels();
    tft.fillRect(0, 0, 240, 218, ILI9341_BLACK); // Clear screen above buttons
    return;
  }
  clearPixels();
  tft.setCursor(10, 10+8+8+8);
  tft.print("Done!");
  playing = false; // No longer playing
}

void make_h_menu(int selection) {
  switch (selection) {
    case 1:
    tft.drawRect(BOXSTART_X, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    break;

    case 2:
    tft.drawRect(BOXSTART_X, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
    tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    break;

    case 3:
    tft.drawRect(BOXSTART_X, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    break;

    case 4:
    tft.drawRect(BOXSTART_X, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
    break;

    default:
    //do the pause/play buttons
    tft.fillScreen(ILI9341_BLACK);
    tft.fillRect(BOXSTART_X, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_GREEN);
    tft.setCursor(BOXSTART_X + 13, BOXSTART_Y + 13);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.println("  >");
    tft.fillRect(BOXSTART_X2, BOXSTART_Y, BOXSIZE * 2, BOXSIZE, ILI9341_RED);
    tft.setCursor(BOXSTART_X2 + 15, BOXSTART_Y + 13);
    tft.println(" ||");

    //do the button for submenu on files
    tft.fillRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_YELLOW);
    tft.setCursor(BOXSTART_X + 10, BOXSTART_Y2 + 13);
    tft.println("FILES");

    //do the test button
    tft.fillRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2/3, BOXSIZE, ILI9341_RED);
    tft.fillRect(BOXSTART_X2 + BOXSIZE * 2/3 , BOXSTART_Y2, BOXSIZE * 2/3 + 2, BOXSIZE, ILI9341_GREEN);
    tft.fillRect(BOXSTART_X2 + BOXSIZE * 4/3 , BOXSTART_Y2, BOXSIZE * 2/3, BOXSIZE, ILI9341_BLUE);
    tft.setCursor(BOXSTART_X2 + 16, BOXSTART_Y2 + 13);
    tft.println("TEST");

    tft.setCursor(30, 280);

    break;
  }

  activeMenu = 1;
  return;
}

void make_file_menu(int page, int selection) {
  activeMenu = 2;
  switch (selection) {
    case 1:
    if (page >= 1) {
      tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
      tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
      make_file_menu(page, 0);
    }
    break;

    case 2:
    if (page <= lastPage) {
      tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
      tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
      make_file_menu(page, 0);
    }
    break;

    default:
    if (page < 1) {
      tft.fillScreen(ILI9341_BLACK);
      page = 1;
    }
    tft.fillRect(0, 0, 240, 270, ILI9341_BLACK);
    tft.fillRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_YELLOW);
    tft.setCursor(BOXSTART_X + 16, BOXSTART_Y2 + 13);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.println("BACK");
    if (page < lastPage) {
      tft.fillRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_YELLOW);
      tft.setCursor(BOXSTART_X2 + 16, BOXSTART_Y2 + 13);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(2);
      tft.println("MORE");
    }
    else {
      tft.fillRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    }
    tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
    displayFiles(page);
    break;
  }
}

void displayFiles(int page) {
  uint8_t i = 25 * (page - 1);
  uint8_t j = 0;

  tft.setCursor(180, 240);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("page ");
  tft.print(page);
  tft.setCursor(10, 10);

  while (i <= 25 * page) {
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(10, 10 + 8 * j);
    j = j + 1;

    if (i >= lastFile) {
      // no more files
      break;
    }

    if (isEventFile[i] == 1) {
      tft.setTextColor(ILI9341_GREEN);
    }

    else if (isDir[i] == 1) {
      tft.setTextColor(ILI9341_BLUE);
    }

    tft.print(fileNames[i]);

    i++;
  }
}
