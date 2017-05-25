#include <Adafruit_GFX.h>     // Core graphics library
#include <SPI.h>              // this is needed for display
#include <Adafruit_ILI9341.h> // Display library
#include <Wire.h>             // this is needed for FT6206
#include <Adafruit_FT6206.h>  // Touch library
#include <SD.h>               // SD card library
#include <FastLED.h>          // FastLED library

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
#define BUTTONSIZE_X 40
#define BUTTONSTART_X 35
#define BUTTONSTART_X2 125
#define BUTTONSTART_Y 270

// Set maximum brightness
#define MAX_BRIGHT 100

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 12

// We have 80 strings x 60 doms/string = 4800 total doms NeoPixels are attached to the Arduino.
#define NUMPIXELS 4800

// Maximum number of characters per line in event files
#define MAX_LINE_LENGTH 4

// Set pin for SD card
const byte chipSelect = 4;

// Global variables for file management
byte page = 1;
String fileNames[255];
byte lastFile;
byte lastPage;
byte fileType[255];

// Switch for pausing events or tests
bool paused = false;

// Switch for stopping events or tests
bool stopped = false;

// True if event or test is playing or paused. False if no event or test running
bool playing = false;

// False if continually pressing touchscreen
bool released = true;

// 0 = none; 1 = h_menu; 2 = file_menu
byte activeMenu = 0;

// Array of pixels
CRGB pixels[NUMPIXELS];

//--------------------------------------------------------------------------------------
// Initial commands
//--------------------------------------------------------------------------------------
void setup () {
  Serial.begin(250000);
  while (!Serial) ; // wait for Arduino Serial Monitor

  delay(500); // needed to properly boot on first try

  // Set LED pin as output
  pinMode(PIN, OUTPUT);

  // Initialize touchscreen
  tft.begin();
  ts.begin();
  
  // Initialize FastLED
  FastLED.addLeds<NEOPIXEL, PIN>(pixels, NUMPIXELS);

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

  parseDir("/");

  // Display initial home menu
  tft.fillScreen(ILI9341_BLACK);
  make_h_menu(0);
}

//--------------------------------------------------------------------------------------
// Handles touch events
//--------------------------------------------------------------------------------------
TS_Point boop () {
  TS_Point p;
  if (ts.touched()) {
    if (released) { // The released flag prevents repreated touch events from being registered when the screen is long-pressed
      Serial.println("boop!");
      p = ts.getPoint();

      // Modifies X and Y coordinates to fit the orientation of the screen
      p.x = -(p.x - 240);
      p.y = -(p.y - 320);

      Serial.println(p.x);
      Serial.println(p.y);

      // Flip released flag now that the screen has been touched
      released = false;
      
      return p;
    }
    
  }
  else {
    released = true;
  }
  
  // If the touchscreen does not register a touch event, flip released flag and set X, Y values to 0
  p.x = 0;
  p.y = 0;
  
  return p;
}

//--------------------------------------------------------------------------------------
// Runs loop() a set number of times. Much better than delay.
//--------------------------------------------------------------------------------------
void wait (int runs) {
  for (int i = 0; i < runs; i++) {
    loop();
  }
  return;
}

//--------------------------------------------------------------------------------------
// Main loop
//--------------------------------------------------------------------------------------
void loop () {
  TS_Point p = boop(); // Checks for touch event

  // Prints buttons on display and responds to touch events
  switch (activeMenu) {
    case 1: // Main menu
      if (p.y >= BUTTONSTART_Y && p.y <= BUTTONSTART_Y + BUTTONSIZE_X) {
        if (p.x >= BUTTONSTART_X && p.x <= BUTTONSTART_X2 - 10) { // Play or Pause or File button selected
          if (playing) {
            if (paused) {
              paused = false; // If paused, play
              make_h_menu(1);
              break;
            }
            paused = true;
            make_h_menu(1); // Else if playing, pause
            break;
          }
          make_h_menu(3); // Else file button selected
          make_file_menu(0, 0); // Switch to file menu (page = 0 to clear screen)
        }
        else if (p.x >= BUTTONSTART_X2 && p.x <= BUTTONSTART_X2 + BUTTONSIZE_X * 2) {
          if (playing) { // If paused, stop button selected
            if (!paused) {
              break;
            }
            make_h_menu(2); // Stop button selected
            paused = false;
            stopped = true;
            break;
          }
          make_h_menu(4); // Test button selected
          play(1, ""); // Run basic LED strip test 
        }
      }
      break;

    case 2: // File menu
      if (p.y >= BUTTONSTART_Y && p.y <= BUTTONSTART_Y + BUTTONSIZE_X) {
        if (p.x >= BUTTONSTART_X && p.x <= BUTTONSTART_X2 - 10) { // Back button selected
          if (page >= 2) { // If not on first page, display previous page
            page = page - 1; // Move back one page
            make_file_menu(page, 1);
          }
          else { // Else, return to main menu
            tft.drawRect(BUTTONSTART_X, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_WHITE);
            tft.drawRect(BUTTONSTART_X2, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_BLACK);
            tft.fillScreen(ILI9341_BLACK);
            make_h_menu(0);
          }
        }
        else if (p.x >= BUTTONSTART_X2 && p.x <= BUTTONSTART_X2 + BUTTONSIZE_X * 2) { // Next button selected
          if (page < lastPage) { //If on last page, do nothing
            page = page + 1; // Move forward one page
            make_file_menu(page, 2);
          }
        }
      }
      else { // Checks to see if an event file is touched
        for (int i = 0; i < lastFile; i++) {
          if (fileType[i] == 2) {
            int j = 10 + 8 * (i % 25);
            if (p.y >= j && p.y <= j + 8) {
              page = 1;
              Serial.println("EVENT SELECTED");
              play(2, fileNames[i]); //Event file selected, run event
              break;
            }
          }
        }
      }
      break;
  }
}

//--------------------------------------------------------------------------------------
// Parse directory
//--------------------------------------------------------------------------------------
void parseDir (String path) {
  File dir = SD.open(path);
  File entry = dir.openNextFile();
  int i = 0;
  String k = "";

  lastFile = 0;
  lastPage = 0;
  
  while (entry) {
    fileNames[i] = "";
    k = entry.name();
    if (!k.startsWith("_")) { // All files that start with underlines are hidden or "deleted"
      if (entry.isDirectory()) { // If entry is a directory, flag it as such
        fileType[i] = 1;
        fileNames[i] = k;
        i++;
        lastFile++;
      }
      else if (k.substring(k.indexOf('.')) == ".I3R") { //If the file extension is .I3R, mark as event file
        fileType[i] = 2;
        Serial.println(k);
        fileNames[i] = k;
        i++;
        lastFile++;
      }
    }
    entry = dir.openNextFile();
  }
  
  // Update number of pages based on number of files
  lastPage = (lastFile - (lastFile % 25)) / 25;
  if (lastFile % 25 > 0) {
    lastPage++;
  }

  // Returns to the beginning of the root directory
  dir.rewindDirectory();
}

//--------------------------------------------------------------------------------------
// Plays an event or test
//--------------------------------------------------------------------------------------
void play (int type, String arg) {
  // if something is already playing, exit play()
  while (playing) { return; }

  // Set flags
  paused = false;
  playing = true;
  
  // Return to main menu and clear screen
  tft.fillScreen(ILI9341_BLACK);
  make_h_menu(0);

  // Set cursor position and font size
  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  
  switch (type) {
    case 1: // led_test() case
      led_test();
      break;

    case 2: // displayEvents() case
      displayEvents(arg);
      break;
  }

  // No longer playing
  playing = false;

  // Clear all LEDs
  clearPixels();
  
  return;
}

//--------------------------------------------------------------------------------------
// Checks for pause and stop
//--------------------------------------------------------------------------------------
bool stopCheck () {
  // Run loop to check for touch event
  loop();
  
  while (paused) { // If paused, wait until unpaused
    loop();
  }
  if (stopped) { // If stopped, clear pixels, clear screen
    stopped = false;
    playing = false;
    
    // Clear screen above buttons
    tft.fillRect(0, 0, 240, 218, ILI9341_BLACK);
    make_h_menu(0);
    
    return true;
  }
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  return false;
}

//--------------------------------------------------------------------------------------
// Tests each string with all 3 solid colors and a gradient to make sure that all leds are working and to help with mapping
//--------------------------------------------------------------------------------------
void led_test () {
  // Text Display
  tft.println("Running LED Test 1");
  tft.setCursor(10, 10+8);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);

  // Initialize base values for cursor position
  int ybase = 10;
  int xbase;
  
  for (int i = 0; i < NUMPIXELS / 60; i++) {
    // Check for pause
    if (stopCheck()) { return; }

    // Move cursor to next line and begin text display
    tft.setCursor(10, ybase + 8 * (i + 1));
    tft.print("String ");
    tft.print(i);
    if (i < 10) {
      xbase = 10 + 8 * 6;
    }
    else {
      xbase = 10 + 9 * 6;
    }

    // Sets string to red
    tft.print(": RED ");
    xbase = xbase + 6 * 6;
    setStringColor(i, MAX_BRIGHT, 0, 0);
    
    // Check for pause
    if (stopCheck()) { return; }

    // Sets string to green
    tft.setCursor(xbase, ybase + 8 * (i + 1));
    tft.print("GREEN ");
    xbase = xbase + 6 * 6;
    setStringColor(i, 0, MAX_BRIGHT, 0);
    
    // Check for pause
    if (stopCheck()) { return; }

    // Sets string to blue
    tft.setCursor(xbase, ybase + 8 * (i + 1));
    tft.print("BLUE ");
    xbase = xbase + 5 * 6;
    setStringColor(i, 0, 0, MAX_BRIGHT);
    
    // Check for pause
    if (stopCheck()) { return; }

    // Displays a gradient from red to blue
    tft.setCursor(xbase, ybase + 8 * (i + 1));
    tft.print("GRAD");
    setStringGrad(i, MAX_BRIGHT, 0, 0, 0, 0, MAX_BRIGHT);

    // Delay to see gradient
    wait(1000);
    
    // Check for pause
    if (stopCheck()) { return; }
    
    // Clear string
    setStringColor(i, 0, 0, 0);
    
    if (i % 25 == 24) { // If output fills screen, clear screen and set cursor to top again
      tft.fillRect(10, 18, 230, 200, ILI9341_BLACK);
      ybase = ybase - 200;
    }
  }

  // Print "Done!" at the end
  tft.setCursor(10, ybase + 8 * (NUMPIXELS / 60) + 8);
  tft.print("Done!");
}

//--------------------------------------------------------------------------------------
// Display a selected event file
//--------------------------------------------------------------------------------------
void displayEvents (String filename) {
  // Initialize variables
  bool newevent = false;
  int event = 1;
  int frame = 0;
  char val[MAX_LINE_LENGTH];
  int led;
  int r;
  int g;
  int b;
  int pos = 0;
  int i = 0;
  int prevLed;
  int numLeds = 0;
  int ledList[NUMPIXELS];
  CRGB color[NUMPIXELS];
  int frameIndicies[255];
  File file = SD.open(filename);

  //Text display NEEDS UPDATING
  tft.print(filename);
  tft.setCursor(10, 10+8);
  tft.print("event: ");
  tft.print(event);

  while (file.available()) {
    // Check for pause
    if (stopCheck()) { return; }
    
    if (newevent) { // After each event, reset variables, clear all LEDs, update text display
      // Update current event number
      event++; 

      // Reset variables
      pos = 0;
      frame = 0;
      numLeds = 0;
      newevent = false;

      // Reset arrays
      memset(ledList, 0, sizeof(ledList));
      memset(color, 0, sizeof(color));
      memset(frameIndicies, 0, sizeof(frameIndicies));

      // Clear all LEDs
      clearPixels();

      // Text display NEEDS UPDATING
      tft.setCursor(10+8*5, 10+8);
      tft.fillRect(10+8*5, 10+8, 10+9*5, 10+8+8, ILI9341_BLACK);
      tft.print(event);
    }
    
    // Reset variable
    i = 0;

    // Reset array
    memset(val, 0, sizeof(val));
    
    while (val[i-1] != '\n') { // Read entire line of file as a char array
      val[i] = file.read();
      i++;
    }
    if (val[0] == 'n') { // If the first index of val is equal to the end-frame escape character, mark end-frame position
      Serial.println("END OF FRAME");
      
      // Set index for end of current frame
      frameIndicies[frame] = numLeds;

      // Increment frame number
      frame++;

      // Reset position
      pos = 0;
    }
    else if (val[0] == 'x') { // If the first index of val is equal to the end-event escape character, display event to LEDs
      // Reset previous LED value
      prevLed = 0;
      
      for (int j = 0; j <= frame; j++) { // Loop through each frame
        for (int k = prevLed; k < frameIndicies[j]; k++) { // Loop through each LED in each frame
          // Set LED
          pixels[ledList[k]] = color[k];
        }
        // Check for pause
        if (stopCheck()) { return; }

        //Display LEDs
        FastLED.show();

        // Set new starting position for inner for loop
        prevLed = frameIndicies[j];
      }
      Serial.println("END OF EVENT");
      paused = true;
      newevent = true;
      make_h_menu(0);
    }
    else {
      switch (pos) {
        case 0: // Parse LED index and add index to array
          led = atoi(val);
          ledList[numLeds] = led;
          Serial.print(led);
          Serial.print(" ");
          pos++;
          break;
          
        case 1: // Parse LED red value
          r = atoi(val);
          Serial.print(r);
          Serial.print(" ");
          pos++;
          break;

        case 2: // Parse LED green value
          g = atoi(val);
          Serial.print(g);
          Serial.print(" ");
          pos++;
          break;
        
        case 3: // Parse LED blue value and add LED color to array
          b = atoi(val);
          Serial.println(b);

          // Make sure LEDs are not going over the max allowed brightness
          int ir = min(r, MAX_BRIGHT);
          int ig = min(g, MAX_BRIGHT);
          int ib = min(b, MAX_BRIGHT);

          color[numLeds] = CRGB(ir,ig,ib);
          pos = 0;
          numLeds++;
          break;
      }
    }
  }
  // Check for pause
  if (stopCheck()) { return; }

  // Print "Done!" at the end
  tft.setCursor(10, 10+8+8+8);
  tft.print("Done!");
  return;
}

//--------------------------------------------------------------------------------------
// Displays a solid color on a given string
//--------------------------------------------------------------------------------------
void setStringColor (int stringNum, byte r, byte g, byte b) {
  // sets start at beginning of string
  int startPos = stringNum * 60;
  
  for (int i = 0; i < 60; i++) { // Loop through string and set each LED to color
    pixels[i+startPos] = CRGB(r,g,b);
  }
  
  //Display LEDs
  FastLED.show();
  
  Serial.print("string ");
  Serial.print(stringNum);
  Serial.println(": solid");
}

//--------------------------------------------------------------------------------------
// Displays a gradient on a given string
//--------------------------------------------------------------------------------------
void setStringGrad (int stringNum, byte ir, byte ig, byte ib, byte fr, byte fg, byte fb) {
  // sets start at beginning of string
  int startPos = stringNum * 60;

  // Initialize r,g,b with starting values
  byte r = ir;
  byte g = ig;
  byte b = ib;
  
  for(int i = 0; i < 60; i++) { // Loop through string and set each LED to color
    // Set LED
    pixels[i+startPos] = CRGB(r,g,b);
    
    // steps from initial red to final red
    r = min(MAX_BRIGHT, r + (int)(((float)fr - (float)ir) / 60.0));

    // steps from initial green to final green
    g = min(MAX_BRIGHT, g + (int)(((float)fg - (float)ig) / 60.0));

    // steps from initial blue to final blue
    b = min(MAX_BRIGHT, b + (int)(((float)fb - (float)ib) / 60.0)); 
  }
  
  // Display LEDs
  FastLED.show();
  
  return;
}

//--------------------------------------------------------------------------------------
// Clears all LEDs
//--------------------------------------------------------------------------------------
void clearPixels () {
  for (int i = 0; i < NUMPIXELS; i++) { // Loop through all LEDs and set to black
    pixels[i] = CRGB::Black;
  }

  // Display LEDs
  FastLED.show();
}

//--------------------------------------------------------------------------------------
// Make home menu. selection highlights selected box.
//--------------------------------------------------------------------------------------
void make_h_menu (int selection) {  
  activeMenu = 1;
  switch (selection) {
    case 1: // highlight left box
      tft.drawRect(BUTTONSTART_X, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_WHITE);
      tft.drawRect(BUTTONSTART_X2, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_BLACK);
      make_h_menu(0);
      break;

    case 2: // highlight right box
      tft.drawRect(BUTTONSTART_X, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_BLACK);
      tft.drawRect(BUTTONSTART_X2, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_WHITE);
      make_h_menu(0);
      break;

    default:
      // Clear boxes
      tft.fillRect(BUTTONSTART_X2, BUTTONSTART_Y, BUTTONSIZE_X * 5, BUTTONSIZE_X, ILI9341_BLACK);

      // Set text presets
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(2);

      if (paused) {
        // Make play button
        tft.fillRect(BUTTONSTART_X, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_GREEN);
        tft.setCursor(BUTTONSTART_X + 35, BUTTONSTART_Y + 13);
        tft.println(">");

        // Make stop button
        tft.fillRect(BUTTONSTART_X2, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_RED);
        tft.setCursor(BUTTONSTART_X2 + 33, BUTTONSTART_Y + 13);
        tft.println("X");
      }
      
      else if (playing) {
        // Make pause button
        tft.fillRect(BUTTONSTART_X, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_YELLOW);
        tft.setCursor(BUTTONSTART_X + 29, BUTTONSTART_Y + 13);
        tft.println("||");
      }

      else {
        // Make files button
        tft.fillRect(BUTTONSTART_X, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_YELLOW);
        tft.setCursor(BUTTONSTART_X + 10, BUTTONSTART_Y + 13);
        tft.println("FILES");
  
        // Make test button
        tft.fillRect(BUTTONSTART_X2, BUTTONSTART_Y, BUTTONSIZE_X * 2/3, BUTTONSIZE_X, ILI9341_RED);
        tft.fillRect(BUTTONSTART_X2 + BUTTONSIZE_X * 2/3 , BUTTONSTART_Y, BUTTONSIZE_X * 2/3 + 2, BUTTONSIZE_X, ILI9341_GREEN);
        tft.fillRect(BUTTONSTART_X2 + BUTTONSIZE_X * 4/3 , BUTTONSTART_Y, BUTTONSIZE_X * 2/3, BUTTONSIZE_X, ILI9341_BLUE);
        tft.setCursor(BUTTONSTART_X2 + 16, BUTTONSTART_Y + 13);
        tft.println("TEST");
      }
      break;
  }
  return;
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void make_file_menu (int page, int selection) {
  activeMenu = 2;
  switch (selection) {
    case 1:
      if (page >= 1) {
        tft.drawRect(BUTTONSTART_X, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_WHITE);
        tft.drawRect(BUTTONSTART_X2, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_BLACK);
        make_file_menu(page, 0);
      }
      break;

    case 2:
      if (page <= lastPage) {
        tft.drawRect(BUTTONSTART_X, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_BLACK);
        tft.drawRect(BUTTONSTART_X2, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_WHITE);
        make_file_menu(page, 0);
      }
      break;

    default:
      if (page < 1) {
        tft.fillScreen(ILI9341_BLACK);
        page = 1;
      }
      tft.fillRect(0, 0, 240, 270, ILI9341_BLACK);
      tft.fillRect(BUTTONSTART_X, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_YELLOW);
      tft.setCursor(BUTTONSTART_X + 16, BUTTONSTART_Y + 13);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(2);
      tft.println("BACK");
      if (page < lastPage) {
        tft.fillRect(BUTTONSTART_X2, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_YELLOW);
        tft.setCursor(BUTTONSTART_X2 + 16, BUTTONSTART_Y + 13);
        tft.setTextColor(ILI9341_BLACK);
        tft.setTextSize(2);
        tft.println("MORE");
      }
      else {
        tft.fillRect(BUTTONSTART_X2, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_BLACK);
      }
      tft.drawRect(BUTTONSTART_X, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_BLACK);
      tft.drawRect(BUTTONSTART_X2, BUTTONSTART_Y, BUTTONSIZE_X * 2, BUTTONSIZE_X, ILI9341_BLACK);
      displayFiles(page);
      break;
  }
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void displayFiles (int page) {
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

    if (fileType[i] == 2) {
      tft.setTextColor(ILI9341_GREEN);
    }

    else if (fileType[i] == 1) {
      tft.setTextColor(ILI9341_BLUE);
    }

    tft.print(fileNames[i]);

    i++;
  }
}
