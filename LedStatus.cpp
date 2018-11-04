#include "LedStatus.hpp"
#include "Arduino.h" //pin stuff
#include <vector>

//10Hz timer0 irq- calls timer0_isr
//if count exceeds count max, toggle pin (unless count max is 0)
//count max determines length of led state (in 1/10sec increments),

//local enum- on/off time in 1/10sec
//slow = 10 = 1.0sec on, 1.0sec off
//fast = 2 = 0.2sec on, 0.2sec off
enum { OFF, ON = 0, SLOW = 10, FAST = 2 };

//local vars
hw_timer_t* m_timer0 = NULL;

//list of all instances of this class
//iterate through in timer0 isr
std::vector<LedStatus*> led_list;

//isr function, check all led's
void IRAM_ATTR timer0_isr()
{
    for(auto i : led_list) i->update();
}

//setup timer0
void timer0_init()
{
    if(m_timer0) return;
    //first instantiation will start timer 0
    //setup timer0 for a 10Hz irq rate
    m_timer0 = timerBegin(0, 80, true);  //80000000/80 = 1000000MHz, true=count up
    timerAttachInterrupt(m_timer0, &timer0_isr, true);//true=edge type irq
    timerAlarmWrite(m_timer0, 100000, true); //1000000/100000= 10Hz, true=auto reload
    timerAlarmEnable(m_timer0);
}



//class functions

//constructor- io pin#, invert? (default is false, so high=on)
LedStatus::LedStatus(uint8_t pin, bool invert)
{
    m_pin = pin;
    m_invert = invert;
    pinMode(pin, OUTPUT);
    off();                      //init state is off
    led_list.push_back(this);   //save this instance to list
    timer0_init();              //init timer0 if not already done
}

//called by timer0 isr (IRAM_ATTR may not be necessary,
// but is called by isr so may be good idea)
void IRAM_ATTR LedStatus::update()
{
    if(m_count_max == 0) return;    //do not toggle if 0
    if(m_count++ >= m_count_max){   //count 0-m_count_max
        m_count = 0;                //reset count
        digitalWrite(m_pin, !digitalRead(m_pin)); //toggle
    }
}
void LedStatus::on()
{
    m_count_max = ON;
    digitalWrite(m_pin, !m_invert);
}
void LedStatus::off()
{
    m_count_max = OFF;
    digitalWrite(m_pin, m_invert);
}
void LedStatus::slow()
{
    m_count_max = SLOW;
    m_count = 0;
}
void LedStatus::fast()
{
    m_count_max = FAST;
    m_count = 0;
}






/*

TODO
blink IP address (last digit)

off 3sec
3sec between digits
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


struct pattern {
    uint8_t* digit[3];
    uint8_t  idx;
}

uint8_t pattern_off[] = { 254, 255 };
uint8_t pattern_on[]  = { 253, 255 };
uint8_t pattern_0[] = { 2, 2, 20, 255 };
uint8_t pattern_1[] = { 5, 5, 20, 255 };
uint8_t pattern_2[] = { 5, 5, 5, 5, 20, 255 };


struct {
    uint8_t pin;
    bool invert;
    uint8_t count;
    pattern* pattern; //NULL = do not change pin
}



*/
