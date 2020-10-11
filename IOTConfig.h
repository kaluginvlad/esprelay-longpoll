/*
*
* ESP8266-IOT Config tool
*
* Vladislav Kalugin (c) 2020
*
*/

#include <Arduino.h>

class IOTConfig {
  public:
    void configure(String conf);
    
    bool confStatus();
    String ssid();
    String pass();
    String host();
    String secret_key();
    String _get_option(int opt_id);
    
  private:
    String _conf;
};
