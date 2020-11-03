#!make

#
# Makefile for ESP8266-longpoll project
# (c) Vladislav Kalugin, 2020
#
# Supported UNIX and WINDOWS
#

#SHELL := '/bin/bash'
SHELL := 'cmd'

#PYTHON_PATH = '/usr/bin/python3'
PYTHON_PATH = 'py'

ESP_URL = 'http://esprelay2.iot.minhinlocal'

#ESP_SER = '/dev/ttyUSB0' 
ESP_SER = 'COM4' 

# Arduino pathes (Windows)
ARDUINO_PATH := 'C:\Program Files (x86)\Arduino\'
ARDUINO_PACK := 'C:\Users\Vladislav Kalugin\AppData\Local\Arduino15\'
ARDUINO_HOME := 'C:\Users\Vladislav Kalugin\Documents\Arduino\'

## Arduino pathes (Linux/UNIX)
#ARDUINO_PATH := '/home/vlk/soft/Arduino'
#ARDUINO_PACK := /home/vlk/.arduino15'
#ARDUINO_HOME := '/home/vlk/Arduino'

all:
	@echo "Building html..."
	make -s html
	@echo "Building sketch..."
	make -s sketch
	@echo "Uploading via OTA..."
	make -s ota_upload

html:
	$(PYTHON_PATH) web_assets/html2cpp.py

sketch: esprelay-longpoll.ino
	arduino-builder -hardware $(ARDUINO_PATH)hardware -hardware $(ARDUINO_PACK)packages -libraries $(ARDUINO_PATH)libraries -libraries $(ARDUINO_HOME)libraries -tools $(ARDUINO_PATH)tools-builder -tools $(ARDUINO_PACK)packages -fqbn esp8266:esp8266:nodemcuv2 -build-path build esprelay-longpoll.ino

ota_upload:
	$(PYTHON_PATH) upload.py $(ESP_URL)

ser_upload:
	esptool --port $(ESP_SER) write_flash 0x0000 build/esprelay-longpoll.ino.bin

clean:
	rm -r build/*
