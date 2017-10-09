#define MAX_BUTTONS 20 //Maximum buttons on the screen

bool released = true;
int buttonMap[MAX_BUTTONS][5];
// Buttons:
//  1 =
//  2 =
//  3 =
//  4 =
//  5 =
//  6 =
//  7 =
//  8 =
//  9 =
// 10 =
//11+ = 
TS_Point boop () {
    TS_Point p;
    if (ts.touched()) {
        if (released) {
            p = ts.getPoint();

            // Modifies X and Y coordinates to fit the orientation of the screen
            p.x = 240 - p.x;
            p.y = 320 - p.y;
            released = false;
            return p;
        }
    }
    else { released = true; }
    p.x = 0;
    p.y = 0;
    return p;
}

void loop () {
    TS_Point p = boop();
    int button = buttonPressed(activeMenu,p);

    switch (button) {
        case 0: //None
        break;

        case 1: //Replay
        replay = true;
        eventOver = false;
        //Continue to case 2 (Play)

        case 2: //Play
        paused = false;
        makeHomeMenu();
        break;

        case 3: //Pause
        paused = true;
        makeHomeMenu();
        break;

        case 4: //Files
        makeHomeMenu();
        page = 0; // page = 0 to clear screen
        workingDir = "/";
        makeFileMenu(true); // Switch to file menu
        break;

        case 5: //Stop
        makeHomeMenu();
        paused = false;
        stopped = true;
        break;

        case 6: //Play Previous
        playPrev = true;
        paused = false;
        stopped = true;
        break;

        case 7: //Play Next
        playNext = true;
        paused = false;
        stopped = true;
        break;

        case 8: //Back (Files)
        if (page >= 2) { // If not on first page, display previous page
            page -= 1; // Move back one page
            makeFileMenu(false);
        }
        else if (workingDir == "/") { // Else if in root directory, return to main menu
            tft.fillScreen(ILI9341_BLACK);
            makeHomeMenu();
        }
        else { // Else move to parent directory
            workingDir = workingDir.substring(0,workingDir.lastIndexOf('/'));
            workingDir = workingDir.substring(0,workingDir.lastIndexOf('/') + 1);
            Serial.println("moved to new working directory");
            Serial.println(workingDir);
            page = 1;
            makeFileMenu(true);
        }
        break;

        case 9: //Next (Files)
        page += 1;
        makeFileMenu(false);
        break;

        case 10: //Play Mode (Files)
        playFolders = !playFolders;
        makeFileMenu(false);
        break;

        default: //Item in File Menu
        int fileIndex = button - 11;
        switch (fileType[fileIndex]) {
            case 1: // If directory, move to that directory
            workingDir += fileNames[fileIndex] + "/";
            page = 0;
            Serial.println("moved to new working directory");
            Serial.println(workingDir);
            makeFileMenu(true);
            break;

            case 2: // If event, play that event
            play(2, fileNames[fileIndex]);
            while ((playNext) || (playPrev)) { // If prev. or next selected, play that event
                clearPixels();
                paused = false;

                if (playNext){ // Play next event
                    playNext = false;
                    playPrev = false;
                    makeHomeMenu();
                    Serial.println("NEXT");
                    play(2, nextEventFile);
                }

                else { // Play prev. event
                    playNext = false;
                    playPrev = false;
                    makeHomeMenu();
                    Serial.println("PREV");
                    play(2, prevEventFile);
                }
            }
            clearPixels();
            // Clear screen above buttons
            tft.fillRect(0, 0, 240, 218, ILI9341_BLACK);
            makeHomeMenu();
            break;
        }
        break;
    }
}

void wait (int timer) {
    for (int i = 0; i < timer; i++) {
        loop();
    }
    return;
}

void clearPixels () {
    for (int i = 0; i < NUM_PIXELS; i++) { // Loop through all LEDs and set to black
        pixels[i] = CRGB::Black;
    }

    // Display LEDs
    FastLED.show();
}

bool openDir () {
    clearDirGlobals();
    parseDir();

    // If the directory has a folder.txt, parse it
    if (hasFolderText) { parseDirText(); }

    // If folder playing is turned on, play the first file in the folder
    if ((playFolders) && (hasEvents) && (workingDir != "/")) {
        playFolder();
        return true;
    }
    return false;
}

void parseDir () {
    int i = 0;
    String k = "";
    File dir = SD.open(workingDir);
    dir.rewindDirectory();
    File entry = dir.openNextFile();

    while (entry) { // While there is a next file in the directory, check for file type
        k = entry.name();
        if ((!k.startsWith("_")) && (k != "TRASHE~1") && (k != "SPOTLI~1") && (k != "FSEVEN~1") && (k != "TEMPOR~1")) { // All files that start with underlines are hidden or "deleted"
          if (k == "FOLDER.TXT") { // If entry is a folder.txt file, parse the file
              hasFolderText = true;
          }
          else if (entry.isDirectory()) { // If entry is a directory, flag it as such
              fileType[i] = 1;
              fileNames[i] = k;
              i++;
              indexOfLastFile++;
              Serial.println(k);
          }
          else if (k.substring(k.indexOf('.')) == ".I3R") { //If the file extension is .I3R, mark as event file
              fileType[i] = 2;
              fileNames[i] = k;
              i++;
              indexOfLastFile++;
              totalEventsInWorkingDir++;
              Serial.println(k);
              hasEvents = true;
          }
        }

        // Close the file and open the next one
        entry.close();
        entry = dir.openNextFile();
    }

    // Update number of pages based on number of files
    lastPage = (indexOfLastFile - (indexOfLastFile % FILES_PER_PAGE)) / FILES_PER_PAGE;
    if (indexOfLastFile % FILES_PER_PAGE > 0) {
        lastPage++;
    }

    // Close the file and directory
    dir.close();
    entry.close();
}

void playFolder () {
    makeHomeMenu(0);
    play(2, fileNames[0]);
    while ((playNext) || (playPrev)) { // If prev. or next selected, play that event
        clearPixels();
        paused = false;

        if (playNext){ // Play next event
            playNext = false;
            playPrev = false;
            makeHomeMenu(4);
            Serial.println("NEXT");
            play(2, nextEventFile);
        }

        else { // Play prev. event
            playNext = false;
            playPrev = false;
            makeHomeMenu(3);
            Serial.println("PREV");
            play(2, prevEventFile);
        }
    }
    clearPixels();
    // Clear screen above buttons
    tft.fillRect(0, 0, 240, 218, ILI9341_BLACK);
    makeHomeMenu(0);
    return;
}

void clearDirGlobals () {
    indexOfLastFile = 0;
    totalEventsInWorkingDir = -1;
    hasEvents = false;
    hasFolderText = false;

    for (int j = 0; j < 255; j++) {
        descriptiveFileNames[j] = "";
        fileNames[j] = "";
    }

    return;
}

void parseDirText () {
    bool mapsProperty = false;
    bool nextMap = false;
    bool autoplayProperty = false;

    int pos = 0;

    File file = SD.open(workingDir + "FOLDER.TXT"); // Open folder.txt in working directory

    while (file.available()) {
        String val = "";
        while (val == "") { val = readBuffer(file); }

        if (nextMap) { // If the previous line was a filename, this line is the map
            descriptiveFileNames[pos] = val;
            nextMap = false;
        }

        else if (autoplayProperty) { // If the previous line was the property for autoplay, this line is the value
            autoplayProperty = false;
            workingDir = workingDir + val;
            openDir();
        }

        else if (mapsProperty) { // If the previous line was the property for maps, all following lines are maps
            // Very inefficient. Replace with better searching method.
            for (int j = 0; j < indexOfLastFile; j++) {
                int length = min(fileNames[j].length(), 6);
                if (val == fileNames[j].substring(0,length)) {
                    pos = j;
                    nextMap = true;
                    break;
                }
            }
        }

        else if (val == "autoplay:" && workingDir == "/") { autoplayProperty = true; }
        else if (val == "maps:") { mapsProperty = true; }
    }

    // Close the event file
    file.close();
    return;
}

String readBuffer(File file) {
    byte i = 0;
    char temp = '\0';
    char val[255];
    memset(val, '\0', sizeof(val));
    while ((temp != '\n') && (temp >= 0)) { // Read entire line of file as a char array, ignoring spaces
        if ((i > 0)){ val[i - 1] = temp; }
        if (!file.available()) { break; }
        temp = file.read();
        i++;
        if (temp == ' ') { i--; }
    }
    return String(val);
}
