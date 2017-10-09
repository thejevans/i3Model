#!/usr/bin/env python
from __future__ import print_function
######################################################################################
#### Authors: John Evans & Elim Cheung
####
#### python command:
#### python i3_arduino_converter.py --nevents <num of events> --infile <input i3 files>
####                                --outdir <directory to contain I3R files of LED instructions>
####                                --bins <number of time bins in animation>
####                                (--verbose --hese --recent --kloppo --diffuse_numu --charge_based)
####
#### Notes:
####   1. pid = 0 (cascade); = 1 (track); = -1 (undetermined)
####
####   2. if you are adding new events from new i3files, 
####      get_fobject_name (filename) is the function you
####      may need to change. 
######################################################################################

###################### PRINTOUTS ########################
import socket
print ('########################################################################################')
print ('#### This job is running on {0}'.format(socket.gethostname()))
print ('########################################################################################')
print ('#### ')
#########################################################
from optparse import OptionParser
from os.path import expandvars
import sys, datetime, os, shutil

from glob import glob
import numpy as np

from icecube import dataclasses, dataio, icetray, millipede
from I3Tray import *

########################
##### global variables
########################
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

maindir = '/data/i3store0/users/elims/display3D/elimstuff/i3files/'
events  = {'date':[], 'id':[], 'energy':[], 'zenith':[], 'pid':[], 'hits':[]} ## hits are sorted in time

## Kloppo info
kloppo = { 'zenith': 101.     , ## in degree
           'energy': 4.45*1000, ## in TeV
           'pid'   : 1        }

## set max brightness to avoid burning the LEDs
max_brightness = 100. 

## counters for print at the end
j, k, l, m = 0, 0, 0, 0

####################################
##### Misc. Functions
####################################
def check_path (path):

    ''' check if path exists; if not, make path '''

    if not os.path.exists (os.path.dirname (path)):
        try:
            os.makedirs (os.path.dirname (path))
        except OSError as exc: # Guard against race condition
            if exc.errno != errno.EEXIST:
                raise

####################################
##### Functions to get event info
####################################
def append_event (date, ID, energy, zenith, pid, hits):

    ''' append this event info to event array '''

    events['date'].append (date)
    events['id'].append (ID)
    events['energy'].append (energy)
    events['zenith'].append (zenith)
    events['pid'].append (pid)
    events['hits'].append (hits)
    return

def print_event (date, ID, energy, zenith, pid, hits):

    ''' print out the event info; called when verbose '''

    pid = 'track' if pid==1 else 'cascade' if pid==0 else 'undetermined'
    energy = 'undertermined' if energy==-1 else energy
    zenith = 'undertermined' if zenith==-1 else zenith
    print ('#### date, ID           : {0}, {1}'.format(date, ID))
    print ('#### energy, zenith, pid: {0} TeV, {1} degrees'.format(energy, zenith, pid))
    print ('#### number of hits     : {0}'.format(len(hits)))
    return

def get_fobject_name (filename):

    ''' return the names of the frame objects for reconstructed
        variables and pulse map;
    
        these frame objects may be different for different
        analyses and for different years of simulations;

        thus, you may need to be modify this part of the code. '''

    if iskloppo:
        return 'OnlineL2_BestFit_MuEx', 'TWSRTHVInIcePulses'
    if isrecent:
        return 'OnlineL2_BestFit_MuEx', 'SplitInIcePulses'    
    if ishese:
        reco_particle = 'OnlineL2_PoleL2MPEFit_TruncatedEnergy_DOMS_Muon' if 'IC86-2' in filename else \
                        'MPEFitMuEX' if 'IC86-4' in filename else \
                        'OnlineL2_BestFit_TruncatedEnergy_DOMS_Muon'
        return reco_particle, 'SplitInIcePulses'
    if isdiffusenumu:
        reco_particle = 'MPEFitMuE' if 'IC59' in filename or 'IC79' in filename else \
                        'MPEFitTruncatedEnergy_SPICEMie_DOMS_Muon' if 'IC86-1' in filename else \
                        'OnlineL2_PoleL2MPEFit_TruncatedEnergy_DOMS_Muon' if 'IC86-2' in filename else \
                        'OnlineL2_BestFit_TruncatedEnergy_DOMS_Muon'
        pulseseries = 'TWOfflinePulseSeriesReco' if 'IC59' in filename else \
                      'SRTOfflinePulses' if 'IC79' in filename or 'IC86-1' in filename else \
                      'SplitInIcePulses'
        return reco_particle, pulseseries
    return 'OnlineL2_BestFit_MuEx', 'SplitInIcePulses'

def get_hits (pulsemap):

    ''' collect all hits '''

    hits = []
    ## loop through each pulse
    for om, pulses in pulsemap.iteritems():
        string, dom = om[0], om[1]
        ## exclude deepcore strings
        if string > 78: continue
        ## mapping between icecube and arduino string
        arduino_string = smap[string]
        ## earliest time
        time = int (min ([ p.time for p in pulses ]))
        ## summed charge of each DOM
        charge = int (np.sum ([ p.charge for p in pulses ])*100.)
        hits.append([time, dom, arduino_string, charge])
    return np.array(sorted(hits))

def get_event_info (frame, reco_particle, pulseseries):

    ''' key function to grab information of a given event '''

    ## header related
    header = frame['I3EventHeader']
    date = str(header.start_time.utc_year)+'/'+str(header.start_time.utc_month.numerator)+'/'+str(header.start_time.utc_day_of_month)
    ID = str(header.run_id)+'_'+str(header.event_id)+'_'+str(header.sub_event_id)

    ## reco related
    if iskloppo:
        energy, zenith, pid = kloppo['energy'], kloppo['zenith'], kloppo['pid']
    else:
        particle = frame[reco_particle]
        energy = round (particle.energy / 1000., 2) ## in TeV
        zenith = round (particle.dir.zenith*180./np.pi, 2) ## in degree
        pid = 1 if isrecent or isdiffusenumu or (ishese and 'best_fit' in header.sub_event_stream) else \
              0 if ishese else -1

    ## hits related
    pulsemap = frame[pulseseries] if type(frame[pulseseries])==dataclasses.I3RecoPulseSeriesMap else \
               frame[pulseseries].apply(frame)
    hits = get_hits (pulsemap)

    if verbose: print_event (date, ID, energy, zenith, pid, hits)
    return date, ID, energy, zenith, pid, hits

####################################
##### Functions to conver to I3R
####################################
def flatten (dom, string):

    ''' Converts from DOM/String format to LED value '''

    if string % 2 == 1:
        led = 60 * string - dom
    else:
        led = 60 * (string - 1) + (dom - 1);

    return led

def eventToArray (event, maxBrightness, totalBins):

    ''' Converts event to Array '''

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

def wav2RGB(wavelength):

    ''' Converts from wavelength to RGB (0.0 to 1.0 scale) '''

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
usage = "%prog [options] --infile <i3file> --outdir <output I3R directory> --bins <num of time bins>"
parser = OptionParser(usage=usage)
#### I/O paths
parser.add_option("-i", "--indir", type="string",
                  default='/data/i3store0/users/elims/event_display/',
                  help="directory with i3Files from real events")
parser.add_option("-o", "--outdir", type="string",
                  default='/data/i3home/elims/display3D/event_files/',
                  help="output directory")
#### type of analyses
parser.add_option("-s", "--hese", action="store_true", default=False,
                  help="want to pickle HESE events ?")
parser.add_option("-r", "--recent", action="store_true", default=False,
                  help="want to pickle recent events from John's folder ?")
parser.add_option("-d", "--diffuse_numu", action="store_true", default=False,
                  help="want to pickle diffuse numu events ?")
parser.add_option("-k", "--kloppo", action="store_true", default=False,
                  help="want to pickle the Kloppo event ?")
parser.add_option("-a", "--infile", type="string",
                  default=None, help="i3File from real events")
#### misc
parser.add_option("-n", "--nevents", type="int", default=-1,
                  help="number of events to write to pickled dictionary")
parser.add_option("-v", "--verbose", action="store_true", default=False,
                  help="print info if True")
parser.add_option("-g", "--charge_based", action="store_true", default=False,
                  help = "if true, red = min charges and blue = low charges; otherwise, red = first, blue = last")
(options, args) = parser.parse_args()

ishese = options.hese
isrecent = options.recent
iskloppo = options.kloppo
isdiffusenumu = options.diffuse_numu
infiles = sorted(glob(maindir+'HESE/*/*.i3.bz2')) if ishese else \
          sorted(glob('/data/i3store0/users/jfelde/recent_i3_files/*.i3')) if isrecent else \
          sorted(glob(maindir+'kloppo/*.i3.bz2')) if iskloppo else \
          sorted(glob(maindir+'diffuse_numu/*.i3.bz2')) if isdiffusenumu else \
          [options.infile] if options.infile else sorted(glob(options.indir+'*.i3*'))
verbose   = options.verbose
nevents   = options.nevents
outdir    = options.outdir
bins      = options.bins
charge_based = options.charge_based

print ('#### number of input files: {0}'.format(len(infiles)))
print ('#### writing to {0}'.format(outdir))
print ('#### ')
check_path (outdir)

#####################################
##### Let's start from here !
#####################################

######################################
## Step 1: Loop through events from i3 files and pull event info
print ('#### STEP 1: Loop through events from i3 files')
nth = 0
## flipped events such that track first
for f in infiles[::-1]:

    if verbose: print ('#### {0}'.format(f))
    #### only pick the first event of each file if HESE
    ith = 0
    #### get reco particle and pulse series based on IceCube season
    reco_particle, pulseseries = get_fobject_name (f)
    #### open file and loop
    d = dataio.I3File(f)
    #### check nth; end loop if collected enough events
    if nth==nevents: break

    while True:
        frame = d.pop_physics ()
        if frame:
            #### if it doesn't have the reconstructed particle, next!
            if not iskloppo and not frame.Has (reco_particle): continue
            #### check nth; end loop if collected enough events
            if nth==nevents: break
            #### if ishese, only first event
            if ishese and ith>0: break
            #### get the event info and append
            date, ID, energy, zenith, pid, hits = get_event_info (frame, reco_particle, pulseseries)
            append_event (date, ID, energy, zenith, pid, hits)
            if verbose: print_event (date, ID, energy, zenith, pid, hits)
            nth += 1; ith += 1
        else:
            break
    print ('#### {0} collected {1} events ...'.format(f, nth))
    d.close ()
print ('####')

###########################################
## Step 2: convert events to I3R files
print ('#### STEP 2: convert events to I3R files')

## make main folder.txt
text = open(outdir + 'folder.txt', 'w')
text.write("contains events:\nfalse\n\nmaps:\nALL\nAll\n\n")

## create folders
check_path (outdir + 'all/')
Texts = {}
for p in np.unique (events['pid']):
    ptype = 'cascades' if p==0 else 'tracks' if p==1 else 'undetermined'
    check_path (outdir + ptype + '/')
    line = "UNDETE\nUndetermined\n\n" if ptype=='undetermined' else \
           ptype.upper () + '\n'+ptype.title ()+'\n\n'
    text.write (line)
    Texts[ptype] = open (outdir + ptype + '/folder.txt', 'w')
    Texts[ptype].write("contains events:\ntrue\n\nmaps:\n")

## start all/folder.txt
Texts['all'] = open (outdir + 'all/folder.txt', 'w')
Texts['all'].write ("contains events:\ntrue\n\nmaps:\n")

## start looping each event (from track first)
for i, event in enumerate (events['hits']):

    ## make a I3R file per event
    output = open (outdir + 'all/' + "%06d" % i + '-' + events['id'][i] + '.I3R', 'w')
    output.write ("q\n%s\n%s\n%s\n%s\n%s\n" % (events['date'][i], events['id'][i], events['energy'][i], events['zenith'][i], events['pid'][i]))
    for item in eventToArray (event, max_brightness, bins):
        output.write ("%s\n" % item)
    output.close ()
    j += 1

    ## add to all folder.txt
    Texts['all'].write ("%06d\n%s\n\n" % (i,events['id'][i]))

    ## copy I3R file to corresponding folder based on pid
    ptype = 'cascades' if events['pid'][i]==0 else 'tracks' if events['pid'][i]==1 else 'undetermined'
    Texts[ptype].write ("%06d\n%s\n\n" % (i,events['id'][i]))
    shutil.copy(outdir + 'all/' + "%06d" % i + '-' + events['id'][i] + '.I3R', outdir + ptype + '/')
    if 'track' in ptype:
        k += 1
    elif 'cascade' in ptype:
        l += 1
    else:
        m += 1

## close all opened folder.txt files
for ptype in Texts.keys ():
    Texts[ptype].close ()
text.close()
print ('####')

## print totals
print ('#### In summary, you collected {0} total events ({1} cascades; {2} tracks; {3} undetermined)!'.format (j, l, k, m))
print ('#### DONE !')
