#!/usr/bin/env python
from __future__ import print_function
######################################################################################
#### Authors: John Evans & Elim Cheung
####
#### python command:
#### python i3_arduino_converter.py --nevents <num of events> --infile <input i3 files>
####                                --outdir <directory to contain I3R files of LED instructions>
####                                --bins <number of time bins in animation>
####                                (--verbose --hese)
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
import sys, datetime, os, shutil

from glob import glob
import numpy as np
import json as json

from icecube import dataclasses, dataio, icetray, millipede
from I3Tray import *

smap = { 1:  1, 2: 2 , 3: 3 , 4 :4 , 5 :5 , 6 :6 ,
         7: 13, 8: 12, 9: 11, 10:10, 11:9 , 12:8 , 13:7 ,
         14:14, 15:15, 16:16, 17:17, 18:18, 19:19, 20:20, 21:21,
         22:30, 23:29, 24:28, 25:27, 26:26, 27:25, 28:24, 29:23, 30:22,
         31:31, 32:32, 33:33, 34:34, 35:35, 36:36, 37:37, 38:38, 39:39, 40:40,
         41:50, 42:49, 43:48, 44:47, 45:46, 46:45, 47:44, 48:43, 49:42, 50:41,
         51:51, 52:52, 53:53, 54:54, 55:55, 56:56, 57:57, 58:58, 59:59,
         60:67, 61:66, 62:65, 63:64, 64:63, 65:62, 66:61, 67:60,
         68:68, 69:69, 70:70, 71:71, 72:72, 73:73, 74:74,
         75:80, 76:79, 77:78, 78:77, 79:76, 80:75 }

hesedir = '/data/i3store0/i3scratch0/users/mlarson/HESE/IC86_2011/data_IC86_2011'
events={'date':[], 'id':[], 'energy':[], 'zenith':[], 'pid':[], 'hits':[]} ## hits are sorted in time

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
##### Functions to get event info
##################################################################################
def get_hits (pulsemap):

    hits = []
    ## loop through each pulse
    for om, pulses in pulsemap.iteritems():
        string, dom = om[0], om[1]
        ## exclude deepcore strings
        if string > 78: continue
        ## only take the first pulse in each DOM
        times, total_charge = [], 0
        for pulse in pulses:
            times.append (pulse.time)
            total_charge += pulse.charge
        ## mapping between icecube and arduino string
        arduino_string = smap[string]
        thishit = [int(round(min(times))), dom, arduino_string, int(round(total_charge*100.))]
        hits.append(thishit)

    return np.array(sorted(hits))

def get_event_info (frame, header):

    ## header related
    date = str(header.start_time.utc_year)+'/'+str(header.start_time.utc_month.numerator)+'/'+str(header.start_time.utc_day_of_month)
    ID = str(header.run_id)+'_'+str(header.event_id)+'_'+str(header.sub_event_id)

    ## reco related
    particle = frame[reco_particle]
    energy = round (particle.energy / 1000., 2) ## in TeV
    zenith = round (particle.dir.zenith*180./np.pi, 2) ## in degree

    ## hits related
    hits = get_hits (frame['SplitInIcePulses'].apply(frame))

    if verbose:
        print ('date, ID      : {0}, {1}'.format(date, ID))
        print ('energy, zenith: {0} TeV, {1} degrees'.format(energy, zenith))
        print ('number of hits: {0}'.format(len(hits)))

    return date, ID, energy, zenith, hits

##################################################################################
##### Parsing variables
##################################################################################
usage = "%prog [options] --infile <i3file> --outdir <output I3R directory> --bins <num of time bins>"
parser = OptionParser(usage=usage)
parser.add_option("-n", "--nevents", type="int", default=0,
                  help="number of events to write to pickled dictionary")
parser.add_option("-v", "--verbose", action="store_true", default=False,
                  help="print info if True")
parser.add_option("-i", "--indir", type="string",
                  default=hesedir,
                  help="directory with i3Files from real events")
parser.add_option("-a", "--infile", type="string",
                  default=None, help="i3File from real events")
parser.add_option("-s", "--hese", action="store_true", default=False,
                  help="want to pickle HESE events ?")
parser.add_option("-b", "--nobestfit", action="store_true", default=False,
                  help="no loop to get bestfit if true")
parser.add_option("-o", "--outdir", type = "string",
                  default = 'events',
                  help = "directory to contain I3R files of LED instructions")
parser.add_option("-t", "--bins", type = "int", default = 32,
                  help = "number of time bins in animation")
(options, args) = parser.parse_args()

ishese    = options.hese
infile    = sorted(glob(hesedir+'*/*.i3.bz2')) if ishese else [options.infile] if options.infile else sorted(glob(options.indir+'*.i3*'))
verbose   = options.verbose
nevents   = options.nevents
nobestfit = options.nobestfit
outdir    = options.outdir
bins      = options.bins

outdir = infile[0:infile.find('.')] if outdir == "events" else outdir
outdir = outdir if outdir.startswith(('/','.','~')) else "./" + outdir
outdir = outdir if outdir.endswith('/') else outdir + "/"

## make outdir
if not os.path.exists(os.path.dirname(outdir)):
    try:
        os.makedirs(os.path.dirname(outdir))
    except OSError as exc: # Guard against race condition
        if exc.errno != errno.EEXIST:
            raise

print (' ')
print ('in_file: {0}'.format(infile))
print ('out_dir: {0}'.format(outdir))
print (' ')

reco_particle = 'MillipedeStarting2ndPass' if ishese else 'OnlineL2_BestFit_MuEx'
print ('number of input files: {0}'.format(len(infile)))

##################################################################################
##### Define basic variables
##################################################################################

max_brightness = 100. ## set max brightness to avoid burning the LEDs

## flags for initial operations for PID folders
firstTrack = True
firstCascade = True
firstUndetermined = True

## counters for print at the end
j = 0
k = 0
l = 0
m = 0

##################################################################################
##### Loop through events to pull event info
##################################################################################
nth = 0
runs = []

for f in infile:
    if verbose: print ('{0}'.format(f))

    #### for hese events, 1 f = 1 data event;
    #### need to find the best fit event from each f
    minllh = np.inf

    #### open f and loop
    d = dataio.I3File(f)
    ith = 0 ## for tracking HESE events
    while d.more():
        frame = d.pop_frame()
        if frame:
            if not (frame.Stop == icetray.I3Frame.Physics): continue
            #### if specify nevents, check nth
            if nevents>0 and nth==nevents: break

            #### if ishese, only 1 event from each run
            header = frame['I3EventHeader']
            if ishese and header.run_id in runs: break

            #### if ishese, best fit = first event if
            #### first P frame has 'best_fit' in sub event stream
            if ishese:
                if ith==0:
                    if 'best_fit' in header.sub_event_stream or nobestfit:
                        date, ID, energy, zenith, hits = get_event_info (frame, header)
                        pid = 1 if 'track' in f else 0
                        events['date'].append (date)
                        events['id'].append (ID)
                        events['energy'].append (energy)
                        events['zenith'].append (zenith)
                        events['pid'].append (pid)
                        events['hits'].append (hits)
                        runs.append (header.run_id)
                        nth += 1
                        break
                llh = frame['MillipedeStarting2ndPassFitParams'].logl
                ith += 1
                if llh < minllh:
                    minllh = llh
                    date, ID, energy, zenith, hits = get_event_info (frame, header)
                continue
            else:
                if not frame.Has (reco_particle): continue
                date, ID, energy, zenith, hits = get_event_info (frame, header)
                events['date'].append (date)
                events['id'].append (ID)
                events['energy'].append (energy)
                events['zenith'].append (zenith)
                events['pid'].append (-1)
                events['hits'].append (hits)
                runs.append (header.run_id)
                nth += 1
        else:
            if ishese:
                pid = 1 if 'track' in f else 0
                if verbose:
                    print ('date, ID           : {0}, {1}'.format(date, ID))
                    print ('energy, zenith, pid: {0} TeV, {1} degrees, {2}'.format(energy, zenith, pid))
                    print ('number of hits     : {0}'.format(len(hits)))

                events['date'].append (date)
                events['id'].append (ID)
                events['energy'].append (energy)
                events['zenith'].append (zenith)
                events['pid'].append (pid)
                events['hits'].append (hits)
                runs.append (header.run_id)
                nth += 1
            break
    print ('{0} collected {1} events ...'.format(f, nth))
    d.close ()

##################################################################################
##### Send events to I3R files and create folder.txt files
##################################################################################

## make main folder.txt
text = open(outdir + 'folder.txt', 'w')

text.write("contains events:\nfalse\n\nmaps:\nALL\nAll\n\n")

## make all subdirectory
if not os.path.exists(os.path.dirname(outdir + 'all/')):
    try:
        os.makedirs(os.path.dirname(outdir + 'all/'))
    except OSError as exc: # Guard against race condition
        if exc.errno != errno.EEXIST:
            raise

## start all folder.txt
allText = open(outdir + 'all/folder.txt', 'w')
allText.write("contains events:\ntrue\n\nmaps:\n")

for i, event in enumerate(events['hits']):

    ## make I3R file
    output = open(outdir + 'all/' + "%06d" % i + '-' + events['id'][i] + '.I3R', 'w')

    output.write("q\n%s\n%s\n%s\n%s\n%s\n" % (events['date'][i], events['id'][i], events['energy'][i], events['zenith'][i], events['pid'][i]))

    for item in eventToArray(event, max_brightness, bins):
        output.write("%s\n" % item)

    output.close()

    j += 1

    ## add to all folder.txt
    allText.write("%06d\n%s\n\n" % (i,events['id'][i]))

    ## if track, copy I3R file and add to tracks folder.txt
    if events['pid'][i] == 1:
        if firstTrack:
            text.write("TRACKS\nTracks\n\n")
            if not os.path.exists(os.path.dirname(outdir + 'tracks/')):
                try:
                    os.makedirs(os.path.dirname(outdir + 'tracks/'))
                except OSError as exc: # Guard against race condition
                    if exc.errno != errno.EEXIST:
                        raise

            trackText = open(outdir + 'tracks/folder.txt', 'w')
            trackText.write("contains events:\ntrue\n\nmaps:\n")
            firstTrack = False

        trackText.write("%06d\n%s\n\n" % (i,events['id'][i]))
        shutil.copy(outdir + 'all/' + "%06d" % i + '-' + events['id'][i] + '.I3R', outdir + 'tracks/')
        k += 1

    ## if cascade, copy I3R file and add to cascades folder.txt
    elif events['pid'][i] == 0:
        if firstCascade:
            text.write("CASCAD\nCascades\n\n")
            if not os.path.exists(os.path.dirname(outdir + 'cascades/')):
                try:
                    os.makedirs(os.path.dirname(outdir + 'cascades/'))
                except OSError as exc: # Guard against race condition
                    if exc.errno != errno.EEXIST:
                        raise

            cascadeText = open(outdir + 'cascades/folder.txt', 'w')
            cascadeText.write("contains events:\ntrue\n\nmaps:\n")
            firstCascade = False

        cascadeText.write("%06d\n%s\n\n" % (i,events['id'][i]))
        shutil.copy(outdir + 'all/' + "%06d" % i + '-' + events['id'][i] + '.I3R', outdir + 'cascades/')
        l += 1

    ## if undetermined, copy I3R file and add to undetermined folder.txt
    else:
        if firstUndetermined:
            text.write("UNDETE\nUndetermined\n\n")
            if not os.path.exists(os.path.dirname(outdir + 'undetermined/')):
                try:
                    os.makedirs(os.path.dirname(outdir + 'undetermined/'))
                except OSError as exc: # Guard against race condition
                    if exc.errno != errno.EEXIST:
                        raise

            undeterminedText = open(outdir + 'undetermined/folder.txt', 'w')
            undeterminedText.write("contains events:\ntrue\n\nmaps:\n")
            firstUndetermined = False

        undeterminedText.write("%06d\n%s\n\n" % (i,events['id'][i]))
        shutil.copy(outdir + 'all/' + "%06d" % i + '-' + events['id'][i] + '.I3R', outdir + 'undetermined/')
        m += 1

## close opened folder.txt files
if not firstUndetermined:
    undeterminedText.close()
if not firstTrack:
    trackText.close()
if not firstCascade:
    cascadeText.close()

text.close()

## print totals
print ('Total Events: {0}'.format(j))
print ('Tracks: {0}'.format(k))
print ('Cascades: {0}'.format(l))
print ('Undetermined: {0}'.format(m))
print (' ')
