#pragma once

#include <Arduino.h>

class PoziomWody
{
private:
    uint8_t m_analogPin;

public:
    PoziomWody(uint8_t analogPin) : m_analogPin(analogPin) {}

    bool operator==(PoziomWody E)
    {
        return E.m_analogPin == m_analogPin;
    }

    int Read()
    {
        return analogRead(m_analogPin);
    }
};