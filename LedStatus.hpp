#pragma once

#include <stdint.h>

struct LedStatus {

    //pin, invert?
    static void init(uint8_t=23, bool=false);

    //set state
    static void on();
    static void off();
    static void slow();
    static void fast();

};



