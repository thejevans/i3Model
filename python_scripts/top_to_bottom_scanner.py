#!/usr/bin/env python
from __future__ import print_function
######################################################################################
#### Author: Elim Cheung
####
#### This is a test script to make sure the mapping between arduino LED location and
#### IceCube DOM / string number.
####
#### python command:
#### python top_to_bottom_scanner.py --outfile <output path/name pickled file>
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

import numpy as np
import cPickle

### True = data from bottom; False = data from top
smap = { 1: [ 1,True] , 2: [2 ,False], 3: [3 ,True] , 4 :[4 ,False], 5 :[5 ,True] , 6 :[6 ,False],
         7: [13,True] , 8: [12,False], 9: [11,True] , 10:[10,False], 11:[9 ,True] , 12:[8 ,False], 13:[7 ,True] ,
         14:[14,False], 15:[15,True] , 16:[16,False], 17:[17,True] , 18:[18,False], 19:[19,True] , 20:[20,False], 21:[21,True] ,
         22:[30,False], 23:[29,True] , 24:[28,False], 25:[27,True] , 26:[26,False], 27:[25,True] , 28:[24,False], 29:[23,True] , 30:[22,False] ,
         31:[31,True] , 32:[32,False], 33:[33,True] , 34:[34,False], 35:[35,True] , 36:[36,False], 37:[37,True] , 38:[38,False], 39:[39,True] , 40:[40,False],
         41:[50,False], 42:[49,True] , 43:[48,False], 44:[47,True] , 45:[46,False], 46:[45,True] , 47:[44,False], 48:[43,True] , 49:[42,False], 50:[41,True] ,
         51:[51,True] , 52:[52,False], 53:[53,True] , 54:[54,False], 55:[55,True] , 56:[56,False], 57:[57,True] , 58:[58,False], 59:[59,True],
         60:[67,True] , 61:[66,False], 62:[65,True] , 63:[64,False], 64:[63,True] , 65:[62,False], 66:[61,True] , 67:[60,False],
         68:[68,False], 69:[69,True] , 70:[70,False], 71:[71,True] , 72:[72,False], 73:[73,True] , 74:[74,False],
         75:[80,False], 76:[79,True] , 77:[78,False], 78:[77,True] , 79:[76,False], 80:[75,True] }

##################################################################################
##### Parsing variables
##################################################################################
usage = "%prog [options] --outpath <path of out pickled file>"
parser = OptionParser(usage=usage)
parser.add_option("-o", "--outfile", type = "string", default = '/data/i3home/elims/display3D/event_files/test.p',
                  help = "pickled output path")
(options, args) = parser.parse_args()
outfile = options.outfile

##################################################################################
##### Pull pulse info
##################################################################################
hits=[]

## loop through each DOM number
for om in np.arange (60)+1:
    for string in np.arange (80)+1:
        time, charge = int(om), min(2, int(om+string+10))
        arduino_string = smap[string][0]
        thishit = [time, om, arduino_string, charge]
        print ('this hit: {0}'.format(thishit))
        hits.append(thishit)

## sort hits by string number
sorted_hits = np.array (hits)
#sorted_hits = np.array (hits[hits[:,2].argsort()])
#sorted_hits = np.array(sorted(hits))
print ('##       ... it has {0} hits'.format(len(sorted_hits)))
#print ('##       ... it takes {0} nano seconds'.format(sorted_hits[-1][0] - sorted_hits[0][0]))
print ('## ')
all_events = [sorted_hits]

print ('all_events: {0}'.format(all_events))

print ('## Save all_events to {0}'.format(outfile))
##### writing out pickled file
cPickle.dump(all_events, open(outfile, "wb"))

print ('## Done.')
