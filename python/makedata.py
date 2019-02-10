
from __future__ import print_function
from PIL import Image
import binascii
import base64
import pyautogui
import time

import mss
import mss.tools

import serial


#pip install python3_xlib python-xlib
# pip install pyautogui
#sudo apt-get install scrot

def get_pixel(im, index):
    row = index // 128
    col = index % 128
    #print("row", row, "col", col)
    return im.getpixel((col, row))

ser = serial.Serial(
    port='/dev/ttyUSB0',
    baudrate=921600
)

ser.isOpen()

def display_image(im):
    #im = pyautogui.screenshot()
    #im = Image.open("test.png")



    #print(im.format, im.size, im.mode)

    im = im.convert('1') # convert image to black and white

    #print("Pixels", im.getpixel((0,0)), im.getpixel((1,0)))

    byte_array = []

    for i in range(0, 128*128, 8):
        binary_str = ""
        for q in range(0,8):
            if get_pixel(im, i + q) == 255:
                # We back the bits from left to right - this way the format is identical to LCDs and MCU just copies the data
                # Colors are also inverted because bit 1 means whit for LCD
                binary_str += '1'
            else:
                binary_str += '0'

        byte = int(binary_str, 2)
        #print(binary_str, "-", byte)

        byte_array.append(byte)

    im.save('result.png')

    #print(binascii.hexlify(bytearray(byte_array)))

    #print("BAse64:")
    b64 = base64.b64encode(bytearray(byte_array))
    #print(b64)


    ser.write(b64)
    ser.write(serial.to_bytes([0x0D]))
        # need 0d CR


while True:
    #im = pyautogui.screenshot()
    area = (500, 500, 800, 800)
    #im = im.crop(area)
    with mss.mss() as sct:
        sct_img = sct.grab(area)

    im = Image.frombytes("RGB", sct_img.size, sct_img.bgra, "raw", "BGRX")
    im = im.resize((128,128))

    # im = Image.open("test.png")
    display_image(im)
    #time.sleep(0.05)
