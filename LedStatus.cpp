#include "LedStatus.hpp"
#include "Arduino.h" //pin stuff

//10Hz timer0 irq- calls led_update
//if count exceeds count max, toggle pin (unless count max is 0)
//count max determines length of led state (in 1/10sec increments),

//local enum- on/off time in 1/10sec
//slow = 10 = 1.0sec on, 1.0sec off
//fast = 2 = 0.2sec on, 0.2sec off
enum { OFF, ON = 0, SLOW = 10, FAST = 2 };

//local vars
uint8_t             m_pin = 255;
bool                m_invert;
uint8_t             m_count;
uint8_t             m_count_max;
hw_timer_t*         m_timer0 = NULL;

//local functions
void IRAM_ATTR led_update()
{
    if(m_count_max == 0) return;    //do not toggle if 0
    if(m_count++ >= m_count_max){   //count 0-m_count_max
        m_count = 0;                //reset count
        digitalWrite(m_pin, !digitalRead(m_pin));
    }
}
void reset_max(uint8_t max){ m_count_max = max; m_count = 0; }


//class functions (all functions static)
void LedStatus::init(uint8_t pin, bool invert)
{
    m_pin = pin;
    m_invert = invert;
    pinMode(pin, OUTPUT);
    off();

    //10Hz irq
    m_timer0 = timerBegin(0, 80, true);  //80000000/80 = 1000000MHz, true=count up
    timerAttachInterrupt(m_timer0, &led_update, true);//true=edge type irq
    timerAlarmWrite(m_timer0, 100000, true); //1000000/100000= 10Hz, true=auto reload
    timerAlarmEnable(m_timer0);
}
void LedStatus::on(){   reset_max(ON); digitalWrite(m_pin, !m_invert); }
void LedStatus::off(){  reset_max(OFF); digitalWrite(m_pin, m_invert); }
void LedStatus::slow(){ reset_max(SLOW); }
void LedStatus::fast(){ reset_max(FAST); }


/*

TODO
blink IP address (last digit)

off 2sec
2sec between digits
0 is short blink 0.1sec on
1-9 is 0.3sec on, 0.3sec off

blink ip address- last number
001 - 254

hundreds digit 0-2
tens digit 0-9 or 0-5 when hundreds is 2
ones digit 0-9

. = 0.1sec
_ = off 2sec
* = 0.3sec on
~ = 0.3sec off

009=
_._._*~*~*~*~*~*~*~*~*~_

023=
_._*~*~_*~*~*~_

254=
_*~*~_*~*~*~*~*~_*~*~*~*~_

uint8_t hun = WiFi.localIP()[3]
uint8_t ten = hun/10%10;    //254/10=25 25%10=5
uint8_t one = hun%10;       //254%10=4
hun /= 100;                 //254/100=2





*/
