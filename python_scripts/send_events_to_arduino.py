#!/usr/bin/env python
from __future__ import print_function
######################################################################################
#### python command:
#### python send_events_to_arduino.py --nevents <num of events to display> 
####                                  --infile <input pickled files>
####                                  --port /dev/ttyACM0
####                                  --baud_rate 115200
######################################################################################

###################### PRINTOUTS ########################
import socket
print ('########################################################################################')
print ('#### This job is running on {0}'.format(socket.gethostname()))
print ('########################################################################################')
print (' ')
#########################################################
from optparse import OptionParser
from os.path import expandvars
import cPickle, serial, time
import numpy as np

##################################################################################
##### Function to send a string to arduino when its ready
##################################################################################
def send_data (data):
    flag = ''
    while not flag:
        arduino.flushInput()
        flag = arduino.read()
    if flag == 'n':
        arduino.flushInput()
        if print_info:
            print ('### sending data to arduino: {0}'.format(data))
        arduino.write(data)
        time.sleep(0.1)
    return

##################################################################################
##### Parsing variables
##################################################################################
usage = "%prog [options] --infile <input pickled file> --nevents <num of events>"
parser = OptionParser(usage=usage)
parser.add_option("-n", "--nevents", type="int", default = 0,
                  help = "number of events to send to arduino/display")
parser.add_option("-i", "--infile", type = "string",
                  default = '/data/i3home/elims/display3D/elimstuff/Run00127637_Subrun00000245.p',
                  help = "pickled file of all events")
parser.add_option("-d", "--port", type = "string", default = "/dev/ttyACM0",
                  help = "arduino device port")
parser.add_option("-b", "--baud_rate", type="int", default = 9600,
                  help = "baud rate of arduino board")
parser.add_option("-p", "--print_info", action='store_true', default = False,
                  help = "print information of everything")
(options, args) = parser.parse_args()
infile = options.infile
nevents = options.nevents
port = options.port
baud_rate = options.baud_rate
print_info = options.print_info

print ('in_file: {0}'.format(infile))
print ('arduino: {0} with a baud rate of {1} Hz'.format(port, baud_rate))
print (' ')
##################################################################################
##### Set up arduino ip address and basic variables
##################################################################################
all_events = cPickle.load(open(infile, 'rb'))
if nevents == 0: nevents = len(all_events)
max_brightness = 100. ## set max brightness to avoid burning the LEDs

arduino = serial.Serial(port, baud_rate)
time.sleep(5)
##################################################################################
##### Send events to arduino: thishit = [time, dom, string, charge]
##################################################################################
nth = 0

for i, event in enumerate(all_events):
    if nth==nevents: break
    ## start setting up info to be sent
    thisevent = all_events[i]
    print ('######################## Event {0} with {1} hits ########################'.format(i, len(thisevent)))
    ## send flag to arduino: starting a new event
    send_data('s')
    ## CHARGE: get max PE for scaling brightness
    max_charge = max(np.sqrt(thisevent.T[3]))
    brightness = (np.sqrt(thisevent.T[3])*max_brightness/max_charge).astype(int)
    ## TIME: array to identify which bin the hits are in (total 32 bins))
    tbin = np.arange(len(thisevent.T[0]))/ int(round(len(thisevent.T[0])/32.))
    wavelength = (700. - 300.*tbin / 32.).astype(int)
    ## STRING: odd/even if two strins
    strings = thisevent.T[2]
    send_data('s')
    for j, pulse in enumerate(thisevent):
        if print_info:
            print ('### Pulse {0}: {1}, {2}, {3}, {4}, {5}'.format
                   (j, thisevent.T[0][j], wavelength[j], thisevent[j][1], thisevent[j][2], brightness[j]) )
        ## send data to arduino
        data = str( (wavelength[j],thisevent[j][1],strings[j],brightness[j]) )
        send_data (data)
        ## if this wavelength is different from next or if last pulse of this event; 
        ## tell arduino to display
        if (j==len(thisevent)-1) or (not wavelength[j]==wavelength[j+1]):
            send_data( 'd' )
            if print_info:
                print ('### DISPLAY!')
                print ('### ')
    if print_info:
        print ('### ')
    nth+=1
    time.sleep(10)
    ## send flag to arduino: starting a new event
    send_data('s')

arduino.close()
