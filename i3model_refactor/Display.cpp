#include "Display.h"
#include "Buffer.h"

Display::Display () {
    released = true;
    playFolders = true;
    activeMenu = 0;
    for(int i = 0; i < MAX_BUTTONS; i++) {
        for(int j = 0; j < 5; j++) {
            buttonMap[i][j] = 0;
        }
    }
}
