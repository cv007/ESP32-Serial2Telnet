#pragma once

#include <stdint.h>

struct LedStatus {

    //pin, invert? default= IO23, no invert (high=on)
    static void init(uint8_t=23, bool=false);

    //set led state
    static void on();
    static void off();
    static void slow();
    static void fast();

};



