#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import print_function
from PIL import Image, ImageFont, ImageDraw
import binascii
import base64
#import pyautogui
#import mss
#import mss.tools
import time
import serial
import threading

import psutil

#pip install python3_xlib python-xlib
# pip install pyautogui
# sudo pip install pil
#sudo apt-get install scrot

# Rpi
# sudo pip3 install Pillow
# sudo apt-get install libopenjp2-7
# sudo apt-get install -y pngtools libtiff-dev libtiff5
# pip install pyautogui
# pip install python3_xlib python-xlib
# pip install psutil

# pm2 start python3 --name "usb-sender" -- ~/usb-sender/usb-sender.py -u ~/usb-sender/

orientation = 0

def serial_open(port):
    return serial.Serial(
        port=port,
        baudrate=921600,
        timeout=0
    )

    #ser.isOpen()

def display_image(img, ser):
    #print(im.format, im.size, im.mode)
    img = img.convert('1') # convert image to black and white

    angle = {
        0: 0,
        1: 0,
        2: 90,
        3: 180,
        4: 0,
        5: -90,
        6: 0
    }
    global orientation
    img = img.rotate(angle[orientation])

    pixels = img.load()

    byte_array = [0]*(16*128)
    index = 0

    for y in range(0, 128):
        for x in range(0, 128, 8):
            for bit in range(0, 8):
                byte_array[index] |= (0 if pixels[(x + bit, y)] == 0 else 1) << (7 - bit)
            index += 1

    #im.save('result.png')
    #print(binascii.hexlify(bytearray(byte_array)))
    #print("BAse64:")
    b64 = base64.b64encode(bytearray(byte_array))
    #print(b64)

    ser.write(b64)
    ser.write(b'\r\n')
    ser.flush()

screen_current = 2

def screen_cycle(dir):
    global screen_current
    screen_current += dir
    print("screen cycle:" + str(screen_current))

def screen_draw():
    if screen_current == 0:
        # area = (500, 500, 628, 628)
        # with mss.mss() as sct:
        #     sct_img = sct.grab(area)

        # img = Image.frombytes("RGB", sct_img.size, sct_img.bgra, "raw", "BGRX")
        # #im = im.resize((128,128))
        # #im = Image.open("test.png")
        img = Image.new("RGB", (128, 128), (255,255,255))
        return (img, 0)
    # CPU/RAM
    if screen_current == 1:
        img = Image.new("RGB", (128, 128), (255,255,255))
        font = ImageFont.truetype("UbuntuMono-Regular.ttf", 20)
        text_draw = ImageDraw.Draw(img)

        text_draw.text((10, 10), "System ", font=font, fill=(0,0,0))
        text_draw.text((10, 40), "CPU: " + str(psutil.cpu_percent()) + "%", font=font, fill=(0,0,0))
        text_draw.text((10, 60), "RAM: " + str(psutil.virtual_memory()[2]) + "%", font=font, fill=(0,0,0))
        text_draw.text((10, 80), "SWAP: " + str(psutil.swap_memory().percent) + "%", font=font, fill=(0,0,0))
        text_draw.text((10, 100), "DISK: " + str(psutil.disk_usage('/').percent) + "%", font=font, fill=(0,0,0))

        img_gear = Image.open("gear2.png")
        img_gear = img_gear.resize((25, 25))
        x = 80
        y = 5
        sizex, sizey = img_gear.size
        img.paste(img_gear, (x, y, x + sizex, y + sizey))

        return (img, 0.1)
    # NETWORK
    if screen_current == 2:
        img = Image.new("RGB", (128, 128), (255,255,255))
        font = ImageFont.truetype("UbuntuMono-Regular.ttf", 20)
        smaller = ImageFont.truetype("UbuntuMono-Regular.ttf", 15)
        text_draw = ImageDraw.Draw(img)

        text_draw.text((10, 10), "Network ", font=font, fill=(0,0,0))

        posy = 40
        #print(psutil.net_if_addrs())
        interfaces = psutil.net_if_addrs()
        for k, v in interfaces.items():
            if k != 'lo':
                text_draw.text((10, posy), k + ": ", font=smaller, fill=(0,0,0))
                posy += 16
                text_draw.text((10, posy), v[0].address, font=smaller, fill=(0,0,0))
                posy += 20


        img_gear = Image.open("network.png")
        img_gear = img_gear.resize((25, 25))
        x = 80
        y = 7
        sizex, sizey = img_gear.size
        img.paste(img_gear, (x, y, x + sizex, y + sizey))

        return (img, 0.1)
    else:
        img = Image.open("test.png")
        img = img.resize((128, 128))
        return (img, 0.1)

def serial_command(cmd):
    global orientation
    print(cmd)
    if "accelerometer" in cmd:
        orientation = int(cmd.split(":")[1])

    if "button" in cmd:
        if "left" in cmd:
            screen_cycle(-1)
        if "right" in cmd:
            screen_cycle(+1)

    if "gesture" in cmd:
        if "left" in cmd:
            screen_cycle(-1)
        if "right" in cmd:
            screen_cycle(+1)

print("Opening port")
ser = False

while True:
    try:
        ser = serial_open("/dev/ttyUSB0")
    except Exception as e:
        print("Cannot open port: " + str(e))
        time.sleep(1)

    print("Port opened")

    while (serial):
        img, delay = screen_draw()

        try:
            display_image(img, ser)

            l = ser.readline()
            if len(l) > 0:
                serial_command(l.decode().strip())

        except Exception as e:
            print(str(e))
            ser = False
            break

        time.sleep(delay)
