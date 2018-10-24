#include "Button.hpp"

Button::Button(uint8_t pin, uint16_t ms)
: m_pin(pin), m_lastms(0), m_prev_state(UP), m_long_ms(ms)
{
    pinMode(pin, INPUT_PULLUP);
}

uint16_t Button::pressed(){
    state_t p = (state_t)digitalRead(m_pin);    //get pin state
    uint16_t t = p ? 0 : millis() - m_lastms;   //if up t=0, else t=down time
    if(p == m_prev_state) return t;             //no change, return time
    m_prev_state = p;                           //changed, save state
    if(p == DOWN) m_lastms = millis();          //if down, record time
    return 0;                                   //changed to up or down, both are 0ms
}

bool Button::down(){ return (state_t)digitalRead(m_pin) == DOWN; }
bool Button::up(){ return !down(); }
bool Button::long_press(){ return pressed() > m_long_ms; }
