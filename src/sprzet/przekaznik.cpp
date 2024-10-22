#include <math.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#include "sprzet/przekaznik.hh"
#include "konfiguracja.hh"
#include "sprzet/tca.hh"
#include "komunikacja/baza.hh"

Przekaznik::Przekaznik()
{
    m_adres = -1;
    m_linia = -1;
    m_uchwyt = -1;
    m_aktywny = false;
}

Przekaznik::Przekaznik(int adres, int linia)
{
    m_adres = adres;
    m_linia = linia;
    m_uchwyt = -1;
    m_aktywny = false;
}

void Przekaznik::Blokuj(const char *komunikat)
{
    std::cerr << komunikat << " Wyłączanie przekaźnika " 
    << std::hex << m_adres << std::dec << " na lini " << m_linia << "." << std::endl;

    m_aktywny = false;    
}

bool Przekaznik::Inicjuj(Tca &tca)
{
    tca.ZmienLinie(m_linia);
    m_uchwyt = wiringPiI2CSetup(m_adres);

    if(m_uchwyt < 0){
        Blokuj("Błąd inicjalizacji uchwytu.");
        return false;
    }

    wiringPiI2CWriteReg8(m_uchwyt, IODIRA, 0);
    wiringPiI2CWriteReg8(m_uchwyt, IODIRB, 0);

    m_aktywny = true;
    AktualizujPiny(tca);

    return true;
}

bool Przekaznik::Inicjuj(Tca &tca, int adres, int linia)
{
    m_adres = adres;
    m_linia = linia;

    tca.ZmienLinie(m_linia);
    m_uchwyt = wiringPiI2CSetup(m_adres);

    if (m_uchwyt < 0)
    {
        Blokuj("Błąd inicjalizacji uchwytu.");
        return false;
    }

    wiringPiI2CWriteReg8(m_uchwyt, IODIRA, 0);
    wiringPiI2CWriteReg8(m_uchwyt, IODIRB, 0);

    m_aktywny = true;
    AktualizujPiny(tca);

    return true;
}

bool Przekaznik::OdczytReczne(int nr_pinu) const
{
    return m_recznie[nr_pinu];
}

void Przekaznik::UstawReczne(int nr_pinu, bool wartosc)
{
    m_recznie[nr_pinu] = wartosc;
}

bool Przekaznik::OdczytPin(int nr_pinu) const
{
    return m_wartosc[nr_pinu];
}

void Przekaznik::UstawPin(int nr_pinu, bool wartosc)
{
    if(OdczytReczne(nr_pinu) == false)
        m_wartosc[nr_pinu] = wartosc;
}

void Przekaznik::AktualizujPiny(Tca &tca)
{
    int i, temp;

    if (m_aktywny == true)
    {
        tca.ZmienLinie(m_linia);

        for (i = 0, temp = 0; i < 8; ++i)
            if (OdczytPin(i) == true)
                temp += pow(2, i);

        wiringPiI2CWriteReg8(m_uchwyt, GPIOA, temp);

        for (i = 8, temp = 0; i < 16; ++i)
            if (OdczytPin(i) == true)
                temp += pow(2, i - 8);

        wiringPiI2CWriteReg8(m_uchwyt, GPIOB, temp);
    }
}

void Przekaznik::AktualizujPinOrazReczne(Tca &tca, int nr_pinu, bool wartosc_pin, bool wartosc_recznie)
{
    if (OdczytPin(nr_pinu) != wartosc_pin || OdczytReczne(nr_pinu) != wartosc_recznie)
    {
        UstawReczne(nr_pinu, false);
        UstawPin(nr_pinu, wartosc_pin);
        if(wartosc_recznie != false)
            UstawReczne(nr_pinu, true);
        AktualizujPiny(tca);
    }
}

std::ostream &operator<<(std::ostream &strm, const Przekaznik &P){
    strm << "Przekaźnik("
         << "Adres: " << std::hex << P.m_adres << std::dec
         << ", Linia: " << P.m_linia
         << ", Uchwyt: " << P.m_uchwyt
         << ", Wartości: " << P.m_wartosc
         << ", Ręczne: " << P.m_recznie
         << ", Aktywny: " << P.m_aktywny
         << ")" << std::endl;

    return strm;
}

std::ostream &operator<<(std::ostream &strm, const std::vector<Przekaznik> &P)
{
    for (unsigned int i = 0; i < P.size(); ++i)
        strm << "ID(" << i << ") " << P[i];

    return strm;
}

std::vector<Przekaznik> StartPrzekazniki(MYSQL *con, Tca &tca){
    std::vector<Przekaznik> przekazniki;
    MYSQL_RES *wynik_zapytania;     //struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania;     //struktura na wiersz tabeli  

    wynik_zapytania = PobierzBaza(con, "SELECT idPlytki_Wyjscia_On_Off, linia, adres, aktywny FROM Plytki_Wyjscia_On_Off"); //odbierz wyniki zapytania o przekazniki

    while ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) //dopoki sa wiersze w tabeli
    {
        if ((unsigned)atoi(wiersz_zapytania[0]) != przekazniki.size()) //jezeli indeksy w bazie są nieprawidłowe, przerwij
        { 
            mysql_free_result(wynik_zapytania);
            throw(std::runtime_error("Blad indeksow w tabeli przekaznikow"));
        }
        Przekaznik temp(atoi(wiersz_zapytania[2]), atoi(wiersz_zapytania[1]));

        if (atoi(wiersz_zapytania[3]) == 1) //sprawdź czy aktywny w bazie, jak tak to zainicjuj go
            temp.Inicjuj(tca);

        przekazniki.push_back(temp);
    }

    mysql_free_result(wynik_zapytania); ///zwolnij wynik zapytania pinow
    
    AktualizujBaza(con, "UPDATE Piny_Wyjscia_On_Off SET wartosc = 0 WHERE recznie = 0");
    AktualizujPrzekazniki(con, tca, przekazniki);

    return przekazniki; //zwroc tablice wczytanych przekaznikow
}

void AktualizujPrzekazniki(MYSQL *con, Tca &tca, std::vector<Przekaznik> &przekazniki)
{
    MYSQL_RES *wynik_zapytania; //struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; //struktura na wiersz tabeli "Przekazniki"

    for (unsigned int i = 0; i < przekazniki.size(); ++i)
    {
        std::string zapytanie_sql = "SELECT pin, wartosc, recznie FROM Piny_Wyjscia_On_Off WHERE idPlytki_Wyjscia_On_Off = ";
        zapytanie_sql.append(std::to_string(i));
        wynik_zapytania = PobierzBaza(con, zapytanie_sql.c_str()); //pobierz poczatkowe stany pinow obslugiwanego przekaznika ktore sa w trybie recznym

        while ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania)))
            przekazniki[i].AktualizujPinOrazReczne(tca, atoi(wiersz_zapytania[0]), atoi(wiersz_zapytania[1]) > 0 ? TRUE : FALSE, atoi(wiersz_zapytania[2]));

        mysql_free_result(wynik_zapytania); ///zwolnij wynik zapytania pinow
    }
}

void ZakonczPrzekazniki(MYSQL *con, Tca &tca, std::vector<Przekaznik> &przekazniki)
{

    for (unsigned int i = 0; i < przekazniki.size(); ++i) // wyłączenie wszystkich przekazników
    {
        for (int j = 0; j < 16; ++j)
        {
            przekazniki[i].UstawReczne(j, 0);
            przekazniki[i].UstawPin(j, 0);
        }
        przekazniki[i].AktualizujPiny(tca);
    }

    AktualizujBaza(con, "UPDATE Piny_Wyjscia_On_Off SET wartosc = 0 WHERE recznie = 0"); //wyzeruj aktywne piny automatyczne w bazie

    przekazniki.clear(); //czyszczenie tablicy przekaznikow
}