/* ESP8266 - IOT
 * 
 * Longpoll based WiFi relay
 * 
 * Vladislav Kalugin (c) 2020
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <string.h>
#include <Ticker.h>

#include "EEPROM_tools.h"
#include "config_tools.h"
#include "WiFi_configurator.h"

// Web pages
#include "web_assets/c++/html_css.h"
#include "web_assets/c++/html_root.h"

// Build version
#define BUILD "aplha-04.11.2020"

bool config_ready;
bool switch_state = false;

Ticker timetable_ticker;

/* Configuration */

#define FORCE_CONF_MODE_PIN   D7
#define RELAY_PIN             D1

#define NTP_SERVER            "ntp1.stratum1.ru"    // Host of the NTP server
#define NTP_UTC_OFFSET        0                     // GMT +3 default
#define NTP_UPDATE_INTERVAL   600000                // 10 mins

const String lp_server = "https://espiot.kaluginvlad.com/";
const String lp_fp = "DC 33 C8 7C BF E5 AE 9E B7 F7 B4 CE 4C 1F EA 38 9F C9 6B AE";

/* ************** */

String lp_access_key;
// WebServer on port 80
ESP8266WebServer web_srv(80);

// HTTP updater
ESP8266HTTPUpdateServer http_updater;

// HTTP client
HTTPClient http;

// UDP fot NTP sync
WiFiUDP ntpUDP;

// NTP Client
NTPClient time_client(ntpUDP, NTP_SERVER, NTP_UTC_OFFSET);

// Structure of timetable
typedef struct  {
  int timestamp;
  int timeout;
  bool switch_state;
} timetable_struct;

// Create struct for timetable (for 50 entries)
timetable_struct actions_timetable[50] = {NULL, NULL, NULL};

// Last timetable sync time
int timetable_sync = 0;
bool timetable_sync_required = false;

// Checks timetable for action
// Runs by Ticker every 1000 ms
void check_timetable() {
  // Sync timetable the next day
  if (time_client.getEpochTime() - timetable_sync >= 86400 ) {
    timetable_sync_required = true;
  }

  for (int i = 0; i < 50; i++) {
    if (actions_timetable[i].timestamp == NULL) continue;

    Serial.println("record");

    int timedelta = time_client.getEpochTime() - actions_timetable[i].timestamp;

    Serial.println("====TIMETABLE DEBUG====");

    Serial.println(actions_timetable[i].timestamp);
    Serial.println(time_client.getEpochTime());

    Serial.println(timedelta);

    Serial.println("=======================");
    
    if ( timedelta >= 0 and timedelta <= actions_timetable[i].timeout ) {
      Serial.println("TIMETABLE: Action!");
      
      // Toggle switch by action
      switch_state = actions_timetable[i].switch_state;
      digitalWrite(RELAY_PIN, switch_state);

      // Clear completed record
      actions_timetable[i] = {NULL, NULL, NULL};
    }
  }
}

// Syncs actions timetable
void sync_timetable() {
  Serial.println("\nSynchronizing timetable...");

  String timetable_url = lp_server+"devapi/get_timetable?access_key="+lp_access_key;
  http.setTimeout(5000);
  http.begin(timetable_url, lp_fp);
  http.addHeader("Content-Type", "application/json");

  int http_code = http.GET();

  Serial.println(http_code);

  String timetable_payload;

  if (http_code == 200) {
    timetable_payload = http.getString();
  } else {
    Serial.println("TT SYNC ERROR: Non-200 htcode");
    return;
  }
  http.end();

  Serial.println(timetable_payload);

  // JSON Doc for timetable
  StaticJsonDocument<1024> timetable_doc;

  // Decode data
  deserializeJson(timetable_doc, timetable_payload);

  // Clean payload string
  timetable_payload = "";

  if (timetable_doc["status"] == "ok") {

    // Clear old data
    actions_timetable[50] = {NULL, NULL, NULL};

    // Write new timetable data
    for (int i = 0; i < timetable_doc["table"].size(); i++) {
      actions_timetable[i] = {timetable_doc["table"][i][0], timetable_doc["table"][i][1], timetable_doc["table"][i][2]};
    }

    Serial.println("Timetable sync completed!");

    // Write last timetable sync time
    timetable_sync = time_client.getEpochTime();

    timetable_ticker.attach(1, check_timetable);

  } else {
    Serial.println("TT SYNC ERROR: Non-ok status");
  }
}

// Send detailed report to server as json
void send_detailed_report() {

  StaticJsonDocument<256> conf = readWiFiConfig();
  
  StaticJsonDocument<256> report_doc;
  JsonObject root = report_doc.to<JsonObject>();

  root["dev_type"] = "esp_relay";
  
  root["ssid"] = conf["ssid"].as<String>();

  conf.clear(); // Unload JSON

  root["ip"] = WiFi.localIP().toString();
  root["mask"] = WiFi.subnetMask().toString();
  root["gw"] = WiFi.gatewayIP().toString();
  root["dns"] = WiFi.dnsIP().toString();
  root["switch"] = switch_state;
  root["build"] = BUILD;

  char serialized_doc[512];
  serializeJson(report_doc, serialized_doc);

  report_doc.clear(); // Unload JSON document from memory

  Serial.println(String(serialized_doc));
  Serial.println("Report done! Uploading...");

  String report_url = lp_server+"devapi/iot_report?access_key="+lp_access_key;
  http.setTimeout(5000);
  http.begin(report_url, lp_fp);
  http.addHeader("Content-Type", "application/json");

  int http_code = http.POST(String(serialized_doc));
  
  Serial.println(http_code);
  Serial.println("Report had been sent!");

  http.end();
}

// Send simple report (relay state) to server as json
void send_status_report() {

  String switch_state_str;

  if (switch_state) switch_state_str = "true"; else switch_state_str = "false";

  String report = "{\"switch\": "+switch_state_str+"}";

  String report_url = lp_server+"devapi/iot_report?access_key="+lp_access_key;
  http.setTimeout(5000);
  http.begin(report_url, lp_fp);
  http.addHeader("Content-Type", "application/json");

  http.POST(report);

  http.end();
}

void setup() {
  // Begin serial
  Serial.begin(115200);
  // Send a sort of a logo :)
  Serial.println(
  "\n\n"
  "    __ __      __            _     _    ____          __                    \n"
  "   / //_/___ _/ /_  ______ _(_)___| |  / / /___ _____/ /_________  ____ ___ \n"
  "  / ,< / __ `/ / / / / __ `/ / __ \\ | / / / __ `/ __  // ___/ __ \\/ __ `__ \\\n"
  " / /| / /_/ / / /_/ / /_/ / / / / / |/ / / /_/ / /_/ // /__/ /_/ / / / / / /\n"
  "/_/ |_\\__,_/_/\\__,_/\\__, /_/_/ /_/|___/_/\\__,_/\\__,_(_)___/\\____/_/ /_/ /_/ \n"
  "                   /____/                                                   "
  );

  Serial.print("\nESP8266 IOT LONGPOLL RELAY\nBuild: ");
  Serial.println(BUILD);
  Serial.println("\n");

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  // Begin EEPROM 
  EEPROM.begin(4096);

  // Start WiFi init
  config_ready = WiFiConfigure(web_srv);

  pinMode(FORCE_CONF_MODE_PIN, INPUT_PULLUP);

  // Add ovrride dialogue
  if (config_ready) {
    Serial.flush();
    Serial.print("Hold conf override button to enter conf mode!\n");
    for (int i = 0; i < 10; i++) {
      if (digitalRead(FORCE_CONF_MODE_PIN) == LOW || Serial.available()) {
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
    http_updater.setup(&web_srv, "/firmware");
    // Start WEB-server
    web_srv.begin();
    Serial.println("Web configurator ready!");
  } else {
      StaticJsonDocument<256> iotconf = readWiFiConfig();
      lp_access_key = iotconf["dkey"].as<String>();

      Serial.print("\nNTP Server: ");
      Serial.println(NTP_SERVER);
      Serial.println("Syncing TIME...");

      time_client.begin();
      time_client.update();

      Serial.print("UNIX Timestamp: ");
      Serial.println(time_client.getEpochTime());

      // Start timetable synchronization
      sync_timetable();
    
      Serial.println("\n\n[ACCOUNTING DATA]\nURL: "+lp_server+"\nFingerprint: "+lp_fp+"\nDevice key: "+lp_access_key+"\n");
      Serial.println("Ready!\n");
  }
}

void loop() {
  if (!config_ready) {
     web_srv.handleClient();
  } else {
    // Wait for longpoll
    request_logpoll();
    // Sync timetable if required
    if (timetable_sync_required) {
      Serial.println("Timetable sync by requirement");
      sync_timetable();
      timetable_sync_required = false;
    }
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
  String url = lp_server+"devapi/longpoll?access_key="+lp_access_key;
  http.setTimeout(25000);
  http.begin(url, lp_fp);

  int http_code = http.GET();

  Serial.println(http_code);
  Serial.println(http.getString());

  
  if (http_code == 200) {
    StaticJsonDocument<200> action;
    DeserializationError error = deserializeJson(action, http.getString());
    
    http.end(); // Close connection

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

    action.clear();

    
  } else if (http_code == 401) {
    Serial.println("Wrong device code! Clearing config in 5 sec...");
    delay(5000);
    EEPROMClear();
    ESP.restart(); 
  } else {
    Serial.println("Unknown http code! Waiting 5 sec...");
    delay(5000);
  }
}

// Process actions
void check_action(String action) {
  if (action == "on") {
    digitalWrite(RELAY_PIN, HIGH);
    switch_state = true;
  } else if (action == "off") {
    digitalWrite(RELAY_PIN, LOW);
    switch_state = false;
  } else if (action == "getswitch") {
    send_status_report();
  } else if (action == "getreport") {
    send_detailed_report();
  } else if (action == "ntpsync") {
    Serial.println("Syncing NTP by request...");
    time_client.update();
  } else if (action == "ttblsync") {
    Serial.println("Syncing timetable by request...");
    sync_timetable();
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
