#include <Adafruit_NeoPixel.h>  
#ifdef __AVR__
#include <avr/power.h>
#endif
  
// Which pin on the Arduino is connected to the NeoPixels?
#define PIN            13
//max brightness
#define MAX_BRIGHT     100
// we have 78 strings x 60 doms/string = 4680 total doms NeoPixels are attached to the Arduino?
#define NUMPIXELS      120
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
  
//define pulse structure
struct Pulse {
  int wave; // which time bin of each hit (0~32)
  int bright;  // a PEs
  int dom;  // a dom number
  int str;  // a string number
};
  
// RGB structure to hold/return r,g,b as three values.
struct RGB {
  int r;
  int g;
  int b;
};
  
//temporarily store a string of number
//input_index: 0=wavelenth; 1=dom; 2=string; 3=brightness
String input;
int input_index;
//define event variables
struct Pulse pulse; //pulse structure: wave, bright, dom, string
// set the neopixels
struct RGB led;
int ir, ig, ib, loc;
  
void setup() {
  Serial.begin(9600);
  pixels.begin();
}
  
void loop() {

  nextpulse();
  if (Serial.available() > 0) {
    char ch = Serial.read();
    if (ch=='s') { //start a new event; turn all pixels off
      for (int ipixel = 0; ipixel < NUMPIXELS; ipixel++) {
        pixels.setPixelColor(ipixel, pixels.Color(0, 0, 0));
      }
      pixels.show();
      nextpulse();
    } else if (ch=='d') { //display when all pulses in this time bin is sent
      pixels.show();
      nextpulse();
    } else if (ch=='(') { //indicater of pulse info
      input_index = 0 ;
      input = "";
    } else if (ch == ',' || ch == ')') {
      if (input_index == 0) {
        pulse.wave = input.toInt();
        //Serial.println(pulse.wave);
      } else if (input_index == 1) {
        pulse.dom = input.toInt();
        //Serial.println(pulse.dom);
      } else if (input_index == 2) {
        pulse.str = input.toInt();
        //Serial.println(pulse.str);
      } else if (input_index == 3) {
        pulse.bright = input.toInt();
        //Serial.println(pulse.bright);
      }
      input_index += 1;
      input = "";
    } else { //if numbers characters
      input += ch;
    }
    //received a pulse; store pulse of this time bin
    if (ch == ')') {
      //determine wavelength (700nm ~ 400nm) from which time bin this is
      //32 bins of time red to violet
      led = color(pulse.wave, pulse.bright); 
      //brightness is rescaled in python; double sure not brighter than 100
      ir = min(led.r, MAX_BRIGHT);
      ig = min(led.g, MAX_BRIGHT);
      ib = min(led.b, MAX_BRIGHT);
      //translate string/dom to strip coordinate
      loc = get_pixel(pulse.str, pulse.dom);
      pixels.setPixelColor(loc, pixels.Color(ir, ig, ib));
      nextpulse();
    }
  }
}

//signal python: next pulse
void nextpulse() {
  Serial.write("n");
  delay(10);
  Serial.flush();
}

// returns pixel number from dom and string assumes strings 1-78 with doms 1-60 
// even from top; odd from bottom
int get_pixel(int str, int dom) {
  int pixel;
  if ( str%2 == 1) {     // odd string
    pixel = 60*str - dom;
  }
  else {                 // even string
    pixel = 60*(str-1) + (dom-1);
  }
  return pixel;
}

//translate wavelength/brighness to red, blue, green
struct RGB color(int wlength, int bness) {
  struct RGB a;
  if (wlength >= 380. && wlength < 440.) {
    a.r = int(bness * (440. - wlength) / 60.);
    a.g = 0;
    a.b = int(bness);
  }
  else if (wlength >= 440. && wlength < 490.) {
    a.r = 0;
    a.g = int(bness * (wlength - 440.) / 50.);
    a.b = int(bness);
  }
  else if (wlength >= 490. && wlength < 510.) {
    a.r = 0;
    a.g = int(bness);
    a.b = int(bness * (510. - wlength) / 20.);
  }
  else if (wlength >= 510. && wlength < 580.) {
    a.r = int(bness * (wlength - 510.) / 70.);
    a.g = int(bness);
    a.b = 0;
  }
  else if (wlength >= 580. && wlength < 645.) {
    a.r = int(bness);
    a.g = int(bness * (645. - wlength) / 65.);
    a.b = 0;
  }
  else if (wlength >= 645. && wlength < 780.) {
    a.r = int(bness);
    a.g = 0;
    a.b = 0;
  }
  else {
    a.r = 0.;
    a.g = 0.;
    a.b = 0.;
  }
  return a;
}
