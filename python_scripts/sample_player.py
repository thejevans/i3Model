import os, re, time
from argparse import ArgumentParser
import numpy as np

indir = '/data/condor_builds/users/bbrinson/I3R/test_run_8/all'

parser = ArgumentParser()

parser.add_argument("-i", "--indir", type = str, default = indir)
parser.add_argument("-f", "--frames", type = int, default = 32, help = "number of frames for event")
parser.add_argument("-br", "--brightness", type = int, default = 150, help = "LED brightness from 0 to 255")

options = parser.parse_args()

indir = options.indir
bins = options.frames 
brightness = options.brightness

#natural sort functio for file/directory sorting
def natural_sort(x):
    convert = lambda text: int(text) if text.isdigit() else text.lower()
    alphanum_key = lambda key: [convert(c) for c in re.split('([0-9]+)', key)]
    return sorted(x, key = alphanum_key)

#funtion for obtaining all files/directories
def get_subdir(a_dir):
    dirs = os.listdir(a_dir)
    dirs = natural_sort(dirs)
    return dirs

#function for obtaining just files
def get_files(a_dir):
    fs = (file for file in os.listdir(a_dir)
          if os.path.isfile(os.path.join(a_dir, file)))
    fs = natural_sort(fs)
    return fs

files = get_files(indir)
flist = [] 

for f in files:
    if f[-4:] == '.npy':
        flist.append(f)

def flatten (dom, string):
    if string % 2 == 1:
        led = 60 * string - dom
    else:
        led = 60 * (string - 1) + (dom - 1);

    return led

def binning(event, bright, bins):
    if bins > int(len(event['time'])):
        bins = int(len(event['time']))

    max_charge = np.sqrt(max(event['charge']))
    brightness = (np.sqrt(event['charge']) * bright / max_charge).astype(int)
    tbin       = np.arange(len(event['time'])) * bins / int(len(event['time']))
    wavelength = (700. - 300. * tbin / bins).astype(int)

    eventArray = [] 
    frame = 0

    for i, pulse in enumerate(event):
        [r, g, b] = wav2RGB(wavelength[i])

        r  *= brightness[i]
        g  *= brightness[i]
        b  *= brightness[i]
        led = flatten(pulse[1], pulse[2])
        
        eventArray.append([led, int(r), int(g), int(b), int(frame)])

        if (i==len(event)-1) or (not wavelength[i]==wavelength[i+1]):
            frame += 1        

    return eventArray

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

fl =  np.load(indir + '/' + flist[0])
pulses = binning(fl, brightness, bins)

LEDs = []
R = []
G = []
B = []
frame = []
hits = []
for i in range(0, len(pulses)):
    LEDs.append(int(pulses[i][0]))
    R.append(int(pulses[i][1]))
    G.append(int(pulses[i][2]))
    B.append(int(pulses[i][3]))
    if i != len(pulses) - 1:
        if pulses[i][4] != pulses[i+1][4]:
            frame = (LEDs, R, G, B)
            hits.append(frame)
            LEDs = []
            R = []
            G = []
            B = []
            frame = []
    else:
        frame = (LEDs, R, G, B)
        hits.append(frame)
        LEDs = []
        R = []
        G = []
        B = []
        frame = []
