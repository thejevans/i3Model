#!/usr/bin/env python
from __future__ import print_function
######################################################################################
#### Author: Elim Cheung
####
#### python command:
#### python i3_pickle_converter.py --nevents <num of events> --infile <input i3 files>
####                               --outfile <output path/name pickled file>
####                               (--verbose --hese)
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
import sys, datetime

from glob import glob
import numpy as np
import cPickle
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

hesedir = '/data/i3store0/users/elims/HESE/'
events={'date':[], 'id':[], 'energy':[], 'zenith':[], 'pid':[], 'hits':[]} ## hits are sorted in time

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
usage = "%prog [options] --infile <i3file> --outpath <path of out pickled file>"
parser = OptionParser(usage=usage)
parser.add_option("-n", "--nevents", type="int", default=0,
                  help="number of events to write to pickled dictionary")
parser.add_option("-v", "--verbose", action="store_true", default=False,
                  help="print info if True")
parser.add_option("-i", "--indir", type="string",
                  default='/data/i3store0/users/elims/event_display/',
                  help="directory with i3Files from real events")
parser.add_option("-a", "--infile", type="string",
                  default=None, help="i3File from real events")
parser.add_option("-o", "--outfile", type="string",
                  default='/data/i3home/elims/display3D/event_files/test.p',
                  help="pickled output path")
parser.add_option("-s", "--hese", action="store_true", default=False,
                  help="want to pickle HESE events ?")
parser.add_option("-b", "--nobestfit", action="store_true", default=False,
                  help="no loop to get bestfit if true")
(options, args) = parser.parse_args()
ishese = options.hese
infile = sorted(glob(hesedir+'*/*.i3.bz2')) if ishese else [options.infile] if options.infile else sorted(glob(options.indir+'*.i3*'))
verbose = options.verbose
outfile = options.outfile
nevents = options.nevents
nobestfit = options.nobestfit

reco_particle = 'MillipedeStarting2ndPass' if ishese else 'OnlineL2_BestFit_MuEx'
print ('number of input files: {0}'.format(len(infile)))
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
    while True:
        frame = d.pop_physics ()
        if frame:
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
##### Dump outfile
##################################################################################
print ('## Save {0} events to {1}'.format(nth, outfile))
cPickle.dump(events, open(outfile, "wb"))

print ('## Done.')
