#include <ArduinoJson.h>

// Updates config in EEPPROM
void updateWiFIConfig(String ssid, String passwd, String host, String secret_key) {
  StaticJsonDocument<256> eeprom_config_doc;
  JsonObject eeprom_config_doc_root = eeprom_config_doc.to<JsonObject>();

  eeprom_config_doc_root["stat"] = 1;
  eeprom_config_doc_root["ssid"] = ssid;
  eeprom_config_doc_root["pass"] = passwd;
  eeprom_config_doc_root["host"] = host;
  eeprom_config_doc_root["dkey"] = secret_key;
  
  char serialized_doc[512];
  serializeJson(eeprom_config_doc, serialized_doc);

  eeprom_config_doc.clear();

  // Write config to the EEPROM
  EEPROMWriteString(4, String(serialized_doc));
  // Update checksum
  eeprom_crc_update();
  Serial.println("New configuration applied! Rebooting...");
  // Restart ESP8266
  ESP.restart();
}

// Loads config from EEPROM
StaticJsonDocument<256> readWiFiConfig() {
  StaticJsonDocument<256> eeprom_config_doc;
  DeserializationError error = deserializeJson(eeprom_config_doc, EEPROMReadString(4));

  if (error) {
    Serial.println("Config JSON deserialization error! Cleaning up...");
    EEPROMClear();
    ESP.restart();
  }

  return eeprom_config_doc;

}