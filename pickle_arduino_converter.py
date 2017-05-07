#!/usr/bin/env python
from __future__ import print_function
######################################################################################
#### python command:
#### python pickle_arduino_converter.py --nevents <num of events to display>
####                                    --infile <input pickled files>
####                                    --outfile <output text file>
######################################################################################

from optparse import OptionParser
from os.path import expandvars
import cPickle
import numpy as np

##################################################################################
##### Parsing variables
##################################################################################
usage = "%prog [options] --infile <input pickled file> --outfile <output text file> --nevents <num of events>"
parser = OptionParser(usage=usage)
parser.add_option("-n", "--nevents", type="int", default = 0,
                  help = "number of events to send to arduino/display")
parser.add_option("-i", "--infile", type = "string",
                  default = './event.p',
                  help = "pickled file of all events")
parser.add_option("-o", "--outfile", type = "string",
                  default = './event.txt',
                  help = "text file of LED instructions")
(options, args) = parser.parse_args()
infile = options.infile
outfile = options.outfile
nevents = options.nevents

print ('in_file: {0}'.format(infile))
print ('out_file: {0}'.format(outfile))
print (' ')

##################################################################################
##### Define basic variables
##################################################################################
all_events = cPickle.load(open(infile, 'rb'))
output = open(outfile, 'wb')
if nevents == 0: nevents = len(all_events)
max_brightness = 100. ## set max brightness to avoid burning the LEDs

##################################################################################
##### Send events to text file: thishit = [time, dom, string, charge]
##################################################################################
for i, event in enumerate(all_events):
    if nth==nevents: break

    ## CHARGE: get max PE for scaling brightness
    max_charge = max(event.T[3])
    brightness = (event.T[3]*max_brightness/max_charge).astype(int)

    ## TIME: array to identify which bin the hits are in (total 32 bins))
    tbin = np.arange(len(event.T[0]))/ int(round(len(event.T[0])/32.))
    wavelength = (700. - 300.*tbin / 32.).astype(int)

    ## STRING: odd/even if two strings
    strings = event.T[2]%2+1 ## remainder 0 = 1st string; remainder 1 = 2nd string
    #strings = thisevent.T[2]

    for j, pulse in enumerate(thisevent):
        print ('### Pulse {0}: {1}, {2}, {3}, {4}'.format(j, wavelength[j], pulse[1], pulse[2], brightness[j]))

        ## send data to arduino
        data = str( (wavelength[j],pulse[1],strings[j],brightness[j]) )
        send_data( data )

        ## if this wavelength is different from next or if last pulse of this event;
        ## tell arduino to display
        if (j==len(event)-1) or (not wavelength[j]==wavelength[j+1]):
            send_data( 'd' )
            print ('### DISPLAY!')
            print ('### ')

    print ('### ')
    nth += 1
