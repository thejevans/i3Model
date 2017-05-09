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
##### Converts from DOM/String format to LED value
##################################################################################
def flatten (dom, string):

    return led

##################################################################################
##### Converts event to Array
##################################################################################
def eventToArray (event, maxBrightness, frames):
    max_charge = max(event.T[3])
    brightness = (event.T[3] * maxBrightness / max_charge).astype(int)

    tbin       = np.arange(len(event.T[0])) / int(round(len(event.T[0]) / frames))
    wavelength = (700. - 300. * tbin / frames).astype(int)

    eventArray = np.array(None)

    for i, pulse in enumerate(event):
        [r, g, b] = wav2RGB(wavelength[i])

        r  *= brightness[i]
        g  *= brightness[i]
        b  *= brightness[i]
        led = flatten(pulse[1], pulse[2])

        np.append(eventArray, [led, r, g, b])

        if (i==len(event)-1) or (not wavelength[i]==wavelength[i+1]):
            np.append(eventArray, ['n'])

    return eventArray

##################################################################################
##### Converts from wavelength to RGB (0.0 to 1.0 scale)
##################################################################################
def wav2RGB(wavelength):
    w = int(wavelength)

    # color
    if w >= 380 and w < 440:
        R = -(w - 440.) / (440. - 350.)
        G = 0.0
        B = 1.0
    elif w >= 440 and w < 490:
        R = 0.0
        G = (w - 440.) / (490. - 440.)
        B = 1.0
    elif w >= 490 and w < 510:
        R = 0.0
        G = 1.0
        B = -(w - 510.) / (510. - 490.)
    elif w >= 510 and w < 580:
        R = (w - 510.) / (580. - 510.)
        G = 1.0
        B = 0.0
    elif w >= 580 and w < 645:
        R = 1.0
        G = -(w - 645.) / (645. - 580.)
        B = 0.0
    elif w >= 645 and w <= 780:
        R = 1.0
        G = 0.0
        B = 0.0
    else:
        R = 0.0
        G = 0.0
        B = 0.0

    # intensity correction
    if w >= 380 and w < 420:
        SSS = 0.3 + 0.7*(w - 350) / (420 - 350)
    elif w >= 420 and w <= 700:
        SSS = 1.0
    elif w > 700 and w <= 780:
        SSS = 0.3 + 0.7*(780 - w) / (780 - 700)
    else:
        SSS = 0.0

    return [int(SSS*R), int(SSS*G), int(SSS*B)]

##################################################################################
##### Parsing variables
##################################################################################
usage  = "%prog [options] --infile <input pickled file> --outfile <output text file> --nevents <num of events>"
parser = OptionParser(usage = usage)

parser.add_option("-n", "--nevents", type = "int", default = 0,
                  help = "number of events to send to arduino/display")
parser.add_option("-i", "--infile", type = "string",
                  default = './events.p',
                  help = "pickled file of all events")
parser.add_option("-o", "--outfile", type = "string",
                  default = './events.txt',
                  help = "text file of LED instructions")

(options, args) = parser.parse_args()
infile          = options.infile
outfile         = options.outfile
nevents         = options.nevents

print ('in_file: {0}'.format(infile))
print ('out_file: {0}'.format(outfile))
print (' ')

##################################################################################
##### Define basic variables
##################################################################################
all_events = cPickle.load(open(infile, 'rb'))
output     = open(outfile, 'w')

if nevents == 0: nevents = len(all_events)

max_brightness = 100. ## set max brightness to avoid burning the LEDs
frames = 32.

##################################################################################
##### Send events to text file: hit = [time, dom, string, charge]
##################################################################################
for i, event in enumerate(all_events):
    if nth == nevents: break

    output.write(eventToArray(event, max_brightness, frames))
    output.write("\nx\n")

    nth += 1

output.close()
