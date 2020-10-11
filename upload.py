# ESP-IOT firmware uploader

import requests, time, sys

esp_host = sys.argv[1]

fw_path = "build/esp8266_longpoll.ino.bin"

print("HOST: %s" % (esp_host))
print("Waiting for ESP8266...") 

while (True):
    try:
        result = requests.get(esp_host)

        if (result.status_code == 200):
            print ("ESP8266 ready!")
        break
    except: 
        pass

    time.sleep(1)

print("Uploading firmware...")

files = {
    "firmware": ("firmware.bin", open(fw_path, "rb"))
}

result = requests.post(esp_host + "/firmware", files=files)

if (result.status_code == 200):
    print("Upload succeeded!")
else:
    print("Got wrong status code: %s" % (result.status_code))
    print(result.text)