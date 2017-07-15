#ifndef i3Model_h
#define i3Model_h

class i3MClass {
public:
  i3MClass();
  void setup();
  void run();
private:

};

extern i3MClass i3Model;

typedef struct LED {
  LED() : index(0), color(CRGB(0,0,0)) {}
  LED(int in_index, CRGB in_color) : index(in_index), color(in_color) {}
  CRGB color;
  int index;
} LED;

typedef struct timeBin {
  TimeBin() {}
  void addLED(LED) {leds.push_back(in_LED);}
  vector<LED> leds;
} timeBin;

typedef struct Event {
  Event() : name(""), descriptiveName(""), date(""), id(""), path(""),
        energy(0), zenith(0), pid(0) {}
  Event(String in_Name, String in_DescriptiveName, String in_Date, String in_ID,
        String in_Path, double in_Energy, double in_Zenith, int in_PID) :
        name(in_Name), descriptiveName(in_DescriptiveName), date(in_Date),
        id(in_ID), path(in_Path), energy(in_Energy), zenith(in_Zenith),
        pid(in_PID) {}
  void addTimeBin(TimeBin in_TimeBin) {timeBins.push_back(in_TimeBin);}
  vector<TimeBin>* timeBins;
  String name;
  String descriptiveName;
  String date;
  String id;
  String path;
  double energy;
  double zenith;
  int pid;
} Event;

typedef struct Directory {
  Directory(String,String,vector<String>*,vector<String>*,vector<byte>*);
  Directory(String,vector<String>*,vector<String>*,vector<byte>*);
  void addFile(String, byte);
  bool addDescriptiveFileName(String,String);
  vector<String>* fileNames;
  vector<String>* descriptiveFileNames;
  vector<byte>* fileTypes;
  String path;
  String autoplay;
  bool isRoot;
  bool containsEvents;
} Directory;

#endif
