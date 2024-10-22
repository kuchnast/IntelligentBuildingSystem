#include <iostream>
#include <math.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#include "konfiguracja.hh"
#include "sprzet/tca.hh"

Tca::Tca(){
    m_adres = 0;
    m_uchwyt = -1;
    m_linia = 0;
}

bool Tca::Inicjuj(int adres){
    m_adres = adres;
    m_uchwyt = wiringPiI2CSetup(m_adres); //ustawia uchwyt do ukladu domyslnie pod adresem 0x70

    if (m_uchwyt < 0)
    {
        fprintf(stderr, "Błąd inicjowania układu TCA\n");
        return false;
    }
    m_linia = 0;
    wiringPiI2CWriteReg8(m_uchwyt, REJESTR_TCA, 0x01); //ustawia aktywna linie na port 0 (dla nr_lini = 0)
    return true;
}

int Tca::ObecnaLinia(){
    return m_linia;
}

void Tca::ZmienLinie(int linia){
    if (ObecnaLinia() != linia)
    {
        if (linia >= 0 && linia <= 7)
        { //gdy linia ma prawidlowa wartosc, zmien ja
            wiringPiI2CWriteReg8(m_uchwyt, REJESTR_TCA, pow(2, linia));
            m_linia = linia;
        }
        else
            throw(std::out_of_range("Podano nieprawidłowy numer lini(" + std::to_string(linia) + ")"));
    }
}

std::ostream &operator<<(std::ostream &strm, const Tca &tca){
    strm << "Tca("
         << "Adres: " << tca.m_adres
         << ", Uchwyt: " << tca.m_uchwyt
         << ", Linia: " << tca.m_linia
         << ")" << std::endl;

    return strm;
}