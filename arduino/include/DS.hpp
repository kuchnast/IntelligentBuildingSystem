#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <stdio.h>
#include <string.h>

struct oneWirePin
{
    byte pin;
    OneWire *oneWire;
    DallasTemperature *linia;

    explicit oneWirePin(byte pin_lini) : pin(pin_lini), oneWire(nullptr), linia(nullptr) {}

    ~oneWirePin()
    {
        if (linia != nullptr)
            delete linia;

        if (oneWire != nullptr)
            delete oneWire;
    }

    bool operator==(oneWirePin E)
    {
        return E.pin == pin;
    }

    void Init()
    {
        if (oneWire == nullptr && linia == nullptr)
        {
            oneWire = new OneWire(pin);
            linia = new DallasTemperature(oneWire);
        }
    }
};

class DS
{
private:
    int m_id;
    oneWirePin *m_linia;
    DeviceAddress m_adres;
    float m_temp;

public:
    explicit DS(int id) : m_id(id), m_linia(nullptr), m_temp(0) {}

    DS(int id, oneWirePin *linia, DeviceAddress adr) : m_id(id), m_linia(linia), m_temp(0)
    {
        memcpy(m_adres, adr, 8);
    }

    bool operator==(const DS &E) const
    {
        return E.m_id == m_id;
    }

    bool checkDSOnline()
    {
        return m_linia->linia->isConnected(m_adres);
    }

    void setResolution(int res)
    {
        if (res >= 9 && res <= 12)
        {
            m_linia->linia->setResolution(res);
        }
    }

    void requestTempThisDevice()
    {
        m_linia->linia->requestTemperaturesByAddress(m_adres);
    }

    float readDSTemp()
    {
        m_temp = m_linia->linia->getTempC(m_adres);
        return m_temp;
    }
};