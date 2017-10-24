#ifndef Display_h
#define Display_h

template<int MAX_BUTTONS>;

struct Display {
    bool released;
    bool playFolders;
    int activeMenu; // 0 = Home menu, 1 = File menu
    int buttonMap[MAX_BUTTONS][5];
    // Buttons:
    //  1 = Replay
    //  2 = Play
    //  3 = Pause
    //  4 = Files
    //  5 = Stop
    //  6 = Play Previous
    //  7 = Play Next
    //  8 = Back (Files)
    //  9 = Next (Files)
    // 10 = Play Mode (Files)
    //11+ = Items in File Menu (Files)
    Display();
};

#endif
