from tkinter import *
from tkinter import font as tkfont
import os, re
from argparse import ArgumentParser
import time, board
import adafruit_dotstar as dotstar
import threading

#define default number of LEDs and max brightness
numpix = 72
brightness = 0.2

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
m = 0

#Define default directory
home_dir = "/home/pi/Documents/IceCubeModel/test_run"
event_file = ''

parser = ArgumentParser()

parser.add_argument("-i", "--indir", help = "Home Directory for Files", type = str, default = home_dir)
parser.add_argument("-p", "--pixels", type = int, default = numpix, help = "Number of LEDs to connect to")
parser.add_argument("-B", "--brightness", type = float, default = brightness, help = "Brightness of LEDs")
parser.add_argument("-D1", "--dotstar_pin_1", default = board.D6, help = "First Dotstar pin")
parser.add_argument("-D2", "--dotstar_pin_2", default = board.D5, help = "Second Dotstar pin")

options = parser.parse_args()

#redefine parameters based on optional inputs
numpix = options.pixels
brightness = options.brightness
file_dir = options.indir
dotpin1 = options.dotstar_pin_1
dotpin2 = options.dotstar_pin_2

#define LEDs
dots = dotstar.DotStar(dotpin1, dotpin2, numpix, brightness = brightness, auto_write = False)

#function for rotating LEDs through colors
def wheel(pos):
    if pos < 0 or pos > 255:
        return 0, 0, 0
    if pos < 85:
        return int(255 - pos*3), int(pos*3), 0
    if pos< 170:
        pos -= 85
        return 0, int(255 - pos*3), int(pos*3)
    pos -= 170
    return int(pos*3), 0, int(255 - pos*3)

def d_int(dot):
    for i in range(0, numpix):
        dot[i] = (0, 0, 0)

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

class Dot_GUI(Tk):
    
    def __init__(self, *args, **kwargs):
        Tk.__init__(self, *args, **kwargs)
        self.geometry("800x480")
        
        self.title_font = tkfont.Font(family = "Helvetica", size = 18, weight = "bold")
        
        self.container = Frame(self)
        self.container.pack(side = TOP, fill = BOTH, expand = True)
        self.current_frame = None
        
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
            global dots
            i = 0
            while test_flag == True:
                i = (i + 1) % 256
                dots.fill(wheel(i))
                dots.show()
            d_int(dots)
            dots.show()
        
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
            if current_file[-4:] == '.I3R':
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
        
        files = get_files(file_dir)
        index = files.index(event_file)
        
        with open(file_dir + '/' + event_file, 'r') as event:
            pulses = [line.rstrip('\n') for line in event]
        pulses = pulses[6:]
        LEDs = []
        R = []
        G = []
        B = []
        hits = []
        for i in range(0, len(pulses)):
            if len(pulses[i]) > 1 and pulses[i][-2] == '.':
                pulses[i] = pulses[i][:-2]
            if i % 5 == 0:
                LEDs.append(int(pulses[i]))
            elif i % 5 == 1:
                R.append(int(pulses[i]))
            elif i % 5 == 2:
                G.append(int(pulses[i]))
            elif i % 5 == 3:
                B.append(int(pulses[i]))
        for n in range(0, len(LEDs)):
            hits.append([LEDs[n], R[n], G[n], B[n]])
                        
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
            global dots
            global pause_flag
            stop_flag = True
            autoplay_flag = True
            play_flag = False
            if pause_flag == True:
                d_int(dots)
                dots.show()
            pause_flag = False
            index -= 1
            event_file = files[index]
            controller.show_frame("Player")
            
        def next_event(index):
            global event_file
            global stop_flag
            global autoplay_flag
            global play_flag
            global dots
            global pause_flag
            stop_flag = True
            autoplay_flag = True
            play_flag = False
            if pause_flag == True:
                d_int(dots)
                dots.show()
            pause_flag = False
            if files[index] == files[-1]:
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
                d_int(dots)
                dots.show()
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
            stop_flag = True
            play_flag = True
            autoplay_flag = True
            if a == True:
                pause_flag = False
            controller.show_frame("Player")
            
        def stop():
            global stop_flag
            global play_flag
            global autoplay_flag
            stop_flag = False
            play_flag = False
            autoplay_flag = False
            controller.show_frame("Player")
            
        def run_event():
            global pause_flag
            global dots
            global m
            k = 0
            if pause_flag == False:
                n = 0
                d_int(dots)
            else:
                pause_flag = False
                n = m
            while k == 0 and play_flag == True:
                for i in range(n, len(LEDs)):
                    if play_flag == False:
                        break
                    ind = LEDs[i] -1
                    color = (R[i], G[i], B[i])
                    dots[ind] = color
                    dots.show()
                    m += 1
                    time.sleep(0.1)
                if play_flag == True:
                    time.sleep(1)
                k += 1
            if pause_flag == False:
                d_int(dots)
                dots.show()
                m = 0
                if autoplay == True:
                    next_event(index)
                
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
            label = Label(self, text = "Event " + event_file, font = controller.title_font)
            label.pack(side = TOP, fill = X, pady = 10)
            
            b0 = Frame(self)
            b0.pack(side = TOP, fill = X)
            
            b1 = Frame(self)
            b1.pack(side = TOP, fill = X)
            
            f0 = Frame(b0, width = 25, height = 125)
            f0.pack_propagate(0)
            f0.pack(side = LEFT)
            
            f1 = Frame(b0, width = 250, height = 125)
            f1.pack_propagate(0)
            f1.pack(side = LEFT)
            
            if stop_flag == True:
                stp = Button(f1, text = "Stop Event", bg = "red", command = stop)
                stp.pack(fill = BOTH, expand = True)
            else:
                ply = Button(f1, text = "Replay Event", bg = "green", command = lambda x = True: play(x))
                ply.pack(fill = BOTH, expand = True)
            
            f2 = Frame(b0, width = 250, height = 125)
            f2.pack_propagate(0)
            f2.pack(side = LEFT)
            
            if stop_flag == False:
                back = Button(f2, text = "Back to Menu", bg = "blue", command = back_menu)
                back.pack(fill = BOTH, expand = True)
            else:
                back = Message(f2, text = "Please Pause or Stop to return to the menu", bg = "blue")
                back.pack(fill = BOTH, expand = True)
            
            f3 = Frame(b0, width = 250, height = 125)
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
            
            f4 = Frame(b2, width = 25, height = 125)
            f4.pack_propagate(0)
            f4.pack(side = LEFT)
            
            f5 = Frame(b2, width = 250, height = 125)
            f5.pack_propagate(0)
            f5.pack(side = LEFT)
            
            if len(files) > 1 and event_file != files[0]:
                if stop_flag == False:
                    prev = Button(f5, text = "Previous Event", bg = "blue", command = lambda x = index: prev_event(x))
                    prev.pack(fill = BOTH, expand = True)
                else:
                    prev = Message(f5, text = "Please Pause or Stop to change events", bg = "blue")
                    prev.pack(fill = BOTH, expand = True)
                
            f6 = Frame(b2, width = 250, height = 125)
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
            
            f7 = Frame(b2, width = 250, height = 125)
            f7.pack_propagate(0)
            f7.pack(side = LEFT)
            
            if len(files) > 1 and event_file != files[-1]:
                if stop_flag == False:
                    nxt = Button(f7, text = "Next Event", bg = "blue", command = lambda x = index: next_event(x))
                    nxt.pack(fill = BOTH, expand = True)
                else:
                    nxt = Message(f7, text = "Please Pause or Stop to change events", bg = "blue")
                    nxt.pack(fill = BOTH, expand = True)
            
        if __name__ == '__main__':
            x1 = threading.Thread(target = page)
            x1.start()
            if play_flag == True:
                x2 = threading.Thread(target = run_event)
                x2.start()
        
if __name__ == "__main__":
    app = Dot_GUI()
    app.mainloop()