from tkinter import *
from tkinter import font as tkfont
import os, re
from argparse import ArgumentParser
import time, board
import adafruit_dotstar as dotstar
from neopixel import *
import threading
import multiprocessing as mp
import numpy as np

#define default number of LEDs and max brightness
numpix = 4800
brightness = 255
frames = 32

#define parameters for navigating directories
pg = 0
lvl = 0

#define various flags for LED player
stop_flag = True #If True, display stop button
test_flag = True #if True, test LEDs when testing is clicked
autoplay = True #If True, autoplay next event
play_flag = False #If True, play event
pause_flag = False #If True, pause event
autoplay_flag = True #If True, display autoplay as On
button_flag = True #If True, display replay button
a_flag = True #If true, let stop function work
init_flag = True
m = 0

#Define default directory
home_dir = "/home/pi/Documents/IceCubeModel/data"
infile = "000000-126598_56835367_0.npy"
event_file = ''

parser = ArgumentParser()

parser.add_argument("-i", "--indir", help = "Home Directory for Files", type = str, default = home_dir)
parser.add_argument("-p", "--pixels", type = int, default = numpix, help = "Number of LEDs to connect to")
parser.add_argument("-B", "--brightness", type = float, default = brightness, help = "Brightness of LEDs")
parser.add_argument("-P", "--neopixel_pin", default = 18, help = "Single pin for Neopixels")
parser.add_argument("-F", "--infile", type = str, default = infile)
parser.add_argument("-f", "--frames", type = int, default = frames)

options = parser.parse_args()

#redefine parameters based on optional inputs
numpix = options.pixels
bright = options.brightness
file_dir = options.indir
neopin = options.neopixel_pin
infile = options.infile
frames = options.frames

#define LEDs
pix = Adafruit_NeoPixel(numpix, neopin, brightness = bright)
pix.begin()

#function for rotating LEDs through colors
def wheel(pos):
    if pos < 0 or pos > 255:
        return Color(0, 0, 0)
    if pos < 85:
        return Color(int(255 - pos*3), int(pos*3), 0)
    if pos< 170:
        pos -= 85
        return Color(0, int(255 - pos*3), int(pos*3))
    pos -= 170
    return Color(int(pos*3), 0, int(255 - pos*3))

def d_int(dot):
    for i in range(0, numpix):
        dot.setPixelColor(i, Color(0, 0, 0))

#natural sort functio for file/directory sorting
def natural_sort(x):
    convert = lambda text: int(text) if text.isdigit() else text.lower()
    alphanum_key = lambda key: [convert(c) for c in re.split('([0-9]+)', key)]
    return sorted(x, key = alphanum_key)

#funtion for obtaining all files/directories
def get_subdir(a_dir):
    dirs = os.listdir(a_dir)
    dirs = natural_sort(dirs)
    i = 0
    while i < len(dirs):
        if dirs[i][-4:] == '.txt':
            dirs.pop(i)
        i += 1
    return dirs

#function for obtaining just files
def get_files(a_dir):
    fs = (file for file in os.listdir(a_dir)
          if os.path.isfile(os.path.join(a_dir, file)))
    fs = natural_sort(fs)
    if fs[-4:] == '.I3R':
        return fs
    else:
        flist = []
        for f in fs:
            if f[-4:] == '.npy':
                flist.append(f)
        return flist

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
    tbin       = np.floor(np.arange(len(event['time'])) / int(len(event['time'])) * bins)
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

class Neo_GUI(Tk):
    
    def __init__(self, *args, **kwargs):
        Tk.__init__(self, *args, **kwargs)
        self.geometry("800x480")
        
        self.title_font = tkfont.Font(family = "Helvetica", size = 18, weight = "bold")
        
        self.container = Frame(self)
        self.container.pack(side = TOP, fill = BOTH, expand = True)
        self.current_frame = None
        
        if init_flag == True:
            global event_file
            global file_dir
            global lvl
            event_file = infile
            file_dir = options.indir + "/test_run_8/all"
            lvl = 2
            self.show_frame("Player")
        else:
            self.show_frame("MainMenu")
    
    def show_frame(self, page_name):
        if self.current_frame is not None:
            self.current_frame.destroy()
        
        cls = globals()[page_name]
        self.current_frame = cls(self.container, self)
        self.current_frame.pack(fill = BOTH, expand = True)
        
class MainMenu(Frame):
    
    def __init__(self, parent, controller):
        Frame.__init__(self, parent)
        self.pack_propagate(0)
        self.controller = controller
        label = Label(self, text = "This is the Main Menu", font = controller.title_font)
        label.pack(side = TOP, fill = X, pady = 10)
        
        f0 = Frame(self, width = 25, height = 180)
        f0.pack_propagate(0)
        f0.pack(side = LEFT)
        
        f1 = Frame(self, width = 360, height = 180)
        f1.pack_propagate(0)
        f1.pack(side = LEFT)
        
        file_button = Button(f1, text = "Files", bg = "yellow", command = lambda: controller.show_frame("File_Menu"))
        file_button.pack(fill = BOTH, expand = True)
        
        f2 = Frame(self, width = 25, height = 180)
        f2.pack_propagate(0)
        f2.pack(side = RIGHT)
        
        def hover1(event):
            test_button.itemconfig(testBG1, fill = "light gray", outline = "white")
            test_button.itemconfig(testBG2, fill = "light gray", outline = "white")
            test_button.itemconfig(testBG3, fill = "light gray", outline = "white")
        
        def hover2(event):
            test_button.itemconfig(testBG1, fill = "red", outline = "red")
            test_button.itemconfig(testBG2, fill = "blue", outline = "blue")
            test_button.itemconfig(testBG3, fill = "green", outline = "green")
        
        def click(event):
            global test_flag
            test_flag = True
            controller.show_frame("Test_Page")
        
        test_button = Canvas(self, width = 360, height = 180)
        test_button.pack(side = RIGHT)
        
        testBG1 = test_button.create_rectangle(0, 0, 120, 179, fill = "red", outline = "red")
        testBG2 = test_button.create_rectangle(119, 0, 240, 179, fill = "blue", outline = "blue")
        testBG3 = test_button.create_rectangle(239, 0, 359, 179, fill = "green", outline = "green")
        testTXT = test_button.create_text((180, 90), text = "Test LEDs", )
        
        test_button.tag_bind(testBG1, "<ButtonRelease-1>", click)
        test_button.tag_bind(testBG2, "<ButtonRelease-1>", click)
        test_button.tag_bind(testBG3, "<ButtonRelease-1>", click)
        test_button.tag_bind(testTXT, "<ButtonRelease-1>", click)
        test_button.tag_bind(testBG1, "<Enter>", hover1)
        test_button.tag_bind(testBG2, "<Enter>", hover1)
        test_button.tag_bind(testBG3, "<Enter>", hover1)
        test_button.tag_bind(testTXT, "<Enter>", hover1)
        test_button.tag_bind(testBG1, "<Leave>", hover2)
        test_button.tag_bind(testBG2, "<Leave>", hover2)
        test_button.tag_bind(testBG3, "<Leave>", hover2)
        test_button.tag_bind(testTXT, "<Leave>", hover2)

class Test_Page(Frame):
    
    def __init__(self, parent, controller):
        Frame.__init__(self, parent)
        self.controller = controller
                
        def stop_testing():
            global test_flag
            test_flag = False
            controller.show_frame("Test_Page")
            
        def start_testing():
            global test_flag
            test_flag = True
            controller.show_frame("Test_Page")
            
        def back_main():
            global test_flag
            test_flag = False
            controller.show_frame("MainMenu")
        
        def page():
            label = Label(self, text = "LED Testing Page", font = controller.title_font)
            label.pack(side = TOP, fill = X, pady = 10)
            
            if test_flag == False:
                text = "Do you want to test the LEDs?"
            else:
                text = "LEDs are currently testing"
                
            message = Label(self, text = text, font = controller.title_font)
            message.pack(side = TOP, fill = X, pady = 10)
            
            f0 = Frame(self, width = 300, height = 150)
            f0.pack_propagate(0)
            f0.pack(side = TOP)
            
            if test_flag == True:
                stop = Button(f0, text = "Stop Testing", bg = "red", command = stop_testing)
                stop.pack(fill = BOTH, expand = True)
            else:
                start = Button(f0, text = "Start Testing", bg = "green", command = start_testing)
                start.pack(fill = BOTH, expand = True)
        
            f1 = Frame(self, width = 300, height = 25)
            f1.pack_propagate(0)
            f1.pack(side = BOTTOM)
            
            f2 = Frame(self, width = 300, height = 150)
            f2.pack_propagate(0)
            f2.pack(side = BOTTOM)
        
            button = Button(f2, text = "Back to Menu", bg = "blue", command = back_main)
            button.pack(fill = BOTH, expand = True)
        
        def Test_LEDs():
            global pix
            j = 0
            while test_flag == True:
                j = (j + 1) % 256
                for i in range(pix.numPixels()):
                    pix.setPixelColor(i, wheel(j))
                pix.show()
                #time.sleep(0.1)
            d_int(pix)
            pix.show()
        
        if __name__ == '__main__':
            x1 = threading.Thread(target = page)
            x1.start()
            if test_flag == True:
                x2 = threading.Thread(target = Test_LEDs)
                x2.start()
            
class File_Menu(Frame):
    
    def __init__(self, parent, controller):
        Frame.__init__(self, parent)
        self.controller = controller
        
        def next_page():
            global pg
            pg += 1
            controller.show_frame("File_Menu")
            
        def back_main():
            global pg
            global file_dir
            pg = 0
            file_dir = options.indir
            controller.show_frame("MainMenu")
            
        def prev_page():
            global pg
            pg -= 1
            controller.show_frame("File_Menu")
            
        def access_dir(current_file):
            global pg
            global lvl
            global file_dir
            pg = 0
            lvl += 1
            if current_file != []:
                file_dir = file_dir + '/' + str(current_file)
            controller.show_frame("File_Menu")
            
        def back_dir():
            global pg
            global lvl
            global file_dir
            pg = 0
            lvl -= 1
            while file_dir[-1] != '/':
                file_dir = file_dir[:-1]
            file_dir = file_dir[:-1]
            controller.show_frame("File_Menu")
            
        def play_event(current_file):
            global event_file
            global play_flag
            event_file = current_file
            play_flag = True
            controller.show_frame("Player")
        
        label = Label(self, text = file_dir + " Page " + str(pg + 1), font = controller.title_font)
        label.pack(side = TOP, fill = X, pady = 10)
        
        files = get_subdir(file_dir)
        
        start = pg*15
        stop = start + 15
        
        if files == []:
            l = Label(self, text = "This Directory is Empty", font = controller.title_font)
            l.pack(side = TOP, fill = X, pady = 10)
        
        for i in range(start, len(files)):
            if i %15 == 0:
                if i != start:
                    break
            
            if i % 5 == 0:
                box = Frame(self)
                box.pack(side = TOP, fill = X)
                fs = Frame(box, width = 25, height = 75)
                fs.pack_propagate(0)
                fs.pack(side = LEFT)
            
            f = Frame(box, width = 150, height = 75)
            f.pack_propagate(0)
            f.pack(side = LEFT)
            
            current_file = files[i]
            if current_file[-4:] == '.I3R' or current_file[-4:] == '.npy':
                command = lambda x = current_file: play_event(x)
                color = "red"
            else: 
                command = lambda x = current_file: access_dir(x)
                color = "yellow"

            file = Button(f, text = str(files[i]), bg = color, command = command)
            file.pack(fill = BOTH, expand = True)
        
        boxb = Frame(self, width = 800, height = 25)
        boxb.pack_propagate(0)
        boxb.pack(side = BOTTOM)
        
        box0 = Frame(self)
        box0.pack(side = BOTTOM, fill = X)
        
        fe = Frame(box0, width = 25, height = 75)
        fe.pack_propagate(0)
        fe.pack(side = RIGHT)
        
        f0 = Frame(box0, width = 150, height = 75)
        f0.pack_propagate(0)
        f0.pack(side = RIGHT)
        
        if stop <= len(files):
            nxt = Button(f0, text = "Next Page", bg = "blue", command = next_page)
            nxt.pack(fill = BOTH, expand = True)
        
        f1 = Frame(box0, width = 150, height = 75)
        f1.pack_propagate(0)
        f1.pack(side = RIGHT)
        
        f2 = Frame(box0, width = 150, height = 75)
        f2.pack_propagate(0)
        f2.pack(side = RIGHT)
        
        if lvl == 0:
            menu = Button(f2, text = "Back to Main Menu", bg = "blue", command = back_main)
            menu.pack(fill = BOTH, expand = True)
        else:
            back = Button(f2, text = "Back to Last Directory", bg = "blue", command = back_dir)
            back.pack(fill = BOTH, expand = True)
        
        f3 = Frame(box0, width = 150, height = 75)
        f3.pack_propagate(0)
        f3.pack(side = RIGHT)
        
        f4 = Frame(box0, width = 150, height = 75)
        f4.pack_propagate(0)
        f4.pack(side = RIGHT)
        
        if start != 0:
            prev = Button(f4, text = "Previous Page", bg = "blue", command = prev_page)
            prev.pack(fill = BOTH, expand = True)
            
class Player(Frame):
    
    def __init__(self, parent, controller):
        Frame.__init__(self, parent)
        self.controller = controller
        
        var = IntVar(self, value = m)
        files = get_files(file_dir)
        index = files.index(event_file)
        
        if event_file[-4:] == '.I3R':
            with open(file_dir + '/' + event_file, 'r') as event:
                pulses = [line.rstrip('\n') for line in event]
            pulses = pulses[6:]
            LEDs = []
            R = []
            G = []
            B = []
            frame = []
            hits = []
            i = 0
            j = 0
            while True:
                while pulses[i] != 'n':
                    if len(pulses[i]) > 1 and pulses[i][-2] == '.':
                        pulses[i] = pulses[i][:-2]
                    if len(pulses[i+1]) > 1 and pulses[i+1][-2] == '.':
                        pulses[i+1] = pulses[i+1][:-2]
                    if len(pulses[i+2]) > 1 and pulses[i+2][-2] == '.':
                        pulses[i+2] = pulses[i+2][:-2]
                    if len(pulses[i+3]) > 1 and pulses[i+3][-2] == '.':
                        pulses[i+3] = pulses[i+3][:-2]
                    LEDs.append(int(pulses[i]))
                    R.append(int(pulses[i+1]))
                    G.append(int(pulses[i+2]))
                    B.append(int(pulses[i+3]))
                    i += 4
                frame = (LEDs, R, G, B)
                hits.append(frame)
                LEDs = []
                R = []
                G = []
                B = []
                frame = []
                j += 1
                if i == len(pulses) - 1:
                    break
                i += 1
        else:
            fl =  np.load(file_dir + '/' + event_file)
            pulses = binning(fl, brightness, frames)
            
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
                        
        if autoplay_flag == False:
            global autoplay
            autoplay = False
        else:
            autoplay = True
        
        if autoplay == True:
            global play_flag
            play_flag = True
        
        def prev_event(index):
            global event_file
            global stop_flag
            global autoplay_flag
            global play_flag
            global pix
            global pause_flag
            stop_flag = True
            autoplay_flag = True
            play_flag = False
            if pause_flag == True:
                d_int(pix)
                pix.show()
            pause_flag = False
            index -= 1
            event_file = files[index]
            controller.show_frame("Player")
            
        def next_event(index):
            global event_file
            global stop_flag
            global autoplay_flag
            global play_flag
            global pix
            global pause_flag
            stop_flag = True
            autoplay_flag = True
            play_flag = False
            if pause_flag == True:
                d_int(pix)
                pix.show()
            pause_flag = False
            if files[index] == files[-2]:
                index = 0
            else:
                index += 1
            event_file = files[index]
            controller.show_frame("Player")
            
        def back_menu():
            global stop_flag
            global play_flag
            global pause_flag
            global autoplay_flag
            global autoplay
            if pause_flag == True:
                d_int(pix)
                pix.show()
            stop_flag = True
            play_flag = False
            pause_flag = False
            autoplay_flag = True
            autoplay = False
            controller.show_frame("File_Menu")
        
        def pause():
            global stop_flag
            global play_flag
            global pause_flag
            global autoplay_flag
            global autoplay
            stop_flag = False
            play_flag = False
            pause_flag = True
            autoplay_flag = False
            autoplay = False
            controller.show_frame("Player")
            
        def play(a):
            global stop_flag
            global play_flag
            global autoplay_flag
            global pause_flag
            global button_flag
            global a_flag
            global m
            stop_flag = True
            play_flag = True
            autoplay_flag = True
            if a == True:
                pause_flag = False
            else:
                m = var.get()
            controller.show_frame("Player")
            button_flag = False
            #a_flag = False
            
        def stop():
            global stop_flag
            global play_flag
            global autoplay_flag
            global a_flag
            stop_flag = False
            play_flag = False
            autoplay_flag = False
            if a_flag == True:
                controller.show_frame("Player")
                a_flag = False
            
        def run_event():
            global pause_flag
            global button_flag
            global a_flag
            global pix
            global m
            global k
            if pause_flag == False:
                k = 0
                m = 0
                d_int(pix)
            else:
                d_int(pix)
                for i in range(0, m):
                    for j in range(0, len(hits[i][0])):
                        ind = hits[i][0][j] -1
                        color = Color(hits[i][2][j], hits[i][1][j], hits[i][3][j])
                        pix.setPixelColor(ind, color)
                pix.show()
                pause_flag = False
            a_flag = True
            while m < len(hits) and play_flag == True:
                for i in range(k, len(hits[m][0])):
                    if play_flag == False:
                        break
                    ind = hits[m][0][i] -1
                    color = Color(hits[m][2][i], hits[m][1][i], hits[m][3][i])
                    pix.setPixelColor(ind, color)
                    time.sleep(0.01)
                    k += 1
                pix.show()
                m += 1
                if pause_flag == False:
                    k = 0
            if play_flag == True:
                button_flag = False
                time.sleep(1)
                button_flag = True
            else:
                button_flag = True
            if pause_flag == False:
                d_int(pix)
                pix.show()
                m = 0
                time.sleep(0.1)
                if autoplay == True:
                    next_event(index)
                else:
                    a_flag = True
                    stop()
                
        def autop(a):
            global autoplay_flag
            global autoplay
            if autoplay_flag == True:
                autoplay_flag = False
                autoplay = False
                a.config(text = "Autoplay", bg = "red")
            else:
                autoplay_flag = True
                autoplay = True
                a.config(text = "Autoplay", bg = "green") 
        
        def page():
            global a_flag
            a_flag = True
            
            label = Label(self, text = "Event " + event_file, font = controller.title_font)
            label.pack(side = TOP, fill = X, pady = 10)
            
            b0 = Frame(self)
            b0.pack(side = TOP, fill = X)
            
            b1 = Frame(self)
            b1.pack(side = TOP, fill = X)
            
            f0 = Frame(b0, width = 25, height = 100)
            f0.pack_propagate(0)
            f0.pack(side = LEFT)
            
            f1 = Frame(b0, width = 250, height = 100)
            f1.pack_propagate(0)
            f1.pack(side = LEFT)
            
            if stop_flag == True:
                stp = Button(f1, text = "Stop Event", bg = "red", command = stop)
                stp.pack(fill = BOTH, expand = True)
            elif button_flag == False:
                ply = Message(f1, text = "Please Wait for Neopixels to clear", bg = "green")
                ply.pack(fill = BOTH, expand = True)
            else:
                ply = Button(f1, text = "Replay Event", bg = "green", command = lambda x = True: play(x))
                ply.pack(fill = BOTH, expand = True)
            
            f2 = Frame(b0, width = 250, height = 100)
            f2.pack_propagate(0)
            f2.pack(side = LEFT)
            
            if stop_flag == False:
                back = Button(f2, text = "Back to Menu", bg = "blue", command = back_menu)
                back.pack(fill = BOTH, expand = True)
            else:
                back = Message(f2, text = "Please Pause or Stop to return to the menu", bg = "blue")
                back.pack(fill = BOTH, expand = True)
            
            f3 = Frame(b0, width = 250, height = 100)
            f3.pack_propagate(0)
            f3.pack(side = LEFT)
            
            if pause_flag == False:
                paus = Button(f3, text = "Pause Event", bg = "yellow", command = pause)
                paus.pack(fill = BOTH, expand = True)
            else:
                paus = Button(f3, text = "Play Event", bg = "yellow", command = lambda x = False: play(x))
                paus.pack(fill = BOTH, expand = True)
            
            b1 = Frame(self, width = 800, height = 25)
            b1.pack_propagate(0)
            b1.pack(side = TOP)
            
            b2 = Frame(self)
            b2.pack(side = TOP, fill = X)
            
            f4 = Frame(b2, width = 25, height = 100)
            f4.pack_propagate(0)
            f4.pack(side = LEFT)
            
            f5 = Frame(b2, width = 250, height = 100)
            f5.pack_propagate(0)
            f5.pack(side = LEFT)
            
            if len(files) > 1 and event_file != files[0]:
                if stop_flag == False:
                    prev = Button(f5, text = "Previous Event", bg = "blue", command = lambda x = index: prev_event(x))
                    prev.pack(fill = BOTH, expand = True)
                else:
                    prev = Message(f5, text = "Please Pause or Stop to change events", bg = "blue")
                    prev.pack(fill = BOTH, expand = True)
                
            f6 = Frame(b2, width = 250, height = 100)
            f6.pack_propagate(0)
            f6.pack(side = LEFT)
            
            if autoplay_flag == True:
                auto = Button(f6, text = "Autoplay", bg = "green")
                auto.config(command = lambda x = auto: autop(x))
                auto.pack(fill = BOTH, expand = True)
            else:
                auto = Button(f6, text = "Autoplay", bg = "red")
                auto.config(command = lambda x = auto: autop(x))
                auto.pack(fill = BOTH, expand = True)
            
            f7 = Frame(b2, width = 250, height = 100)
            f7.pack_propagate(0)
            f7.pack(side = LEFT)
            
            if len(files) > 1 and event_file != files[-2]:
                if stop_flag == False:
                    nxt = Button(f7, text = "Next Event", bg = "blue", command = lambda x = index: next_event(x))
                    nxt.pack(fill = BOTH, expand = True)
                else:
                    nxt = Message(f7, text = "Please Pause or Stop to change events", bg = "blue")
                    nxt.pack(fill = BOTH, expand = True)
                
            if pause_flag == True:
                label2 = Label(self, text = "Select Frame", font = controller.title_font)
                label2.pack(side = TOP, fill = X, pady = 10)
                
                b3 = Frame(self, width = 800, height = 50)
                b3.pack_propagate(0)
                b3.pack(side = TOP)
                
                slide = Scale(b3, from_ = 1, to = len(hits), orient = HORIZONTAL, width = 40, sliderlength = 60, variable = var)
                slide.pack(fill = BOTH, expand = True)
                slide.set(m)
            
        if __name__ == '__main__':
            x1 = threading.Thread(target = page)
            x1.start()
            if play_flag == True:
                x2 = threading.Thread(target = run_event)
                x2.start()
        
if __name__ == "__main__":
    app = Neo_GUI()
    app.mainloop()