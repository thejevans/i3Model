#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>       // this is needed for display
#include <Adafruit_ILI9341.h>
#include <Wire.h>      // this is needed for FT6206
#include <Adafruit_FT6206.h>
#include <SD.h>


#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_FT6206 ts = Adafruit_FT6206();

#define BOXSIZE 40
#define BOXSTART_X 35
#define BOXSTART_X2 125
#define BOXSTART_Y 220
#define BOXSTART_Y2 270
#define BX_FI_SZ 40
#define BX_FI_X 40
#define BX_FI_Y 80

const int chipSelect = 4;

int page = 0;
File root;
bool moreFiles = false;
bool paused = false;
bool playing = false;
String fileNames[99];
int lastFile = 0;

//0 = none; 1 = h_menu; 2 = file_menu
int activeMenu = 0;

#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
#include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN            12


// we have 78 strings x 60 doms/string = 4680 total doms NeoPixels are attached to the Arduino?
#define NUMPIXELS      4680

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // wait for Arduino Serial Monitor
  
  tft.begin();
  ts.begin();
  pixels.begin();
  
  pixels.show();
  
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  root = SD.open("/");

  File entry = root.openNextFile();
  int i = 0;
  String k = "";
  while(entry) {
    fileNames[i] = "";
    k = entry.name();
    if (!entry.isDirectory()) {
      k = k + "\t\t\t\t\t\t\t";
      k = k + entry.size();
    }
    fileNames[i] = k;
    entry = root.openNextFile();
    i = i + 1;
    lastFile = lastFile + 1;
  }
  root.rewindDirectory();
  
  make_h_menu(0);
}

TS_Point boop() {
  TS_Point p;
  if (ts.touched()) {
    Serial.println("boop!");
    p = ts.getPoint();
    p.x = -(p.x - 240);
    p.y = -(p.y - 320);
    Serial.println(p.x);
    Serial.println(p.y);
    return p;
  }
  p.x = 0;
  p.y = 0;
  return p;
}

void loop() {
  TS_Point p = boop();
  switch (activeMenu) {
    case 1:
    if (p.x >= BOXSTART_X && p.x <= BOXSTART_X2 - 10) {
      if (p.y >= BOXSTART_Y && p.y <= BOXSTART_Y2 - 10) {
        make_h_menu(1);
        paused = false;
      }
      else if (p.y >= BOXSTART_Y2 && p.y <= BOXSTART_Y2 + BOXSIZE) {
        make_h_menu(3);
        if(playing) {
          break;
        }
        page = 1;
        moreFiles = false;
        make_file_menu(root, 0, 0);
      }
    }
    else if (p.x >= BOXSTART_X2 && p.x <= BOXSTART_X2 + BOXSIZE * 2) {
      if (p.y >= BOXSTART_Y && p.y <= BOXSTART_Y2 - 10) {
        make_h_menu(2);
        paused = true;
      }
      else if (p.y >= BOXSTART_Y2 && p.y <= BOXSTART_Y2 + BOXSIZE) {
        make_h_menu(4);
        led_test();
      }
    }
    break;

    case 2:
    if (p.x >= BOXSTART_X && p.x <= BOXSTART_X2 - 10) {
      if (p.y >= BOXSTART_Y2 && p.y <= BOXSTART_Y2 + BOXSIZE) {
        if (page >= 2) {
          page = page - 1;
          make_file_menu(root, page, 1);
        }
        else {
          tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
          tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
          make_h_menu(0);
        }
      }
    }
    else if (p.x >= BOXSTART_X2 && p.x <= BOXSTART_X2 + BOXSIZE * 2) {
      if (p.y >= BOXSTART_Y2 && p.y <= BOXSTART_Y2 + BOXSIZE) {
        if (page <= lastFile/25) {
          page = page + 1;
          make_file_menu(root, page, 2);
        }
      }
    }
    break;
    
    default:
    break;
  }
}

void led_test() {
  while(playing) {
    return;
  }
  paused = false;
  playing = true;
  tft.fillRect(0, 0, 240, 218, ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Running LED Test 1");
  tft.setCursor(10, 10+8);
  int base = 10;
  for(int i = 0; i < 78; i++) {
    loop();
    while (paused) {
      loop();
    }
    tft.setCursor(10, base + 8 * (i + 1));
    tft.print("String ");
    tft.print(i);
    tft.print(" RED ");
    setStringColor(i, 255, 0, 0);
    tft.print("GREEN ");
    setStringColor(i, 0, 255, 0);
    tft.print("BLUE");
    setStringColor(i, 0, 0, 255);
    setStringColor(i, 0, 0, 0);
    if(i % 25 == 24) {
      tft.fillRect(10, 18, 230, 200, ILI9341_BLACK);
      base = base - 200;
    }
  }
  tft.setCursor(10, base + 8 * 79);
  tft.print("Done!");
  playing = false;
}

void setStringColor(int stringNum, byte r, byte g, byte b) {
  int startPos = stringNum * 60;
  for(int i = startPos; i < startPos + 60; i++) {
    pixels.setPixelColor(i, r, g, b);
    //pixels.show();
  }
  Serial.println("lights");
  pixels.show();
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
    //tft.setTextColor(ILI9341_BLACK);  
    //tft.setTextSize(2);
    tft.println(" ||");

    //do the button for submenu on files
    tft.fillRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_YELLOW);
    tft.setCursor(BOXSTART_X + 10, BOXSTART_Y2 + 13);
    //tft.setTextColor(ILI9341_BLACK);  
    //tft.setTextSize(2);
    tft.println("FILES");

    //do the test button
    tft.fillRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2/3, BOXSIZE, ILI9341_RED);
    tft.fillRect(BOXSTART_X2 + BOXSIZE * 2/3 , BOXSTART_Y2, BOXSIZE * 2/3 + 2, BOXSIZE, ILI9341_GREEN);
    tft.fillRect(BOXSTART_X2 + BOXSIZE * 4/3 , BOXSTART_Y2, BOXSIZE * 2/3, BOXSIZE, ILI9341_BLUE);
    tft.setCursor(BOXSTART_X2 + 16, BOXSTART_Y2 + 13);
    //tft.setTextColor(ILI9341_BLACK);  
    //tft.setTextSize(2);
    tft.println("TEST");
  
    tft.setCursor(30, 280);
    //tft.setTextColor(ILI9341_CYAN);  
    //tft.setTextSize(2);
    //tft.println(event_file);
    break;
  }
  
  activeMenu = 1;
  return;
}

void make_file_menu(File dir, int page, int selection) {
  activeMenu = 2;
  switch (selection) {
    case 1:
    if (page >= 1) {
      tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
      tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
      make_file_menu(dir, page, 0);
    }
    break;

    case 2:
    if (moreFiles) {
      tft.drawRect(BOXSTART_X, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_BLACK);
      tft.drawRect(BOXSTART_X2, BOXSTART_Y2, BOXSIZE * 2, BOXSIZE, ILI9341_WHITE);
      make_file_menu(dir, page, 0);
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
    if (page <= lastFile/25) {
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
    displayFiles(dir, page);
    break;
  }
}

void displayFiles(File dir, int page) {
  uint8_t i = 25 * (page - 1);
  uint8_t j = 0;

  tft.setCursor(180, 240);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("page ");
  tft.print(page);
  
  tft.setCursor(10, 10);
  
  while (i <= 25 * page) {
    tft.setCursor(10, 10 + 8 * j);
    j = j + 1;
    
    if (i >= lastFile) {
      // no more files
      moreFiles = false;
      break;
    }

    tft.print(fileNames[i]);
    
    i = i + 1;
  }
  moreFiles = true;
}
