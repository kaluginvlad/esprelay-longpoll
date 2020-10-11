/*
 * WiFi-EEPROM configuration library
 * 
 * Vladislav Kalugin (c) 2020
 *
 * EEPROM ADDRESSES:
 * -- 0000 - 0003 - Checksum 
 * -- 0004 - **** - String data (JSON)
 * 
 */

#include <Ticker.h>
#include <ArduinoJson.h>

#include "web_assets/c++/html_config.h"

const int MAX_CONNECTION_TICKS = 100; // Amount of the WiFi ticks before connection fail status
                                      // 1 Tick - 200 ms
                                      
#define AP_SSID "ESP_TEST"            // SSID of the Soft-AP
#define AP_PASS "IOT_PA55W()RD!"      // Password of the Soft-AP

#define LED_PIN 2                     // LED pin
bool REVERSE_LED = true;              // Reverse led signal

#define CONF_JSON_BLANK "{\"stat\":0,\"ssid\":null,\"pass\":null,\"host\":null,\"dev_key\":null}"

// Setup ticker for wifi connection checker
Ticker wifi_update_ticker;
// Led blink ticker
Ticker led_blink_ticker;

// Start WiFi in AP mode
void startAP() {
  // Set WiFi to the AP mode
  WiFi.mode(WIFI_AP);
  // Start AP
  WiFi.softAP(AP_SSID, AP_PASS);
}

// Checks WiFi status
void checkWiFiFail() {
  // IF connection lost
  if (WiFi.status() != WL_CONNECTED) {
    // Stop ticker
    wifi_update_ticker.detach();
    // Turn off led
    digitalWrite(LED_PIN, REVERSE_LED);
    Serial.println("WiFi connection lost! Restarting...");
    // Restart unit
    ESP.restart();
  }
}

// Handle config webpage
void HTTPhandleWiFiConf(ESP8266WebServer &web_srv) {

  // Configuration page
  web_srv.on("/config", [&web_srv]() {
    web_srv.send(200, "text/html", html_config);
  });

  // Config API
  web_srv.on("/api/configure", [&web_srv](){
    // Check all args
    if (web_srv.hasArg("SSID") && web_srv.hasArg("PASS") && web_srv.hasArg("HOST") && web_srv.hasArg("KEY")) {
      // Send info, that configuration had been accepred
      web_srv.send(200, "text/html", "Configuration accepted! Rebooting...");
      delay(500);
      // Call config update fuction;
      updateWiFIConfig(web_srv.arg("SSID"), web_srv.arg("PASS"), web_srv.arg("HOST"), web_srv.arg("KEY"));
    }
    else {
      web_srv.send(400, "text/html", "Invalid arguments!");
    }
  });

  // Check WiFi creds
  web_srv.on("/api/wificheck", [&web_srv]() {
    if (WiFi.status() == WL_CONNECTED) {
      web_srv.send(200, "text/html", "already connected");
      return;
    }

    WiFi.begin(web_srv.arg("SSID"), web_srv.arg("PASS"));
    for (int i = 0; i < 100; i++) {
      if (WiFi.status() == WL_CONNECTED) {
        web_srv.send(200, "text/html", "success");
        break;
      }
      delay(100);
    }
    web_srv.send(200, "text/html", "fail");

  });

}

void led_toggle() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

// Configures WiFi
bool WiFiConfigure(ESP8266WebServer &web_srv) {

  // Add config page handling
  HTTPhandleWiFiConf(web_srv);

  // Setup Led pin as output
  pinMode(LED_PIN, OUTPUT);

  // Verify EEPROM checksum
  if (EEPROMReadlong(0) == eeprom_crc()) {
    Serial.println("EEPROM Checksum - OK!");
    
    StaticJsonDocument<256> iotconf = readWiFiConfig();

    if (iotconf["stat"] == 1) {
      Serial.println("Config status - OK!");
      Serial.println("Setting up Wi-Fi...");

      // Set WiFi to the STATION (client) mode
      WiFi.mode(WIFI_STA);
      // Start WiFi connection
      WiFi.begin(iotconf["ssid"].as<String>(), iotconf["pass"].as<String>());
      // Set the hostname
      WiFi.hostname(iotconf["host"].as<String>());
      
    }
    else {
      Serial.println("Config status - UNCONFIGURED!");
      Serial.println("WARNING!!!! Starting WiFi in AP mode!");

      // Start blink
      led_blink_ticker.attach(0.1, led_toggle);

      startAP();
     
      return false;
    }
  } 
  else {
    Serial.println("EEPROM Checksum - FAIL!\nCleaning EEPROM...");
    // Clear EEPROM
    EEPROMClear();
    // Write empty config to a EEPROM
    EEPROMWriteString(4, CONF_JSON_BLANK);
    // Update checksum
    eeprom_crc_update();
    Serial.println("WARNING!!!! Starting WiFi in AP mode!");
      
    // Start blink
    led_blink_ticker.attach(0.1, led_toggle);
    startAP();
    
    return false;
  }

  int connectionTicks = 0;

  Serial.print("Waiting for WiFi connection...");  

  // Wait till WiFi is connecting
  while (WiFi.status() != WL_CONNECTED && connectionTicks < MAX_CONNECTION_TICKS) {
    Serial.print(".");
    connectionTicks++;
    led_toggle();
    delay(200);
  }

  // Add config page handling
  HTTPhandleWiFiConf(web_srv);

  // Check connection ticks
  if (connectionTicks >= MAX_CONNECTION_TICKS) {
    Serial.println("\n\nWiFi connection FAIL!\nStarting as AP!");
    startAP();

    // Start blink
    led_blink_ticker.attach(0.1, led_toggle);
    //led_blink_ticker.start();

    return false;
  }
  else {
    // Print ip and other stuff
    Serial.println("\n\nWiFi connected!\nIP address: " + WiFi.localIP().toString() + "\nNet mask: " + WiFi.subnetMask().toString() + "\nGateway: " + WiFi.gatewayIP().toString() + "\nDNS: " + WiFi.dnsIP().toString());
    // Register WiFi fail checker
    wifi_update_ticker.attach(1, checkWiFiFail);
    // Turn on led
    digitalWrite(LED_PIN, !REVERSE_LED);

    return true;
  }
}
