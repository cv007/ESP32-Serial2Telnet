#include "LedStatus.hpp"
#include "Arduino.h"

uint8_t             m_pin = 255;
bool                m_invert;
uint8_t             m_count;
uint8_t             m_count_max = 20; //2 seconds
hw_timer_t*         m_timer0 = NULL;

void pin_on(){ digitalWrite(m_pin, !m_invert); }
void pin_off(){ digitalWrite(m_pin, m_invert); }

void IRAM_ATTR led_update();

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
void LedStatus::on(){ m_count_max = 0; m_count = 0; pin_on(); }
void LedStatus::off(){ m_count_max = 0; m_count = 0; pin_off(); }
void LedStatus::slow(){ m_count_max = 10; m_count = 0; }
void LedStatus::fast(){ m_count_max = 2; m_count = 0; }


void IRAM_ATTR led_update() {
    if(m_count_max == 0) return; //OFF or ON, do not toggle
    if(m_count++ >= m_count_max){
        m_count = 0; //count 0-m_count_max
        digitalWrite(m_pin, !digitalRead(m_pin));
    }
}
