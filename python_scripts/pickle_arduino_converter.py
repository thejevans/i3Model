#!/usr/bin/env python
from __future__ import print_function
######################################################################################
#### python command:
#### python pickle_arduino_converter.py --infile <input pickled file>
####                                    --outdir <directory to contain I3R files of LED instructions>
####                                    --bins <number of time bins in animation>
######################################################################################

from optparse import OptionParser
import os
from os.path import expandvars
import shutil
import cPickle
import numpy as np

##################################################################################
##### Converts from DOM/String format to LED value
##################################################################################
def flatten (dom, string):
    if string % 2 == 1:
        led = 60 * string - dom
    else:
        led = 60 * (string - 1) + (dom - 1);

    return led

##################################################################################
##### Converts event to Array
##################################################################################
def eventToArray (event, maxBrightness, totalBins):
    max_charge = np.sqrt(max(event.T[3]))
    brightness = (np.sqrt(event.T[3]) * maxBrightness / max_charge).astype(int)

    tbin       = np.arange(len(event.T[0])) / int(round(len(event.T[0]) / totalBins))
    wavelength = (700. - 300. * tbin / totalBins).astype(int)

    eventArray = np.array([])

    for i, pulse in enumerate(event):
        [r, g, b] = wav2RGB(wavelength[i])

        r  *= brightness[i]
        g  *= brightness[i]
        b  *= brightness[i]
        led = flatten(pulse[1], pulse[2])

        if (r > 0) or (g > 0) or (b > 0):
            eventArray = np.append(eventArray, [led, int(r), int(g), int(b)])

        if (i==len(event)-1) or (not wavelength[i]==wavelength[i+1]):
            eventArray = np.append(eventArray, ['n'])

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

    return [SSS*R, SSS*G, SSS*B]

##################################################################################
##### Parsing variables
##################################################################################
usage  = "%prog [options] --infile <input pickled file> --outdir <output I3R directory> --bins <num of time bins>"
parser = OptionParser(usage = usage)

parser.add_option("-i", "--infile", type = "string",
                  default = './events.p',
                  help = "pickled file of all events")
parser.add_option("-o", "--outdir", type = "string",
                  default = 'events',
                  help = "directory to contain I3R files of LED instructions")
parser.add_option("-b", "--bins", type = "int", default = 32,
                  help = "number of time bins in animation")

(options, args) = parser.parse_args()
infile          = options.infile
outdir          = options.outdir
bins            = options.bins

outdir = infile[0:infile.find('.')] if outdir == "events" else outdir
outdir = outdir if outdir.startswith(('/','.','~')) else "./" + outdir
outdir = outdir if outdir.endswith('/') else outdir + "/"

print ('in_file: {0}'.format(infile))
print ('out_dir: {0}'.format(outdir))
print (' ')

##################################################################################
##### Define basic variables
##################################################################################
all_events = cPickle.load(open(infile, 'rb'))

if not os.path.exists(os.path.dirname(outdir)):
    try:
        os.makedirs(os.path.dirname(outdir))
    except OSError as exc: # Guard against race condition
        if exc.errno != errno.EEXIST:
            raise

max_brightness = 100. ## set max brightness to avoid burning the LEDs

##################################################################################
##### Send events to text file: hit = [time, dom, string, charge]
##################################################################################

text = open(outdir + 'folder.txt', 'w')

text.write("contains events:\nfalse\n\nmaps:\nALL\nAll\n\nTRACKS\nTracks\n\nCASCAD" + "\nCascades\n\nUNDETE\nUndetermined")

text.close()

if not os.path.exists(os.path.dirname(outdir + 'all/')):
    try:
        os.makedirs(os.path.dirname(outdir + 'all/'))
    except OSError as exc: # Guard against race condition
        if exc.errno != errno.EEXIST:
            raise

firstTrack = True
firstCascade = True
firstUndetermined = True

allText = open(outdir + 'all/folder.txt', 'w')
allText.write("contains events:\ntrue\n\nmaps:\n")

for i, event in enumerate(all_events['hits']):


    output = open(outdir + 'all/' + all_events['id'][i] + '.I3R', 'w')

    output.write("q\n%s\n%s\n%s\n%s\n%s\n" % (all_events['date'][i], all_events['id'][i], all_events['energy'][i], all_events['zenith'][i], all_events['pid'][i]))

    for item in eventToArray(event, max_brightness, bins):
        output.write("%s\n" % item)

    output.close()

    allText.write("%06d\n%s\n\n" % (i,all_events['id'][i]))

    if all_events['pid'][i] == 1:
        if firstTrack:
            if not os.path.exists(os.path.dirname(outdir + 'tracks/')):
                try:
                    os.makedirs(os.path.dirname(outdir + 'tracks/'))
                except OSError as exc: # Guard against race condition
                    if exc.errno != errno.EEXIST:
                        raise

            trackText = open(outdir + 'tracks/folder.txt', 'w')
            trackText.write("contains events:\ntrue\n\nmaps:\n")
            firstTrack = False

        trackText.write("%06d\n%s\n\n" % (i,all_events['id'][i]))
        shutil.copy(outdir + 'all/' + all_events['id'][i] + '.I3R', outdir + 'tracks/')

    elif all_events['pid'][i] == 0:
        if firstCascade:
            if not os.path.exists(os.path.dirname(outdir + 'cascades/')):
                try:
                    os.makedirs(os.path.dirname(outdir + 'cascades/'))
                except OSError as exc: # Guard against race condition
                    if exc.errno != errno.EEXIST:
                        raise

            cascadeText = open(outdir + 'cascades/folder.txt', 'w')
            cascadeText.write("contains events:\ntrue\n\nmaps:\n")
            firstCascade = False

        cascadeText.write("%06d\n%s\n\n" % (i,all_events['id'][i]))
        shutil.copy(outdir + 'all/' + all_events['id'][i] + '.I3R', outdir + 'cascades/')

    else:
        if firstUndetermined:
            if not os.path.exists(os.path.dirname(outdir + 'undetermined/')):
                try:
                    os.makedirs(os.path.dirname(outdir + 'undetermined/'))
                except OSError as exc: # Guard against race condition
                    if exc.errno != errno.EEXIST:
                        raise

            undeterminedText = open(outdir + 'undetermined/folder.txt', 'w')
            undeterminedText.write("contains events:\ntrue\n\nmaps:\n")
            firstUndetermined = False

        undeterminedText.write("%06d\n%s\n\n" % (i,all_events['id'][i]))
        shutil.copy(outdir + 'all/' + all_events['id'][i] + '.I3R', outdir + 'undetermined/')

if not firstUndetermined:
    undeterminedText.close()
if not firstTrack:
    trackText.close()
if not firstCascade:
    cascadeText.close()
