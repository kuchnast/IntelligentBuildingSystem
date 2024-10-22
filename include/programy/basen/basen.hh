#pragma once

#include <iostream>
#include <chrono>
#include "konfiguracja.hh"

#define TEMP_SPRZEGLO_MIN 45
#define TEMP_BASEN_ZADANA 30
#define ZWLOKA_SPRZEGLO 60
#define ZWLOKA_BASEN 30

#define CZAS_POMPA 1800

#define CZAS_DZIALANIA_ATRAKCJE 600

struct OdliczanieCzasu
{
    std::chrono::system_clock::time_point czas;
    const std::chrono::duration<int> ile_sekund;
    bool czy_ustawiono;

    OdliczanieCzasu(int czas_odliczania) : czas(std::chrono::system_clock::now()), ile_sekund(czas_odliczania), czy_ustawiono(false) {}

    void UstawCzasJesliNieUstawiono()
    {
        if (czy_ustawiono == false)
        {
            czas = std::chrono::system_clock::now();
            czy_ustawiono = true;
        }
    }

    bool CzyCzasUplynol(std::chrono::system_clock::time_point &teraz)
    {
        using namespace std::chrono;
        if (czy_ustawiono)
        {
            if (duration_cast<seconds>(teraz - czas).count() >= ile_sekund.count())
            {
                czy_ustawiono = false;
                return true;
            }

            return false;
        }

        return false;
    }

    void Reset()
    {
        czy_ustawiono = false;
        czas = std::chrono::system_clock::now();
    }
};

struct OdliczanieCzasowZwloki
{
    std::chrono::system_clock::time_point czas_zal;
    std::chrono::system_clock::time_point czas_wyl;
    const std::chrono::duration<int> ile_sekund_zal;
    const std::chrono::duration<int> ile_sekund_wyl;
    bool czy_odliczanie_zal, czy_ustawiono_zal;
    bool czy_odliczanie_wyl, czy_ustawiono_wyl;

    OdliczanieCzasowZwloki(int czas_odliczania_zal, int czas_odliczania_wyl)
        : czas_zal(std::chrono::system_clock::now()),
          czas_wyl(std::chrono::system_clock::now()),
          ile_sekund_zal(czas_odliczania_zal),
          ile_sekund_wyl(czas_odliczania_wyl),
          czy_odliczanie_zal(false),
          czy_ustawiono_zal(false),
          czy_odliczanie_wyl(false),
          czy_ustawiono_wyl(false)
    {
    }

    void UstawCzasZalJesliNieUstawiono()
    {
        if (czy_ustawiono_zal == false && czy_odliczanie_zal == false)
        {
            czas_zal = std::chrono::system_clock::now();
            czy_odliczanie_zal = true;
        }
    }

    void UstawCzasWylJesliNieUstawiono()
    {
        if (czy_ustawiono_wyl == false && czy_odliczanie_wyl == false)
        {
            czas_wyl = std::chrono::system_clock::now();
            czy_odliczanie_wyl = true;
        }
    }

    bool CzyCzasZalUplynol(std::chrono::system_clock::time_point &teraz)
    {
        using namespace std::chrono;
        if (czy_ustawiono_zal == false)
        {
            if (duration_cast<seconds>(teraz - czas_zal).count() >= ile_sekund_zal.count())
            {
                if (DEBUG)
                    std::cout << "\tCzas załączenia minął!\n";

                czy_odliczanie_zal = false;
                czy_ustawiono_zal = true;
                return true;
            }

            return false;
        }

        return false;
    }

    bool CzyCzasWylUplynol(std::chrono::system_clock::time_point &teraz)
    {
        using namespace std::chrono;
        if (czy_ustawiono_wyl == false)
        {
            if (duration_cast<seconds>(teraz - czas_wyl).count() >= ile_sekund_wyl.count())
            {
                if (DEBUG)
                    std::cout << "\tCzas wyłączenia minął!\n";

                czy_odliczanie_wyl = false;
                czy_ustawiono_wyl = true;
                return true;
            }

            return false;
        }

        return false;
    }

    void ResetZal()
    {
        czy_ustawiono_zal = false;
        czy_odliczanie_zal = false;
        czas_zal = std::chrono::system_clock::now();
    }

    void ResetWyl()
    {
        czy_ustawiono_wyl = false;
        czy_odliczanie_wyl = false;
        czas_wyl = std::chrono::system_clock::now();
    }
};

float GetTempBaza(MYSQL *con, int id_czujnika);

bool GetWyjscieBaza(MYSQL *con, int id_wyjscia);

void SetWyjscieBaza(MYSQL *con, int id_wyjscia, bool stan);

bool GetWejscieBaza(MYSQL *con, int id_wejscia);

bool CzyWPrzedzialeCzasu(std::chrono::system_clock::time_point &now, std::tm *m_akt_od, std::tm *m_akt_do);