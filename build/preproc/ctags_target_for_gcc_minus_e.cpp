# 1 "/home/vlk/projects/esprelay-longpoll/esp8266_longpoll.ino"
/* ESP8266 - IOT

 * 

 * Longpoll based WiFi relay

 * 

 * Vladislav Kalugin (c) 2020

 */
# 8 "/home/vlk/projects/esprelay-longpoll/esp8266_longpoll.ino"
# 9 "/home/vlk/projects/esprelay-longpoll/esp8266_longpoll.ino" 2
//#include <WiFiClient.h>
# 11 "/home/vlk/projects/esprelay-longpoll/esp8266_longpoll.ino" 2
# 12 "/home/vlk/projects/esprelay-longpoll/esp8266_longpoll.ino" 2
# 13 "/home/vlk/projects/esprelay-longpoll/esp8266_longpoll.ino" 2
# 14 "/home/vlk/projects/esprelay-longpoll/esp8266_longpoll.ino" 2
# 15 "/home/vlk/projects/esprelay-longpoll/esp8266_longpoll.ino" 2
# 16 "/home/vlk/projects/esprelay-longpoll/esp8266_longpoll.ino" 2

# 18 "/home/vlk/projects/esprelay-longpoll/esp8266_longpoll.ino" 2

// Web pages
# 21 "/home/vlk/projects/esprelay-longpoll/esp8266_longpoll.ino" 2
# 22 "/home/vlk/projects/esprelay-longpoll/esp8266_longpoll.ino" 2

bool config_ready;
bool switch_state = false;

/* Configuration */




const String lp_server = "https://espiot.kaluginvlad.com/";
const String lp_fp = "DC 33 C8 7C BF E5 AE 9E B7 F7 B4 CE 4C 1F EA 38 9F C9 6B AE";

/* ************** */

String lp_access_key;

// Create instance of WebServer on port 80
ESP8266WebServer web_srv(80);
// Create instance of HTTP updater
ESP8266HTTPUpdateServer httpUpdater;
// Create HTTP client
HTTPClient http;

// Send report to server as json
void send_report() {
  conf.configure(EEPROMReadString(4)); // Init configurator

  StaticJsonDocument<500> report_doc;
  JsonObject root = report_doc.to<JsonObject>();

  root["dev_type"] = "esp_relay";

  root["ssid"] = conf.ssid();
  root["ip"] = WiFi.localIP().toString();
  root["mask"] = WiFi.subnetMask().toString();
  root["gw"] = WiFi.gatewayIP().toString();
  root["dns"] = WiFi.dnsIP().toString();
  root["switch"] = switch_state;

  char serialized_doc[1000];
  serializeJson(report_doc, serialized_doc);

  String report_url = lp_server+"api/iot_report?access_key="+lp_access_key;
  http.setTimeout(2000);
  http.begin(report_url, lp_fp);
  http.addHeader("Content-Type", "application/json");

  int http_code = http.POST(String (serialized_doc));

  Serial.println(http_code);
  Serial.println(http.getString());
  Serial.println("Report had been sent!");

}

void setup() {
  // Begin serial
  Serial.begin(115200);
  // Send a sort of logo :)
  Serial.println(
  "\n\n"
  "    __ __      __            _     _    ____          __                    \n"
  "   / //_/___ _/ /_  ______ _(_)___| |  / / /___ _____/ /_________  ____ ___ \n"
  "  / ,< / __ `/ / / / / __ `/ / __ \\ | / / / __ `/ __  // ___/ __ \\/ __ `__ \\\n"
  " / /| / /_/ / / /_/ / /_/ / / / / / |/ / / /_/ / /_/ // /__/ /_/ / / / / / /\n"
  "/_/ |_\\__,_/_/\\__,_/\\__, /_/_/ /_/|___/_/\\__,_/\\__,_(_)___/\\____/_/ /_/ /_/ \n"
  "                   /____/                                                   "
  );

  Serial.println("\nESP8266-IOT CONFIGURATOR ver. 0.6 (c) Vladislav Kalugin, 02.10.2020");
  Serial.println("ESP IOT Relay ver 0.1 alpha\n");

  pinMode(D1, 0x01);

  // Begin EEPROM 
  EEPROM.begin(4096);

  // Start WiFi init
  config_ready = WiFiConfigure(web_srv);

  pinMode(D7, 0x02);

  // Add ovrride dialogue
  if (config_ready) {
    Serial.flush();
    Serial.print("Hold conf override button to enter conf mode!\n");
    for (int i = 0; i < 10; i++) {
      if (digitalRead(D7) == 0x0 || Serial.available()) {
        config_ready = false;
        Serial.println("\nCONFIG MODE OVERRIDEN!");
        break;
      }
      Serial.print(".");
      delay(100);
    }
  }

  if (!config_ready) {
    // Add WEB server handlers
    web_srv.on("/", HTTP_handleRoot);
    web_srv.on("/style.css", HTTP_handleCSS);

    // Start HTTP Updater  
    httpUpdater.setup(&web_srv, "/firmware");
    // Start WEB-server
    web_srv.begin();
    Serial.println("Web configurator ready!");
  } else {
     conf.configure(EEPROMReadString(4)); // Init configurator
     lp_access_key = conf.secret_key();

     Serial.println("\n\n[SERVICE DATA]\nURL: "+lp_server+"\nFingerprint: "+lp_fp+"\nDevice key: "+lp_access_key+"\n");
     Serial.println("READY!\n");
  }
}

void loop() {
  if (!config_ready) {
     web_srv.handleClient();
  } else {
    request_logpoll();
  }
  delay(50);
}

void HTTP_handleRoot() {
  web_srv.send(200, "text/html", html_root);
}

// Handle style file
void HTTP_handleCSS() {
  web_srv.send(200, "text/css", html_css);
}

void request_logpoll() {
  String url = lp_server+"longpoll?access_key="+lp_access_key;
  http.setTimeout(25000);
  http.begin(url, lp_fp);
  int http_code = http.GET();

  Serial.println(http_code);
  Serial.println(http.getString());

  if (http_code == 200) {
    StaticJsonDocument<200> action;
    DeserializationError error = deserializeJson(action, http.getString());

    // Test if parsing succeeds.
    if (error) {
      Serial.println("Data decoding failed");
    } else {
       if (action["status"] == "noact") {
         Serial.println("IDLE");
       } else if (action["status"] == "ok") {
         Serial.println("Got OK!");
         check_action(action["action"]);
       } else {
         Serial.println("Got unknown action status!");
         delay(1000);
       }
    }


  } else if (http_code == 401) {
    Serial.println("Wrong device code! Clearing config in 5 sec...");
    delay(5000);
    EEPROMClear();
    ESP.restart();
  } else {
    Serial.println("Unknown http code! Waiting 5 sec...");
    delay(5000);
  }

  http.end();
}

// Process action
void check_action(String action) {
  if (action == "on") {
    digitalWrite(D1, 0x1);
    switch_state = true;
  } else if (action == "off") {
    digitalWrite(D1, 0x0);
    switch_state = false;
  } else if (action == "getreport") {
    send_report();
  } else if (action == "clreeprom") {
    EEPROMClear();
    ESP.restart();
  } else if (action == "reboot") {
    ESP.restart();
  }
  else {
    Serial.println("Got unknown action!");
  }
}
