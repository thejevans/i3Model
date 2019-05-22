#!/usr/bin/env python
from __future__ import print_function
######################################################################################
#### Authors: John Evans & Elim Cheung
####
#### python command:
#### python write_data_to_SDcard.py --nevents <num of events to display>
####                                --infile <input pickled files>
####                                --frames <number of frames in animation>
####                                --port /dev/ttyACM0
####                                --baud_rate 115200
####                                (--verbose)
######################################################################################

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
        if verbose:
            print ('## sending data to arduino: {0}'.format(data))
        arduino.write(data)
        time.sleep(0.1)
    return

##################################################################################
##### Converts from DOM/String format to LED value
##################################################################################
def flatten (dom, string):
    #string = string % 2 + 1
    if string % 2 == 1:
        led = 60 * string - dom
    else:
        led = 60 * (string - 1) + (dom - 1)
    return led

##################################################################################
##### Convert event info to arduino info and send
##################################################################################
def send_event (hits, maxBrightness, frames):
    #### rescale charges
    max_charge = np.sqrt(max(hits.T[3]))
    brightness = (np.sqrt(hits.T[3]) * maxBrightness / max_charge).astype(int)
    #### adjust time bins
    tbin       = np.arange(len(hits.T[0])) / int(round(len(hits.T[0]) / frames))
    wavelength = (700. - 300. * tbin / frames).astype(int)
    #### convert time to color; send info
    for i, pulse in enumerate(hits):
        [r, g, b] = wav2RGB(wavelength[i])

        r  *= brightness[i]
        g  *= brightness[i]
        b  *= brightness[i]
        led = flatten(pulse[1], pulse[2])

        if (r > 0) or (g > 0) or (b > 0):
            data = str( (led, int(r), int(g), int(b)) )
            send_data (data)

        if i==len(hits)-1:
            data = 'x'
            send_data (data)
            break

        if not wavelength[i]==wavelength[i+1]:
            data = 'n'
            send_data (data)
    return 

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

    return [SSS*R, SSS*G, SSS*B]

##################################################################################
##### Parsing variables
##################################################################################
usage  = "%prog [options] --infile <input pickled file> --outfile <output i3rgb file> --nevents <num of events> --frames <num of frames>"
parser = OptionParser(usage = usage)

parser.add_option("-n", "--nevents", type = "int", default = 0,
                  help = "number of events to send to arduino/display")
parser.add_option("-i", "--infile", type = "string",
                  default = './events.p',
                  help = "pickled file of all events")
parser.add_option("-f", "--frames", type = "int", default = 32,
                  help = "number of frames in animation")
parser.add_option("-d", "--port", type = "string", default = "/dev/ttyACM0",
                  help = "arduino device port")
parser.add_option("-b", "--baud_rate", type="int", default = 9600,
                  help = "baud rate of arduino board")
parser.add_option("-v", "--verbose", action = "store_true", default = False,
                  help = "print progree as it goes")

(options, args) = parser.parse_args()
infile    = options.infile
nevents   = options.nevents
frames    = options.frames
port      = options.port
baud_rate = options.baud_rate
verbose   = options.verbose

if verbose:
    print ('in_file: {0}'.format(infile))
    print ('arduino: {0} with a baud rate of {1} Hz'.format(port, baud_rate))
    print ('send {0} events ..'.format(nevents))

##################################################################################
##### Define basic variables
##################################################################################
with open (infile, 'rb') as f:
    all_events = cPickle.load(f)
f.close()

if nevents==0: nevents=len(all_events['date'])
max_brightness = 100. ## set max brightness to avoid burning the LEDs

arduino = serial.Serial(port, baud_rate)
time.sleep(5)
##################################################################################
##### Send events to text file: hit = [time, dom, string, charge]
##################################################################################
nth = 0
for i in np.arange(len(all_events['date'])):
    if nth==nevents: 
        print ('nth {0} = nevents {1}'.format(nth, nevents))
        break
    #if verbose: 
    print (' ')
    print ('############### {0}th events ###############'.format(i))
    send_event (all_events['hits'][i], max_brightness, frames)
    nth += 1
    print ('========= nth : {0}'.format(nth))
send_data ('e')

if verbose: print ('## done! check .txt on the arduino side!')
