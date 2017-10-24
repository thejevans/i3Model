#include "Parser.h"
#include "Display.h"
#include "Directory.h"
#include "Arduino.h"
#include <SD.h>               // SD card library

String Parser::readBuffer(char *val, File *file) {
    byte i = 0;
    char temp = '\0';
    memset(*val, '\0', sizeof(*val));
    while ((temp != '\n') && (temp >= 0)) { // Read entire line of file as a char array, ignoring spaces
        if ((i > 0)){ *val[i - 1] = temp; }
        if (!*file.available()) { break; }
        temp = *file.read();
        i++;
        if (temp == ' ') { i--; }
    }
    return String(*val);
}

void Parser::eventFile (int *ledList, int *timeBinIndicies, CRGB *color, Display *workingDisplay) {
    template<int MAX_LINE_LENGTH>;

    File event = File.open(*workingDisplay.currentEventFile);

    char val[MAX_LINE_LENGTH];

    byte r;
    byte g;
    byte b;
    byte pos = 0;
    byte i = 0;

    int timeBin = 0;
    int led;
    int j = 0;
    int prevLed;
    int numLeds = 0;

    bool header = false;

    // Reset arrays (needed for all events after first)
    memset(*ledList, 0, sizeof(*ledList));
    memset(*color, CRGB(0,0,0), sizeof(*color));
    memset(*timeBinIndicies, 0, sizeof(*timeBinIndicies));

    while ((*workingDisplay.playing) && (event.available()) && (timeBin < NUM_PIXELS)) {
        readBuffer(&val, &event);

        if (val[0] == 'q') { header = !header; }

        else if (header) { *workingDisplay.println(val); }

        else if (val[0] == 'n') { // If the first index of val is equal to the end time bin escape character, mark end time bin position
            Serial.println("END OF TIME BIN");
            *timeBinIndicies[timeBin] = numLeds;
            timeBin++;
            pos = 0;
        }

        else if (isDigit(val[0])){ // If the line is parsable as a number, then it is led information
            switch (pos) {
                case 0: // Parse LED index and add index to array
                led = atoi(val);
                *ledList[numLeds] = led;
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
                int ir = min(r, MAX_BRIGHTNESS);
                int ig = min(g, MAX_BRIGHTNESS);
                int ib = min(b, MAX_BRIGHTNESS);

                *color[numLeds] = CRGB(ir,ig,ib);
                pos = 0;
                numLeds++;
                break;
            }
        }
    }

    // Close the event file
    event.close();
}

void Parser::config(Directory *workingDirectory) {
    template<int MAX_LINE_LENGTH>;

    char val[MAX_LINE_LENGTH];

    bool mapsProperty = false;
    bool nextMap = false;
    bool autoplayProperty = false;

    int pos = 0;

    File file = SD.open(*workingDirectory.name + "FOLDER.TXT"); // Open folder.txt in working directory

    while (file.available()) {
        while (val == "") { readBuffer(&val, &file); }

        if (nextMap) { // If the previous line was a filename, this line is the map
            *workingDirectory.descriptiveFileNames[pos] = val;
            nextMap = false;
        }

        else if (autoplayProperty) { // If the previous line was the property for autoplay, this line is the value
            autoplayProperty = false;
            *workingDirectory.autoplay = true;
            *workingDirectory.autoplayDir = val;
        }

        else if (mapsProperty) { // If the previous line was the property for maps or a map, this is line is a filename
            // Very inefficient. Replace with better searching method.
            for (int j = 0; j < numFiles; j++) {
                int length = min(*workingDirectory.fileNames[j].length(), 6);
                if (val == *workingDirectory.fileNames[j].substring(0,length)) {
                    pos = j;
                    nextMap = true;
                    break;
                }
            }
        }

        else if (val == "autoplay:" && name == "/") { autoplayProperty = true; }
        else if (val == "maps:") { mapsProperty = true; }
    }

    // Close the folder.txt file
    file.close();
    return;
}
