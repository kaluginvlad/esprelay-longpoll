#!make

#
# Makefile for ESP8266-longpoll project
# (c) Vladislav Kalugin, 2020
#

SHELL := '/bin/bash'

ESP_URL = 'http://esprelay.iot.minhinlocal'
ESP_SER = '/dev/ttyUSB0' 

all:
	@echo "Building html..."
	make -s html
	@echo "Building sketch..."
	make -s sketch
	@echo "Uploading via OTA..."
	make -s ota_upload

html:
	python3 web_assets/html2cpp.py

sketch: esprelay-longpoll.ino
	arduino-builder -hardware $$ARDUINO_PATH"hardware" -hardware $$ARDUINO_PACK"packages" -libraries $$ARDUINO_PATH"libraries" -libraries $$ARDUINO_HOME"libraries" -tools $$ARDUINO_PATH"tools-builder" -tools $$ARDUINO_PACK"packages" -fqbn esp8266:esp8266:nodemcuv2 -build-path build esprelay-longpoll.ino

ota_upload:
	python3 upload.py $(ESP_URL)

ser_upload:
	esptool --port $(ESP_SER) write_flash 0x0000 build/esprelay-longpoll.ino.bin

clean:
	rm -r build/*
