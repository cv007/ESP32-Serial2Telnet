#pragma once

#include <WiFi.h>

struct Commander {

    //WiFiClient&   - so can print to
    //String        - command line string (already trimmed)
    static void process(WiFiClient&, String);

};
