#pragma once

#include "Arduino.h"

struct Button {

    //provide IO#, assume pullup wanted, and high=off, low=on
    //also provide long press time in ms if wanted (default is 2sec)
    Button(uint8_t pin, uint16_t = 3000);

    //returns pressed time in ms
    //caller just calls as needed and processes the return value
    uint16_t pressed();

    bool down();
    bool up();
    bool long_press();

    private:

    typedef enum : bool { DOWN, UP } state_t;
    const uint8_t m_pin;
    uint32_t m_lastms;
    state_t m_prev_state;
    uint16_t m_long_ms;
};
