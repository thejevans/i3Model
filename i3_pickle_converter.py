#!/usr/bin/env python
from __future__ import print_function
######################################################################################
#### python command:
#### python i3_pickle_converter.py --nevents <num of events> --infile <input i3 files> 
####                               --outfile <output path/name pickled file>
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

from icecube import dataclasses, dataio, icetray
from I3Tray import *

##################################################################################
##### Parsing variables
##################################################################################
usage = "%prog [options] --infile <i3file> --outpath <path of out pickled file>"
parser = OptionParser(usage=usage)
parser.add_option("-n", "--nevents", type="int", default = 0,
                  help = "number of events to write to pickled dictionary")
parser.add_option("-i", "--infile", type = "string", 
                  default = '/data/i3store0/users/elims/event_display/Viewer_Run00127637_Subrun00000245.i3',
                  help = "i3File from real events")
parser.add_option("-o", "--outfile", type = "string", default = '/data/i3home/elims/display3D/event_files/test.p',
                  help = "pickled output path")
(options, args) = parser.parse_args()
infile = options.infile
outfile = options.outfile
nevents = options.nevents

i=0; all_events=[]
##################################################################################
##### Start I3Tray
##################################################################################
tray = I3Tray ()

### see how many input files
if len(infile) > 1:
    tray.AddModule ('I3Reader', 'reader', FilenameList = sorted(glob(infile)) )
elif len(infile) == 1:
     tray.AddModule ('I3Reader', 'reader', Filename = infile )

##################################################################################
##### Keep track of number of events
##################################################################################
def count(frame):
    global i
    print ('#########################################################')
    print ('##  working on event {0}th event ...'.format(i))
    i += 1
    return

tray.AddModule(count, 'count')

##################################################################################
##### Pull pulse info
##################################################################################
def pull_pulses (frame):
    """Get all SplitInIcePulses out of the frame."""
    try:
        hits=[]
        splitinicepulses = frame['SplitInIcePulses'].apply(frame)
        ## loop through each pulse
        for om, pulses in splitinicepulses.iteritems():
            string, dom = om[0], om[1]
            ## exclude deepcore strings
            if string > 78: continue
            ## only take the first pulse in each DOM
            for pulse in pulses:
                time, charge = int(round(pulse.time)), int(round(pulse.charge*100))
                continue  
            thishit = [time, dom, string, charge]
            hits.append(thishit)

        ## sort hits by time
        sorted_hits = np.array(sorted(hits))
        print ('##       ... it has {0} hits'.format(len(sorted_hits)))
        print ('##       ... it takes {0} nano seconds'.format(sorted_hits[-1][0] - sorted_hits[0][0]))
        print ('## ')
        all_events.append(sorted_hits)
        
    except:
        print ('##                             try failed :(')
        print ('## ')
        return

tray.AddModule (pull_pulses, 'pullpulses')

##################################################################################
##### Finish I3Tray and store events
##################################################################################
tray.AddModule ('TrashCan', 'tc')
if nevents==0:
    tray.Execute()
else:
    tray.Execute(nevents)

print ('## Save all_events to {0}'.format(outfile))
##### writing out pickled file
cPickle.dump(all_events, open(outfile, "wb"))

print ('## Done.')
