#pragma once

struct Button {
    Button(uint8_t pin);

    //returns pressed time in ms
    uint16_t pressed();

    private:
    const uint8_t m_pin;
    uint32_t m_lastms;
    bool m_prev_state;
};


Button::Button(uint8_t pin)
: m_pin(pin), m_lastms(0), m_prev_state(1)
{
    pinMode(pin, INPUT_PULLUP);
}


uint16_t Button::pressed(){
    bool b = digitalRead(m_pin);
    //same state as last time, return 0 if up(1), down time in ms if down(0)
    if(b == m_prev_state) return b ? 0 : millis() - m_lastms;
    //changed, toggle state
    m_prev_state = !m_prev_state;
    //if down, record time
    if(b == 0) m_lastms = millis();
    return 0;
}
