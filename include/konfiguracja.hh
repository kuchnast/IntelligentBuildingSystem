#ifndef KONFIGURACJA_HH
#define KONFIGURACJA_HH

#include <iostream>
#include <chrono>
#include <ctime>
#include <string>

#define DEBUG 1
#define DEBUG_RUCH 0
#define DEBUG_ARDUINO 1
#define CZY_SPRAWDZAC_DATE_ZDARZEN_RUCHU 0

#define LICZBA_BAZ 3
#define SERWEROWNIA 0
#define SCHODY 1
#define KOTLOWNIA 2

inline void SendError(const char * str, int error_type = 0)
{
    switch (error_type)
    {
    case 0:
        if(!DEBUG)
            return;
        break;

    case 1:
        if (!DEBUG_RUCH)
            return;
        break;

    case 2:
        if (!DEBUG_ARDUINO)
            return;
        break;

    default:
        return;
        break;
    }

    time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cerr << ctime(&tt) << ") " << str << std::endl;
}

inline void SendData(const char *str)
{
    time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string c(ctime(&tt));
    c.pop_back();
    std::cerr << c.c_str() << ") " << str << std::endl;
}
#endif