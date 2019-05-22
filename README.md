# UMD i3Model
## LED model of the IceCube Observatory

The display is a 3D array of 4800 NeoPixel LEDs arranged on one serial data pin. Data to be displayed is stored on a microSD card. The user interface is a 2.8" Adafruit capacative touch screen. These are all controlled by an Arduino Due.

To prepare files for the Due, there is a python-based converter that takes `.i3` files and converts them to `.I3R` format.

Hardware: Elim Thompson (Cheung), Greg Sullivan

Python software: Elim Thompson (Cheung), John Evans

Arduino software: John Evans, Elim Thompson (Cheung)

## Documentation

**Folder.txt (config) functionality**

Since update 1.1.0 Every time events are converted to the I3R format, the resulting file structure includes a file named “folder.txt” inside each directory. This is a config file that is parsed by the Arduino software whenever it parses the directory in which it resides.

Settings used in folder.txt:

syntax:

_property:_

	argument


has events:			_depreciated_

	This was used to determine whether or not the directory contained events. Better methods were used in later versions, but this option may show up in old files.

autoplay:			_only in root directory_

	This property is used to autoplay events. The argument passed is the absolute path to the directory containing the events wished to loop over. Upon parsing this setting on startup, the system will play all events in the directory in a loop.

maps:

	This property has a different syntax from the other properties available. ‘maps:’ must be the last property in a folder.txt file. All following lines will be parsed as if they are filename maps with syntax as follows:

maps:

	#####1			first 6 digits of a filename, generally numeric characters

	Descriptive file name 1

	#####2

	Descriptive file name 2

	…

	…

	It is not necessary that every file in the folder have a descriptive file name.

**.I3R Format**

Events are stored on a microSD card connected to the Arduino through the Adafruit TFT screen. The I3R file format is simply a text file with a different extension to allow the Arduino to easily determine event files. The format has multiple sections as determined by the following escape characters:

q 				_begin_ _header section_

The character ‘q’ can be found at the top of any I3R file created after version 1.0.0 and denotes the start of the header section of the file. The next five lines contain data pertaining to the event in the file as follows:

	date of event

	event ID

	event energy			_in TeV_

	Zenith				_in degrees_

	PID				_track (1), cascade (0), or undetermined (-1)_

	After these lines, the Arduino expects to start seeing LED data in this format:

	####				_LED address (0-4799)_

	###				_red value (0-255)_

	###				_green value (0-255)_

	###				_blue value (0-255)_

	####

	###

	###

	###

	...

	n				_end of time bin_

	####

	###

	###

	###

	...

	n

	...

	x				_end of event (depreciated)_

**Limits of microSD card**

The Arduino library for SD cards uses the original FAT file system. It does this mainly to save memory, but also for compatibility. This decision causes some problems that we have had to work around.



*   File names are truncated to 8 characters and case sensitivity in file names is lost. This also means that in order to avoid conflicts with the possibility that multiple files could have the same first 8 characters in their file names, the library truncates files with longer filenames to the first 6 characters of their name, followed by ‘~#’ where ‘#’ is an iterated value starting at 1. The following names:

		helloworlda.txt
	
		helloworldb.txt

	would then be truncated to:

		HELLOW~1.TXT

		HELLOW~2.TXT


    and since the file system does not guarantee alphabetic ordering, the knowledge of which file is which has been lost without examining the contents. To mitigate these problems, when converting events, we add a leading set of 6 incrementing numeric characters to files and use the folder.txt file in each directory to map these 6 characters to a distinct descriptive filename up to 255 characters in length that can contain any ASCII characters.



*   The library is not able to distinguish between hidden ‘system’ files, deleted files, and the files that we want it to recognize. To solve this, the following conditional is added to filter when crawling a directory on the SD card:

    	(!k.startsWith("_")) && (k != "TRASHE~1") && (k != "SPOTLI~1") && (k != "FSEVEN~1") && (k != "TEMPOR~1")

*   There is a limit to how many files and/or directories that one directory can contain of 512 files. In order to work around this, when converting events to I3R format, the converter generates a new folder when it hits a limit of 511 files (to allow for folder.txt). This has an added benefit of not overtaxing the arduino in terms of mapping descriptive file names and in terms of memory required to store strings.

**Logic level shifter**

The Arduino Due can only output 3.3-volt logic. The power supply we use provides the LEDs with 5 volts. The difference between these values means that the signal being sent to the LEDs is not reliable. In order to remedy this, we connected one channel of a 74AHCT125 Quad Level-Shifter between the Due and the LEDs, while providing the chip 5 volts from the power supply. This shifts the logic from 3.3 volts to 5 volts and removes reliability issues. Note: if using a 3.7-volt lithium-ion battery, or a board that can provide 5-volt logic, this will not be necessary.

**LED timing**

Due to the fact that we connected all 4800 LEDs in series, the time to send a signal to the LEDs is noticeable (~170ms). This timing is the same for both the Adafruit NeoPixel library and the FastLED library. To mitigate this to some degree, it may be possible to shave time off of the 1-bit by building a custom library in assembly. This solution was deemed unnecessarily complicated for our situation and we instead decided to reduce the number of time bins per event to make events play faster.

Another possible solution that we did not pursue is interlacing. If we put every odd LED string on one pin and every even string on another, we could theoretically cut the effective refresh rate in half without noticeable disruption of the event display.

One problematic side-effect of this is that during the time while LEDs are being updated, the TFT screen does not register touch events. This means that in order to pause a playing event, the user must hold the pause button until a touch event is registered.

**TFT screen**

To interface with the model, we use a TFT touchscreen from Adafruit. This screen also has an SD card reader attached that we use for event storage. The user interface on the screen has three main elements: the home screen, the file menu, and the event player.



*   _The home screen:_ Currently, this screen only consists of a button to access the file menu, but could easily contain more information or images.
*   _The file menu:_ When selected from the home screen, the file menu shows the contents of the root directory, with subdirectories having blue buttons and event files having green buttons. All non-event files are hidden from view. Along with those, there are also up to 3 control buttons. 

    The first is a button simply showing either ‘D’ or ‘E’ that determines whether selecting a directory containing events automatically plays all events in a loop, or the user is able to select individual events to play. In the second mode, the player will only be activated when a user selects an event directly from the file menu. The default mode is to automatically play entire directories.


    The second and third buttons are ‘BACK’ and ‘NEXT’ that allow the user to navigate a directory containing more than eight items, as only eight items fit on the screen at a time.

*   _The event player:_ When an event is playing on the LEDs, the touchscreen displays any metadata for the event (taken from the header of the .I3R file) starting at the top of the screen. At the bottom of the screen, there are control buttons for the player. There are three sets of buttons, depending on the state of the player: playing, paused, and event over. 

    While playing, the only button visible is the ‘PAUSE’ button. Due to the ~170ms required for the LEDs to update, the touchscreen does not respond well to the pause button. The pause button must be long-pressed until the system responds. At that point, the player will convert to the paused state. 


    While paused, the player will display up to four buttons. If the event is not the first event in its directory, there will be a ‘PREV’ button with the descriptive name of the previous event in the directory shown below it. If the event is not the last event in its directory, there will be a ‘NEXT’ button with the descriptive name of the next event in the directory shown below it. Pressing either of these will result in the player switching to either the previous or next event, respectively. The other two buttons are ‘PLAY’, which resumes the paused event, and ‘EXIT’, which exits the player and returns to the home screen.


    After an event is over, the player will show almost the same screen as while paused. The only difference is that instead of ‘PLAY’, there is a button labeled ‘REPLAY’ that restarts the event that just ended. If no button is pressed after ?? seconds, the player will start the next event in the directory. If there is no next event, the player will jump back to the first event in the directory.


**Functions and their definitions**

```cpp
void setup ()
```

Initial commands

	TS_Point boop ()

Handles touch events

	void wait (int timer)

Runs loop() a set number of times. Much better than delay.

	void loop ()

Main loop

	bool parseDir (bool initRun)

Parse directory

	bool parseDirText (bool initRun)

Parse folder.txt

	void play (int type, String arg)


Plays an event or test: type = (1 = ledTest(), 2 = displayEvents()), arg = passed to displayEvents()


	bool stopCheck (int timer)


Checks for pause and stop: timer = (< 0 = no timer, > 0 = number of loops)


	void ledTest ()


Tests each string with all 3 solid colors and a gradient to make sure that all leds are working and to help with mapping


	void displayEvents (String filename)


Display a selected event file: filename = name of event file in working directory to be displayed


	void setStringColor (int stringNum, byte r, byte g, byte b)


Displays a solid color on a given string: stringNum = string number (0 - 79), r,g,b = color values


	void setStringGrad (int stringNum, byte ir, byte ig, byte ib, byte fr, byte fg, byte fb) 


Displays a gradient on a given string: stringNum = string number (0 - 79), ir,ig,ib = initial color values, fr,fg,fb = final color values


	void clearPixels ()


Clears all LEDs


	void makeHomeMenu (int selection)


Make home menu. selection = (0 = no selection, 1 = top left, 2 = top right, 3 = bottom left, 4 = bottom right)


	void makeFileMenu (int selection, bool changedDir)


Make file menu. selection = (0 = no selection, 1 = back, 2 = next), changedDir = passed to displayFiles()


	bool displayFiles (bool changedDir)


Displays file and directory buttons: changedDir = True if the working directory has changed


	void pullFile(String filename)


Pull a file from the Serial port


_Could be used to add files to the SD card via USB without reprogramming the Arduino. Not fully implemented._


	void nextPulse()


signal python: next pulse


_Could be used to stream data from an external source to the LEDs, possibly for real-time presentation. Not fully implemented._
