#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <string>

#include "sprzet/czujniki_ruchu.hh"
#include "konfiguracja.hh"
#include "sprzet/tca.hh"
#include "komunikacja/baza.hh"

CzujnikiRuchu::CzujnikiRuchu(int adres, int linia)
{
    m_adres = adres;
    m_linia = linia;
    m_uchwyt = -1;
    m_aktywny = false;
}

void CzujnikiRuchu::Blokuj(std::string komunikat)
{
    std::cerr << komunikat << " Wyłączanie czujników "
              << std::hex << m_adres << std::dec << " na lini " << m_linia << "." << std::endl;

    m_aktywny = false;
}

bool CzujnikiRuchu::Inicjuj(Tca &tca)
{
    tca.ZmienLinie(m_linia);

    m_uchwyt = wiringPiI2CSetup(m_adres); //inicjacja przekaznika
    if (m_uchwyt < 0)
    {
        Blokuj("Błąd inicjalizacji uchwytu.");
        return false;
    }

    wiringPiI2CWriteReg8(m_uchwyt, IODIRA, 255); //ustawienie kierunku na INPUT
    wiringPiI2CWriteReg8(m_uchwyt, IODIRB, 255);

    m_aktywne_piny.flip(); //aktywuj wszystkie czujniki
    m_aktywny = true;
    return true;
}

bool CzujnikiRuchu::OdczytAktywnyPin(int nr_pinu) const
{
    return m_aktywne_piny[nr_pinu];
}

void CzujnikiRuchu::UstawAktywnyPin(int nr_pinu, bool wartosc)
{
    m_aktywne_piny[nr_pinu] = wartosc;
}

bool CzujnikiRuchu::OdczytPolaryzacjaPin(int nr_pinu) const
{
    return m_polaryzacja[nr_pinu];
}

void CzujnikiRuchu::UstawPolaryzacjaPin(int nr_pinu, bool wartosc)
{
    m_polaryzacja[nr_pinu] = wartosc;
}

bool CzujnikiRuchu::OdczytCzyDziala(int nr_pinu) const
{
    return m_czy_dziala[nr_pinu];
}

const bitset<16> &CzujnikiRuchu::OdczytCzyDziala() const
{
    return m_czy_dziala;
}

void CzujnikiRuchu::UstawCzyDziala(bitset<16> &czy_dziala)
{
    m_czy_dziala = czy_dziala;
}

bitset<16> CzujnikiRuchu::SprawdzRuch(Tca &tca) const
{
    bitset<16> ruch;
    int odczyt;

    if (m_aktywny == false)
    {
        return (*this).m_czy_dziala;
    }

    tca.ZmienLinie(m_linia);

    odczyt = wiringPiI2CReadReg8(m_uchwyt, GPIOB);
    if (odczyt >= 0)
    {
        for (int i = 0; i < 8; ++i)
        {
            ruch[7 - i] = ((odczyt >> i) & 0x01);
        }
    }
    else
    {
        SendData((std::string("Błąd odczytu czujników ruchu GPIOB (0-7) adres ") + std::to_string(m_adres)).c_str());
    }

    if(DEBUG_RUCH) std::cerr << "OdcB: " << bitset<8>(odczyt) << "\n";

    odczyt = wiringPiI2CReadReg8(m_uchwyt, GPIOA);
    if (odczyt >= 0)
    {
        for (int i = 0; i < 8; ++i)
        {
            ruch[8 + i] = ((odczyt >> i) & 0x01);
        }
    }
    else
    {
        SendData((std::string("Błąd odczytu czujników ruchu GPIOA (8-15) adres ") + std::to_string(m_adres)).c_str());
    }

    if(DEBUG_RUCH) std::cerr << "OdcA: " << bitset<8>(odczyt) << "\n";
    
    if(DEBUG_RUCH) std::cerr << "Ruch: " << ruch << "\n";
    if(DEBUG_RUCH) std::cerr << "Pol^: " << m_polaryzacja << "\n";
    if(DEBUG_RUCH) std::cerr << "Akt&: " << m_aktywne_piny << "\n";

    ruch = (ruch ^ m_polaryzacja) & m_aktywne_piny;

    if(DEBUG_RUCH) std::cerr << "Ruch: " << ruch << "\n";
    if(DEBUG_RUCH) std::cerr << "CzD^: " << m_czy_dziala << "\n";

    return ruch ^ (*this).m_czy_dziala;
}

std::ostream &operator<<(std::ostream &strm, const CzujnikiRuchu &C)
{
    strm << "Czujnik Ruchu("
         << "Adres: " << std::hex << C.m_adres << std::dec
         << ", Linia: " << C.m_linia
         << ", Uchwyt: " << C.m_uchwyt
         << ", Aktywne piny: " << C.m_aktywne_piny
         << ", Polaryzacja piny: " << C.m_polaryzacja
         << ", Aktywny: " << C.m_aktywny
         << ", Czy_dziala: " << C.m_czy_dziala
         << ")" << std::endl;

    return strm;
}

std::ostream &operator<<(std::ostream &strm, const std::vector<CzujnikiRuchu> &C)
{
    for (unsigned int i = 0; i < C.size(); ++i)
        strm << "ID(" << i << ") " << C[i];

    return strm;
}

std::vector<CzujnikiRuchu> StartCzujnikiRuchu(MYSQL *con, Tca &tca)
{
    std::vector<CzujnikiRuchu> ruch;
    MYSQL_RES *wynik_zapytania;
    MYSQL_ROW wiersz_zapytania;

    wynik_zapytania = PobierzBaza(con, "SELECT idPlytki_Wejscia_On_Off, linia, adres, aktywny FROM Plytki_Wejscia_On_Off"); //odbierz wyniki zapytania o przekazniki

    while ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) //dopoki sa wiersze w tabeli
    {
        if ((unsigned)atoi(wiersz_zapytania[0]) != ruch.size()) //jezeli indeksy w bazie są nieprawidłowe, przerwij
        {
            mysql_free_result(wynik_zapytania);
            throw(std::runtime_error("Blad indeksow w tabeli czujników ruchu"));
        }
        CzujnikiRuchu temp(atoi(wiersz_zapytania[2]), atoi(wiersz_zapytania[1]));

        if (atoi(wiersz_zapytania[3]) == 1) //sprawdź czy aktywny w bazie, jak tak to zainicjuj go
            temp.Inicjuj(tca);

        ruch.push_back(temp);
    }

    mysql_free_result(wynik_zapytania); ///zwolnij wynik zapytania pinow

    AktualizujBaza(con, "UPDATE Piny_Wejscia_On_Off SET czy_teraz_aktywny = 0");
    AktualizujFlagiPinyCzujnikiRuchu(con, tca, ruch);

    return ruch;
}

void AktualizujFlagiPinyCzujnikiRuchu(MYSQL *con, Tca &tca, std::vector<CzujnikiRuchu> &ruch)
{
    MYSQL_RES *wynik_zapytania; //struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; //struktura na wiersz tabeli "Przekazniki"
    int nr_pinu, polaryzacja, aktywny;

    for (unsigned int i = 0; i < ruch.size(); ++i)
    {
        std::string zapytanie_sql = "SELECT pin, polaryzacja, aktywny FROM Piny_Wejscia_On_Off WHERE idPlytki_Wejscia_On_Off = ";
        zapytanie_sql.append(std::to_string(i));
        wynik_zapytania = PobierzBaza(con, zapytanie_sql.c_str()); //pobierz poczatkowe stany pinow obslugiwanego przekaznika ktore sa w trybie recznym

        while ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania)))
        {
            nr_pinu = atoi(wiersz_zapytania[0]);
            polaryzacja = atoi(wiersz_zapytania[1]);
            aktywny = atoi(wiersz_zapytania[2]);

            if (ruch[i].OdczytPolaryzacjaPin(nr_pinu) != polaryzacja)
                ruch[i].UstawPolaryzacjaPin(nr_pinu, polaryzacja);

            if (ruch[i].OdczytAktywnyPin(nr_pinu) != aktywny)
                ruch[i].UstawAktywnyPin(nr_pinu, aktywny);
        }

        mysql_free_result(wynik_zapytania); ///zwolnij wynik zapytania pinow
    }
}

void AktualizujRuch(MYSQL *con, Tca &tca, std::vector<CzujnikiRuchu> &ruch)
{
    const char *cz1 = "UPDATE Piny_Wejscia_On_Off SET czy_teraz_aktywny=";
    const char *cz2 = " WHERE idPiny_Wejscia_On_Off=";
    bitset<16> temp;

    for (unsigned int i = 0; i < ruch.size(); ++i)
    {
        if(DEBUG_RUCH) std::cerr << "=== Czujnik " << i << " ===\n";
        temp = ruch[i].SprawdzRuch(tca);
        if(DEBUG_RUCH) std::cerr << "Temp: " << temp << "\n";
        if(DEBUG_RUCH) std::cerr << "=== END ===\n\n";
        for (unsigned int j = 0; j < 16; ++j)
        {
            if(temp[j]==true){
                std::string temp = "Ruch na czujniku " + std::to_string(16 * i + j);
                SendData(temp.c_str());
                std::string zapytanie_sql = cz1 + std::to_string(!ruch[i].OdczytCzyDziala(j)) + cz2 + std::to_string(16*i + j);
                AktualizujBaza(con, zapytanie_sql.c_str());
            }
        }
        temp ^= ruch[i].OdczytCzyDziala();
        ruch[i].UstawCzyDziala(temp);
    }
}