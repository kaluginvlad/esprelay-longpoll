/*
*
* ESP8266-IOT Config tool
*
* Vladislav Kalugin (c) 2020
*
*/

#include <string.h>
#include <Arduino.h>

#include "IOTConfig.h"

void IOTConfig::configure(String conf) {
  _conf = conf;
}

String IOTConfig::_get_option(int opt_id) {
  int ind_s = 0;
  int ind_e = 0;

  int i = 0;
  
  while (ind_e != -1) {
    ind_e = _conf.indexOf(";", ind_s);

    if (i == opt_id) {
      return _conf.substring(ind_s, ind_e);
    }
    
    i++;
    ind_s = ind_e + 1;
  }
}

bool IOTConfig::confStatus() {
  String confStatus = this->_get_option(0);
  if (confStatus == "1") {
    return true;
  } else {
    return false;
  }
}

// Functions to get option

String IOTConfig::ssid() {
  return this->_get_option(1);
}

String IOTConfig::pass() {
  return this->_get_option(2);
}

String IOTConfig::host() {
  return this->_get_option(3);
}

String IOTConfig::secret_key() {
  return this->_get_option(4);
}
