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
#define BUTTON_SIZE_Y 40
#define BUTTON_START_X 35
#define BUTTON_START_X2 125
#define BUTTON_START_Y 270

// Set maximum brightness
#define MAX_BRIGHT 100

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 12

// We have 80 strings x 60 doms/string = 4800 total doms NeoPixels are attached to the Arduino.
#define NUM_PIXELS 4800

// Maximum number of characters per line in event files
#define MAX_LINE_LENGTH 4

// Set pin for SD card
#define CHIP_SELECT 4

// Designates number of files displayed per page of the file menu
#define FILES_PER_PAGE 8 

// Global variables for file management
String file_names[255]; // List of filenames in working directory
String working_dir = "/"; // Sets initial working directory as the root of the SD card
byte page = 1; // Controls which page of the file menu is active
byte index_of_last_file; // Last index of file_names that contains a file name for the working directory
byte last_page; // Last page of file menu for given working directory. Computed from index_of_last_file
byte file_type[255]; // 1 = directory, 2 = event
byte files_on_screen; // Number of files displayed on current page of the file menu

// Switch for pausing events or tests
bool paused = false;

// Switch for stopping events or tests
bool stopped = false;

// True if event or test is playing or paused. False if no event or test running
bool playing = false;

// False if continually pressing touchscreen
bool released = true;

// 0 = none; 1 = h_menu; 2 = file_menu
byte active_menu = 0;

// Array of pixels
CRGB pixels[NUM_PIXELS];

//--------------------------------------------------------------------------------------
// Initial commands
//--------------------------------------------------------------------------------------
void setup () {
  Serial.begin(250000); // Set baud rate of 250000
  while (!Serial) ; // wait for Arduino Serial Monitor

  delay(500); // needed to properly boot on first try

  // Set LED pin as output
  pinMode(PIN, OUTPUT);

  // Initialize touchscreen
  tft.begin();
  ts.begin();
  
  // Initialize FastLED
  FastLED.addLeds<NEOPIXEL, PIN>(pixels, NUM_PIXELS);

  // Clear NeoPixels
  clear_pixels();

  // Clear TFT
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  // End program if no SD card detected
  if (!SD.begin(CHIP_SELECT)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

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
  else { // Screen has not been touched, flip released flag
    released = true;
  }
  
  // If the touchscreen does not register a touch event or if released is false, set X, Y values to 0
  p.x = 0;
  p.y = 0;
  
  return p;
}

//--------------------------------------------------------------------------------------
// Runs loop() a set number of times. Much better than delay.
//--------------------------------------------------------------------------------------
void wait (int timer) {
  for (int i = 0; i < timer; i++) {
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
  switch (active_menu) {
    case 1: // Main menu
      if (p.y >= BUTTON_START_Y && p.y <= BUTTON_START_Y + BUTTON_SIZE_Y) {
        if (p.x >= BUTTON_START_X && p.x <= BUTTON_START_X2 - 10) { // Play or Pause or Replay or File button selected
          if (playing) { // If event or test is playing, then it is not the File button
            if (paused) {
              paused = false; // If paused, play
              make_h_menu(1);
              break;
            }
            paused = true; // Else if playing, pause
            make_h_menu(1);
            break;
          }
          make_h_menu(3); // Else file button selected
          page = 0; // page = 0 to clear screen
          make_file_menu(0, true); // Switch to file menu
        }
        else if (p.x >= BUTTON_START_X2 && p.x <= BUTTON_START_X2 + BUTTON_SIZE_Y * 2) {
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
      if (p.y >= BUTTON_START_Y && p.y <= BUTTON_START_Y + BUTTON_SIZE_Y) {
        if (p.x >= BUTTON_START_X && p.x <= BUTTON_START_X2 - 10) { // Back button selected
          if (page >= 2) { // If not on first page, display previous page
            page -= 1; // Move back one page
            make_file_menu(1, false);
          }
          else if (working_dir.lastIndexOf('/') == 0) { // Else if in root directory, return to main menu
            // Since make_file_menu is not called, we must highlight the selected button and clear the screen here
            tft.drawRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_WHITE);
            tft.drawRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
            tft.fillScreen(ILI9341_BLACK);
            
            make_h_menu(0);
          }
          else { // If not in root directory, modify working directory to move up one and redisplay file menu
            working_dir = working_dir.substring(0,working_dir.lastIndexOf('/'));
            working_dir = working_dir.substring(0,working_dir.lastIndexOf('/') + 1);
            Serial.println("moved to new working directory");
            Serial.println(working_dir);
            page = 1;
            make_file_menu(0, true);
          }
        }
        else if (p.x >= BUTTON_START_X2 && p.x <= BUTTON_START_X2 + BUTTON_SIZE_Y * 2) { // Next button selected
          if (page < last_page) { // If on last page, do nothing
            page += 1; // Move forward one page
            make_file_menu(2, false);
          }
        }
      }
      else { // Checks to see if an event file or a directory is selected
        int xstart;
        int ystart;
        int j;
        for (int i = 0; i < files_on_screen; i++) { // Loops through all button positions in the current page of the file menu
          // Sets the top left corner of the current button
          xstart = BUTTON_START_X - 16 + (i % 2) * (BUTTON_START_X2 + 16 - BUTTON_START_X);
          ystart = 10 + (int(i / 2) % 6) * (BUTTON_SIZE_Y + 10);

          // If a button has been selected, check to see if the file is a directory or event file
          if ((p.y >= ystart && p.y <= ystart + BUTTON_SIZE_Y) && (p.x >= xstart && p.x <= xstart + BUTTON_SIZE_Y * 2 + 16)) {
            // Move to correct file_names index
            j = i + (page - 1) * FILES_PER_PAGE;
            
            switch (file_type[j]) {
              case 1: // If a directory, change working directory to that directory and redisplay file menu
                working_dir += file_names[j] + "/";
                page = 0;
                Serial.println("moved to new working directory");
                Serial.println(working_dir);
                make_file_menu(0, true);
                break;
              
              case 2: // If an event file, play the event file
                play(2, file_names[j]);
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
void parse_dir (String path) {
  File dir = SD.open(path);
  
  // Returns to the beginning of the root directory
  dir.rewindDirectory();
  
  File entry = dir.openNextFile();
  int i = 0;
  String k = "";

  index_of_last_file = 0;
  last_page = 0;
  
  while (entry) {
    file_names[i] = "";
    k = entry.name();
    if (!k.startsWith("_")) { // All files that start with underlines are hidden or "deleted"
      if (entry.isDirectory()) { // If entry is a directory, flag it as such
        file_type[i] = 1;
        file_names[i] = k;
        i++;
        index_of_last_file++;
        Serial.println(k);
      }
      else if (k.substring(k.indexOf('.')) == ".I3R") { //If the file extension is .I3R, mark as event file
        file_type[i] = 2;
        file_names[i] = k;
        i++;
        index_of_last_file++;
        Serial.println(k);
      }
    }
    entry = dir.openNextFile();
  }
  
  // Update number of pages based on number of files
  last_page = (index_of_last_file - (index_of_last_file % FILES_PER_PAGE)) / FILES_PER_PAGE;
  if (index_of_last_file % FILES_PER_PAGE > 0) {
    last_page++;
  }

  dir.close();
  entry.close();
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

    case 2: // display_events() case
      display_events(arg);
      break;
  }

  // No longer playing
  playing = false;

  // Clear all LEDs
  clear_pixels();
  
  return;
}

//--------------------------------------------------------------------------------------
// Checks for pause and stop
//--------------------------------------------------------------------------------------
bool stop_check () {
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
  
  for (int i = 0; i < NUM_PIXELS / 60; i++) {
    // Check for pause
    if (stop_check()) { return; }

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
    set_string_color(i, MAX_BRIGHT, 0, 0);
    
    // Check for pause
    if (stop_check()) { return; }

    // Sets string to green
    tft.setCursor(xbase, ybase + 8 * (i + 1));
    tft.print("GREEN ");
    xbase = xbase + 6 * 6;
    set_string_color(i, 0, MAX_BRIGHT, 0);
    
    // Check for pause
    if (stop_check()) { return; }

    // Sets string to blue
    tft.setCursor(xbase, ybase + 8 * (i + 1));
    tft.print("BLUE ");
    xbase = xbase + 5 * 6;
    set_string_color(i, 0, 0, MAX_BRIGHT);
    
    // Check for pause
    if (stop_check()) { return; }

    // Displays a gradient from red to blue
    tft.setCursor(xbase, ybase + 8 * (i + 1));
    tft.print("GRAD");
    set_string_grad(i, MAX_BRIGHT, 0, 0, 0, 0, MAX_BRIGHT);

    // Delay to see gradient
    wait(1000);
    
    // Check for pause
    if (stop_check()) { return; }
    
    // Clear string
    set_string_color(i, 0, 0, 0);
    
    if (i % 25 == 24) { // If output fills screen, clear screen and set cursor to top again
      tft.fillRect(10, 18, 230, 200, ILI9341_BLACK);
      ybase = ybase - 200;
    }
  }

  // Print "Done!" at the end
  tft.setCursor(10, ybase + 8 * (NUM_PIXELS / 60) + 8);
  tft.print("Done!");
}

//--------------------------------------------------------------------------------------
// Display a selected event file
//--------------------------------------------------------------------------------------
void display_events (String filename) {
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
  int prev_led;
  int num_leds = 0;
  int led_list[NUM_PIXELS];
  CRGB color[NUM_PIXELS];
  int frame_indicies[255];
  File file = SD.open(working_dir + filename);

  //Text display NEEDS UPDATING
  tft.print(filename);
  tft.setCursor(10, 10+8);
  tft.print("event: ");
  tft.print(event);

  while (file.available()) {
    // Check for pause
    if (stop_check()) { return; }
    
    if (newevent) { // After each event, reset variables, clear all LEDs, update text display
      // Update current event number
      event++; 

      // Reset variables
      pos = 0;
      frame = 0;
      num_leds = 0;
      newevent = false;

      // Reset arrays
      memset(led_list, 0, sizeof(led_list));
      memset(color, 0, sizeof(color));
      memset(frame_indicies, 0, sizeof(frame_indicies));

      // Clear all LEDs
      clear_pixels();

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
      frame_indicies[frame] = num_leds;

      // Increment frame number
      frame++;

      // Reset position
      pos = 0;
    }
    else if (val[0] == 'x') { // If the first index of val is equal to the end-event escape character, display event to LEDs
      // Reset previous LED value
      prev_led = 0;
      
      for (int j = 0; j <= frame; j++) { // Loop through each frame
        for (int k = prev_led; k < frame_indicies[j]; k++) { // Loop through each LED in each frame
          // Set LED
          pixels[led_list[k]] = color[k];
        }
        // Check for pause
        if (stop_check()) { return; }

        //Display LEDs
        FastLED.show();

        // Set new starting position for inner for loop
        prev_led = frame_indicies[j];
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
          led_list[num_leds] = led;
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

          color[num_leds] = CRGB(ir,ig,ib);
          pos = 0;
          num_leds++;
          break;
      }
    }
  }
  // Check for pause
  if (stop_check()) { return; }

  // Print "Done!" at the end
  tft.setCursor(10, 10+8+8+8);
  tft.print("Done!");
  return;
}

//--------------------------------------------------------------------------------------
// Displays a solid color on a given string
//--------------------------------------------------------------------------------------
void set_string_color (int string_num, byte r, byte g, byte b) {
  // sets start at beginning of string
  int start_pos = string_num * 60;
  
  for (int i = 0; i < 60; i++) { // Loop through string and set each LED to color
    pixels[i+start_pos] = CRGB(r,g,b);
  }
  
  //Display LEDs
  FastLED.show();
  
  Serial.print("string ");
  Serial.print(string_num);
  Serial.println(": solid");
}

//--------------------------------------------------------------------------------------
// Displays a gradient on a given string
//--------------------------------------------------------------------------------------
void set_string_grad (int string_num, byte ir, byte ig, byte ib, byte fr, byte fg, byte fb) {
  // sets start at beginning of string
  int start_pos = string_num * 60;

  // Initialize r,g,b with starting values
  byte r = ir;
  byte g = ig;
  byte b = ib;
  
  for(int i = 0; i < 60; i++) { // Loop through string and set each LED to color
    // Set LED
    pixels[i+start_pos] = CRGB(r,g,b);
    
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
void clear_pixels () {
  for (int i = 0; i < NUM_PIXELS; i++) { // Loop through all LEDs and set to black
    pixels[i] = CRGB::Black;
  }

  // Display LEDs
  FastLED.show();
}

//--------------------------------------------------------------------------------------
// Make home menu. selection highlights selected box.
//--------------------------------------------------------------------------------------
void make_h_menu (int selection) {  
  active_menu = 1;
  switch (selection) {
    case 1: // highlight left box
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_WHITE);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      make_h_menu(0);
      break;

    case 2: // highlight right box
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_WHITE);
      make_h_menu(0);
      break;

    default:
      // Clear boxes
      tft.fillRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 5, BUTTON_SIZE_Y, ILI9341_BLACK);

      // Set text presets
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(2);

      if (paused) {
        // Make play button
        tft.fillRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_GREEN);
        tft.setCursor(BUTTON_START_X + 35, BUTTON_START_Y + 13);
        tft.println(">");

        // Make stop button
        tft.fillRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_RED);
        tft.setCursor(BUTTON_START_X2 + 33, BUTTON_START_Y + 13);
        tft.println("X");
      }
      
      else if (playing) {
        // Make pause button
        tft.fillRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_YELLOW);
        tft.setCursor(BUTTON_START_X + 29, BUTTON_START_Y + 13);
        tft.println("||");
      }

      else {
        // Make files button
        tft.fillRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_YELLOW);
        tft.setCursor(BUTTON_START_X + 10, BUTTON_START_Y + 13);
        tft.println("FILES");
  
        // Make test button
        tft.fillRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2/3, BUTTON_SIZE_Y, ILI9341_RED);
        tft.fillRect(BUTTON_START_X2 + BUTTON_SIZE_Y * 2/3 , BUTTON_START_Y, BUTTON_SIZE_Y * 2/3 + 2, BUTTON_SIZE_Y, ILI9341_GREEN);
        tft.fillRect(BUTTON_START_X2 + BUTTON_SIZE_Y * 4/3 , BUTTON_START_Y, BUTTON_SIZE_Y * 2/3, BUTTON_SIZE_Y, ILI9341_BLUE);
        tft.setCursor(BUTTON_START_X2 + 16, BUTTON_START_Y + 13);
        tft.println("TEST");
      }
      break;
  }
  return;
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void make_file_menu (int selection, bool changed_dir) {
  active_menu = 2;
  switch (selection) {
    case 1:
      if (page >= 1) {
        tft.drawRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_WHITE);
        tft.drawRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
        make_file_menu(0, changed_dir);
      }
      break;

    case 2:
      if (page <= last_page) {
        tft.drawRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
        tft.drawRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_WHITE);
        make_file_menu(0, changed_dir);
      }
      break;

    default:
      if (page < 1) {
        tft.fillScreen(ILI9341_BLACK);
        page = 1;
      }
      tft.fillRect(0, 0, 240, 270, ILI9341_BLACK);
      display_files(changed_dir);
      tft.fillRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_YELLOW);
      tft.setCursor(BUTTON_START_X + 16, BUTTON_START_Y + 13);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(2);
      tft.println("BACK");
      if (page < last_page) {
        tft.fillRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_YELLOW);
        tft.setCursor(BUTTON_START_X2 + 16, BUTTON_START_Y + 13);
        tft.setTextColor(ILI9341_BLACK);
        tft.setTextSize(2);
        tft.println("MORE");
      }
      else {
        tft.fillRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      }
      tft.drawRect(BUTTON_START_X, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      tft.drawRect(BUTTON_START_X2, BUTTON_START_Y, BUTTON_SIZE_Y * 2, BUTTON_SIZE_Y, ILI9341_BLACK);
      break;
  }
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void display_files (bool changed_dir) {
  if (changed_dir) {
    tft.setCursor(10, 10);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.println("Loading Files...");
    
    parse_dir(working_dir);
    
    tft.fillRect(10, 10, 10+6*16, 10+8, ILI9341_BLACK);
  }
  
  int xstart;
  int ystart;
  int j;
  String filename;
  files_on_screen = 0;
  
  for (int i = FILES_PER_PAGE * (page - 1); i < FILES_PER_PAGE * page; i++) {
    if (i + 1 > index_of_last_file) {
      // no more files
      break;
    }

    j = i - (page - 1) * FILES_PER_PAGE;
    
    xstart = BUTTON_START_X - 16 + (j % 2) * (BUTTON_START_X2 + 16 - BUTTON_START_X);
    ystart = 10 + (int(j / 2) % 6) * (BUTTON_SIZE_Y + 10);
    
    switch (file_type[i]) {
      case 1:
        tft.fillRect(xstart, ystart, BUTTON_SIZE_Y * 2 + 16, BUTTON_SIZE_Y, ILI9341_BLUE);
        filename = file_names[i];
        break;
        
      case 2:
        tft.fillRect(xstart, ystart, BUTTON_SIZE_Y * 2 + 16, BUTTON_SIZE_Y, ILI9341_GREEN);
        filename = file_names[i].substring(0, file_names[i].indexOf('.'));
        break;
    }
    
    tft.setCursor(xstart + 1, ystart + 13);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.println(filename);
    
    files_on_screen++;
  }
  
  // Build what happens when a directory is selected
  // 
  // 
  // 

  tft.setCursor(180, 240);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("page ");
  tft.print(page);
  tft.setCursor(10, 10);
}

void pull_file(String filename) {
  if (SD.exists(filename)) {
    SD.remove(filename);
  }
  
  File my_file = SD.open(filename, FILE_WRITE);
  delay(100);
  
  String input;
  char ch;
  String led;
  String r;
  String g;
  String b;
  int pos = 0;
  bool done = false;

  while (!done){
    nextpulse();
    if (Serial.available() > 0) {
      char ch = Serial.read();
      switch (ch) {
        case 'e': // end of sending
          my_file.close();
          done = true;
          break;
          
        case 'x': //start a new event
          my_file.println('x');
          nextpulse();
          break;
          
        case 'n': //end of this time bin
          my_file.println('n');
          nextpulse();
          break;
          
        case '(': //indicater of pulse info
          pos = 0 ;
          input = "";
          break;
          
        case ',':
        case ')':
          switch (pos) {
            case 0:
              //led = input.toInt();
              led = input;
              //my_file.write(led);
              my_file.println(led);
              break;
              
            case 1:
              //r = input.toInt();
              r = input;
              //my_file.write(r);
              my_file.println(r);
              break;
              
            case 2:
              //g = input.toInt();
              g = input;
              //my_file.write(g);
              my_file.println(g);
              break;
              
            case 3:
              //b = input.toInt();
              b = input;
              //my_file.write(b);
              my_file.println(b);
              break;
          }
          pos += 1;
          input = "";
          if (ch == ')') {
            nextpulse();
          }
          break;
          
        default:
          if (isDigit(ch)) {
            input += ch;
          }
          break;
      }
    }
  }
}

//signal python: next pulse
void nextpulse() {
  Serial.write("n");
  delay(2);
  Serial.flush();
}



