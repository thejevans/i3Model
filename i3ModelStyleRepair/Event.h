#ifndef Event_h
#define Event_h

#include <StandardCplusplus.h>
#include <vector>
#include <Arduino.h>
#include "TimeBin.h"

struct Event {
  Event() : name(""), descriptiveName(""), date(""), id(""), path(""),
        energy(0), zenith(0), pid(0) {}
  Event(String in_Name, String in_DescriptiveName, String in_Date, String in_ID,
        String in_Path, double in_Energy, double in_Zenith, int in_PID) :
        name(in_Name), descriptiveName(in_DescriptiveName), date(in_Date),
        id(in_ID), path(in_Path), energy(in_Energy), zenith(in_Zenith),
        pid(in_PID) {}
  void addTimeBin(TimeBin in_TimeBin) {timeBins.push_back(in_TimeBin);}
  void addTimeBins(vector<TimeBin>);
  vector<TimeBin>* timeBins;
  String name;
  String descriptiveName;
  String date;
  String id;
  String path;
  double energy;
  double zenith;
  int pid;
};

#endif
