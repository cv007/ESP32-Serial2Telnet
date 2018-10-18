#pragma once

#include <WiFi.h>

/*
  wifi list                     //list all stored ssid/pass data
  wifi add 0 ssid=test1         //store ssid in slot 0
  wifi add 0 pass=pass1         //store password in slot 0
  wifi erase 0                  //erase ssid/pass of slot 0

  sys boot                      //show boot2ap flag status (AP / STA)
  sys boot=AP                   //set boot mode
  sys boot=STA
  sys erase all                 //clear all stored data

  net info
  net hostname                  //show hostname (dhcp server will record this name)
  net hostname=myname           //set hostname (max 32 chars)
  net APname                    //show APname (ssid name for access point mode)
  net APname=myAPname           //set APname (max 32 chars)
  net mac                       //show mac address
*/

struct Commander {

    //WiFiClient&   - so can print to
    //uint8_t*      - command line buffer (always 0 terminated)
    //return        - false = keep adding to buffer (no cr or lf found)
    //              - true = caller can discard buffer and start new command buffer
    static bool process(WiFiClient&, uint8_t*);

};
