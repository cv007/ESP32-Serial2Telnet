#include "Button.hpp"
#include "Arduino.h" //pin stuff

Button::Button(uint8_t pin, uint16_t ms)
: m_pin(pin), m_lastms(0), m_prev_state(UP), m_long_ms(ms)
{
    pinMode(pin, INPUT_PULLUP);
}

uint16_t Button::pressed(){
    if(up()){
        m_prev_state == UP;                     //save state
        return 0;                               //up always 0
    }
    if(m_prev_state == UP){                     //down, if just pressed-
        m_prev_state = DOWN;                    //save state
        m_lastms = millis();                    //record time
    }
    return millis() - m_lastms;                 //down time
}

auto Button::state() -> state_t { return (state_t)digitalRead(m_pin); }
bool Button::down(){ return state() == DOWN; }
bool Button::up(){ return state() == UP; }
bool Button::long_press(){ return pressed() > m_long_ms; }
