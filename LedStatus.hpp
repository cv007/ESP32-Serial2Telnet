#pragma once

#include <stdint.h>

struct LedStatus {

    //pin, invert? default= no invert (high=on)
    LedStatus(uint8_t, bool=false);

    //set led state
    void        on      ();
    void        off     ();
    void        slow    ();
    void        fast    ();

    //called from timer0 isr
    void        update  ();

    private:

    uint8_t     m_pin;
    bool        m_invert;
    uint8_t     m_count;
    uint8_t     m_count_max;

};


