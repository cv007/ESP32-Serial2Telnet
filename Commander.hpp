#pragma once

#include <WiFi.h>

/*
  wifi list                     //list all stored ssid/pass data
  wifi add 0 ssid=test1         //store ssid in slot 0
  wifi add 0 pass=pass1         //store password in slot 0
  wifi erase 0                  //erase ssid/pass of slot 0

  sys boot                      //show boot flag status (AP / STA)
  sys boot=AP                   //set boot mode
  sys boot=STA
  sys erase all                 //clear all stored data

  net info
  net hostname                  //show hostname (dhcp server will record this name)
                                //this shows the both the current hardware and stored value
                                //(both should be the same)

  net hostname=myname           //set hostname (max 32 chars)
                                //if not set, default is- espressif

  net APname                    //show APname (ssid name for access point mode)
  net APname=myAPname           //set APname (max 32 chars)
  net mac                       //show mac address
  net status                    //show telnet servers status

  uart2 baud                    //show uart2 baud
  uart2 baud=115200             //set uart2 baud

*/

struct Commander {

    //WiFiClient&   - so can print to
    //String        - command line string (already trimmed)
    static void process(WiFiClient&, String);

};
