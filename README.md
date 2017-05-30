# UMD i3Model
## LED model of the IceCube Observatory

The display is a 3D array of 4800 NeoPixel LEDs arranged on one serial data pin. Data to be displayed is stored on a microSD card. The user interface is a 2.8" Adafruit capacative touch screen. These are all controlled by an Arduino Due.

To prepare files for the Due, there is a python-based converter that takes `.i3` files and converts them to `.I3R` format.

Hardware: Elim Cheung, Greg Sullivan

Python software: Elim Cheung, John Evans

Arduino software: John Evans
