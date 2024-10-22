#include <string.h>
#include <stdlib.h>

#include "programy/zdarzenia_ruchu/zdarzenie_ruchu.hh"
#include "konfiguracja.hh"
#include "komunikacja/baza.hh"

ZdarzenieRuchu::ZdarzenieRuchu()
{
    m_prog = 0;
    m_czas_dzialania = 0;
    m_czy_dziala = false;
    m_aktywny = false;
    m_tryb_pracy = TRYB_PRACY::BEZ_CZASU_DATY;
}

bool ZdarzenieRuchu::Inicjuj(int prog, int czas_dzialania, struct tm &akt_od, struct tm &akt_do, TRYB_PRACY tryb_pracy)
{
    if (prog < 0 || prog > 10)
        return false;

    m_prog = prog;
    m_czas_dzialania = czas_dzialania;
    m_akt_od = akt_od;
    m_akt_do = akt_do;
    m_aktywny = true;
    m_tryb_pracy = tryb_pracy;

    return true;
}

void ZdarzenieRuchu::Blokuj(const char *komunikat)
{
    std::cerr << komunikat << ". Blokada zdarzenia." << std::endl;
    m_aktywny = false;
}

bool ZdarzenieRuchu::OdczytCzyDziala() const
{
    return m_czy_dziala;
}

TRYB_PRACY ZdarzenieRuchu::OdczytTrybPracy() const
{
    return m_tryb_pracy;
}

void ZdarzenieRuchu::UstawCzyDziala(bool czy_dziala)
{
    m_czy_dziala = czy_dziala;
}

void ZdarzenieRuchu::UstawCzyDzialaIWBazie(MYSQL *con, bool czy_dziala, int id_zdarzenia)
{
    const char *str1 = "UPDATE Zdarzenia_Ruchu SET czy_dziala = ";
    const char *str2 = " WHERE idZdarzenia_Ruchu = ";

    std::string zapytanie_sql;
    zapytanie_sql = str1 + std::to_string(czy_dziala) + str2 + std::to_string(id_zdarzenia);

    UstawCzyDziala(czy_dziala);
    AktualizujBaza(con, zapytanie_sql.c_str());
}

void ZdarzenieRuchu::UstawCzasAktywacji(time_t &teraz)
{
    m_czas_aktywacji = teraz;
}

bool ZdarzenieRuchu::CzyCzasMinal(time_t &teraz) const
{
    if (m_tryb_pracy == TRYB_PRACY::AKT_GDY_CZAS)
    {
        return false;
    }

    if (difftime(teraz, m_czas_aktywacji) >= m_czas_dzialania)
    {
        return true;
    }

    return false;
}

bool ZdarzenieRuchu::CzyWPrzedzialeProgu(int prog) const
{
    if (prog <= m_prog || m_prog == 0)
        return true;

    return false;
}

bool ZdarzenieRuchu::CzyWPrzedzialeCzasu(struct tm *teraz) const
{
    if (m_tryb_pracy == TRYB_PRACY::BEZ_CZASU_DATY)
        return true;
    //
    //TO DO: Dodać obsługę gdy różnica zadanych czasów jest mniejsza od godziny
    //
    if (m_tryb_pracy == TRYB_PRACY::AKT_GDY_CZAS || m_tryb_pracy == TRYB_PRACY::AKT_GDY_CZAS_I_RUCH)
    {
        if (m_akt_od.tm_hour < m_akt_do.tm_hour)    //Gdy godziny nie przekraczają przez północ - część wzpólna zbiorów
        {
            if (teraz->tm_hour > m_akt_od.tm_hour && teraz->tm_hour < m_akt_do.tm_hour) // Godzina większa od min i mniejsza od max
            {
                return true;
            }
            else if (teraz->tm_hour == m_akt_od.tm_hour) // Godzina równa min
            {
                if (teraz->tm_min > m_akt_od.tm_min) // Minuta większa od min
                    return true;
                else if (teraz->tm_min == m_akt_od.tm_min) //Minuta równa min
                {
                    if (teraz->tm_sec >= m_akt_od.tm_sec) // Sekunda większa lub równa min
                        return true;
                }
            }
            else if (teraz->tm_hour == m_akt_do.tm_hour) // Godzina równa max
            {
                if (teraz->tm_min < m_akt_do.tm_min) // Minuta mniejsza od max
                    return true;
                else if (teraz->tm_min == m_akt_do.tm_min) // Minuta równa max
                {
                    if (teraz->tm_sec <= m_akt_od.tm_sec) // Sekunda mniejsza lub równa max
                        return true;
                }
            }
        }
        else //Gdy godziny przekraczają przez północ - suma zbiorów
        {
            if (teraz->tm_hour > m_akt_od.tm_hour || teraz->tm_hour < m_akt_do.tm_hour) // Godzina większa od min i mniejsza od max
            {
                return true;
            }
            else if (teraz->tm_hour == m_akt_od.tm_hour) // Godzina równa min
            {
                if (teraz->tm_min > m_akt_od.tm_min) // Minuta większa od min
                    return true;
                else if (teraz->tm_min == m_akt_od.tm_min) //Minuta równa min
                {
                    if (teraz->tm_sec >= m_akt_od.tm_sec) // Sekunda większa lub równa min
                        return true;
                }
            }
            else if (teraz->tm_hour == m_akt_do.tm_hour) // Godzina równa max
            {
                if (teraz->tm_min < m_akt_do.tm_min) // Minuta mniejsza od max
                    return true;
                else if (teraz->tm_min == m_akt_do.tm_min) // Minuta równa max
                {
                    if (teraz->tm_sec <= m_akt_od.tm_sec) // Sekunda mniejsza lub równa max
                        return true;
                }
            }
        }

            
    }

    return false;
}

std::ostream &operator<<(std::ostream &strm, const TRYB_PRACY &T)
{
    switch (T)
    {
    case TRYB_PRACY::BEZ_CZASU_DATY:
        strm << "BEZ_CZASU_DATY";
        break;

    case TRYB_PRACY::AKT_GDY_CZAS:
        strm << "AKT_GDY_CZAS";
        break;

    case TRYB_PRACY::AKT_GDY_CZAS_I_RUCH:
        strm << "AKT_GDY_CZAS_I_RUCH";
        break;

    default:
        break;
    }

    return strm;
}

std::ostream & operator<<(std::ostream &strm, const ZdarzenieRuchu &Z)
{
    strm << "Zdarzenie Ruchu("
         << "Prog: " << Z.m_prog
         << ", Czas działania: " << Z.m_czas_dzialania
         << ", Czas aktywacji: " << ctime(&Z.m_czas_aktywacji)
         << ", Aktywny od: " << asctime(&Z.m_akt_od)
         << ", Aktywny od: " << asctime(&Z.m_akt_do)
         << ", Czy działa: " << Z.m_czy_dziala
         << ", Tryb pracy: " << Z.m_tryb_pracy
         << ", Aktywny: " << Z.m_aktywny
                      << ")" << std::endl;

    return strm;
}

std::ostream &operator<<(std::ostream &strm, const std::vector<ZdarzenieRuchu> &Z)
{
    for (unsigned int i = 0; i < Z.size(); ++i)
        strm << "ID(" << i << ") " << Z[i];

    return strm;
}

std::vector<ZdarzenieRuchu> StartZdarzeniaRuchu(MYSQL *con)
{
    std::vector<ZdarzenieRuchu> z_ruch;
    MYSQL_RES *wynik_zapytania;
    MYSQL_ROW wiersz_zapytania;

    wynik_zapytania = PobierzBaza(con, "SELECT idZdarzenia_Ruchu, prog_jasnosci, czas_dzialania, aktywny_od, aktywny_do, aktywny, idPiny_Wejscia_On_Off FROM Zdarzenia_Ruchu");

    while ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) //dopoki sa wiersze w tabeli
    {
        if ((unsigned)atoi(wiersz_zapytania[0]) != z_ruch.size()) //jezeli indeksy w bazie są nieprawidłowe, przerwij
        {
            mysql_free_result(wynik_zapytania);
            throw(std::runtime_error("Blad indeksow w tabeli zdarzeń ruchu"));
        }

        ZdarzenieRuchu temp;
        struct tm czas_od, czas_do;

        memset(&czas_od, 0, sizeof(struct tm)); //zerowanie struktury
        memset(&czas_do, 0, sizeof(struct tm));

        strptime(wiersz_zapytania[3], "%Y-%m-%d %H:%M:%S", &czas_od);
        strptime(wiersz_zapytania[4], "%Y-%m-%d %H:%M:%S", &czas_do);

        if (atoi(wiersz_zapytania[5]) == 1) //sprawdź czy aktywny w bazie, jak tak to zainicjuj go
        {
            if (atoi(wiersz_zapytania[6]) == -1)
                temp.Inicjuj(atoi(wiersz_zapytania[1]), atoi(wiersz_zapytania[2]), czas_od, czas_do, TRYB_PRACY::AKT_GDY_CZAS);
            else
                temp.Inicjuj(atoi(wiersz_zapytania[1]), atoi(wiersz_zapytania[2]), czas_od, czas_do, TRYB_PRACY::BEZ_CZASU_DATY);

            std::cout << (atoi(wiersz_zapytania[6]) == -1) << ' ' << wiersz_zapytania[6] << "atoi: " << atoi(wiersz_zapytania[6]) << std::endl;
        }
        z_ruch.push_back(temp);
    }

    mysql_free_result(wynik_zapytania); ///zwolnij wynik zapytania pinow

    AktualizujBaza(con, "UPDATE Zdarzenia_Ruchu SET czy_dziala = 0");

    return z_ruch;
}

void AktywujZdarzeniaRuchu(MYSQL *con[], int index, int idPinRuchu, int prog_jasnosci, std::vector<ZdarzenieRuchu> &z_ruch)
{
    MYSQL_RES *wynik_zapytania;
    MYSQL_ROW wiersz_zapytania;
    int idZdarzenia;
    time_t teraz;
    time(&teraz);
    struct tm *data = localtime(&teraz);

    std::string zapytanie_sql = "SELECT idZdarzenia_Ruchu, idPiny_Wyjscia_On_Off, idUrzadzenia_Wyjscia FROM Zdarzenia_Ruchu WHERE aktywny=1 AND idPiny_Wejscia_On_Off=";
    zapytanie_sql += std::to_string(idPinRuchu);

    wynik_zapytania = PobierzBaza(con[index], zapytanie_sql.c_str());

    while ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) //dopoki sa wiersze w tabeli
    {
        idZdarzenia = atoi(wiersz_zapytania[0]);

        //sprawdzenie warunków przed włączeniem lub aktualizacją
        if (z_ruch[idZdarzenia].CzyWPrzedzialeProgu(prog_jasnosci))
        {
            //if(DEBUG) std::cerr << "W przedziale progu." << std::endl;
            if (z_ruch[idZdarzenia].CzyWPrzedzialeCzasu(data))
            {
                //if(DEBUG) std::cerr << "W przedziale czasu." << std::endl;
                //włączenie zdarzenia
                if (z_ruch[idZdarzenia].OdczytCzyDziala() == false)
                {
                    //jeżeli tylko połączenie z tą bazą jest aktywne
                    if (DEBUG)
                        std::cerr << "Aktywuj zdarzenie " << idZdarzenia << std::endl;

                    ZmienPinPrzekaznikaWBazie(con[atoi(wiersz_zapytania[2])], atoi(wiersz_zapytania[1]), true);
                    if (SprawdzPinPrzekaznikaWBazie(con[atoi(wiersz_zapytania[2])], atoi(wiersz_zapytania[1]))) // sprawdz czy pin prawidłowo ustawiony w bazie
                    {
                        z_ruch[idZdarzenia].UstawCzyDzialaIWBazie(con[index], true, idZdarzenia);
                        z_ruch[idZdarzenia].UstawCzasAktywacji(teraz);
                    }
                }
                //aktualizacja zdarzenia
                else
                {
                    z_ruch[idZdarzenia].UstawCzasAktywacji(teraz);
                }
            }
        }
    }
    mysql_free_result(wynik_zapytania); //zwolnij wynik zapytania
}

void AktywujZdarzeniaCzasu(MYSQL *con[], int index, int prog_jasnosci, std::vector<ZdarzenieRuchu> &z_ruch)
{
    MYSQL_RES *wynik_zapytania;
    MYSQL_ROW wiersz_zapytania;
    int idZdarzenia;
    time_t teraz;
    time(&teraz);
    struct tm *data = localtime(&teraz);

    std::string zapytanie_sql = "SELECT idZdarzenia_Ruchu, idPiny_Wyjscia_On_Off, idUrzadzenia_Wyjscia FROM Zdarzenia_Ruchu WHERE aktywny = 1 AND idPiny_Wejscia_On_Off = -1";

    wynik_zapytania = PobierzBaza(con[index], zapytanie_sql.c_str());

    while ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) //dopoki sa wiersze w tabeli
    {
        idZdarzenia = atoi(wiersz_zapytania[0]);

        //sprawdzenie warunków przed włączeniem lub aktualizacją
        if (z_ruch[idZdarzenia].CzyWPrzedzialeProgu(prog_jasnosci))
        {
            //if(DEBUG) std::cerr << "W przedziale progu." << std::endl;
            if (z_ruch[idZdarzenia].CzyWPrzedzialeCzasu(data))
            {
                //if(DEBUG) std::cerr << "W przedziale czasu." << std::endl;
                //włączenie zdarzenia
                if (z_ruch[idZdarzenia].OdczytCzyDziala() == false)
                {
                    //jeżeli tylko połączenie z tą bazą jest aktywne
                    if (DEBUG)
                        std::cerr << "Aktywuj zdarzenie " << idZdarzenia << std::endl;

                    
                    ZmienPinPrzekaznikaWBazie(con[atoi(wiersz_zapytania[2])], atoi(wiersz_zapytania[1]), true);
                    if (SprawdzPinPrzekaznikaWBazie(con[atoi(wiersz_zapytania[2])], atoi(wiersz_zapytania[1]))) // sprawdz czy pin prawidłowo ustawiony w bazie
                        z_ruch[idZdarzenia].UstawCzyDzialaIWBazie(con[index], true, idZdarzenia);
                }
            }
        }
    }
    mysql_free_result(wynik_zapytania); //zwolnij wynik zapytania
}

void DezaktywujZdarzenieRuchu(MYSQL *con[], int index, int idZdarzenia, ZdarzenieRuchu &z_ruch)
{
    MYSQL_RES *wynik_zapytania;
    MYSQL_ROW wiersz_zapytania;
    int idPin;
    int idUrzadzenie;

    std::string zapytanie_sql = "SELECT idPiny_Wyjscia_On_Off, idUrzadzenia_Wyjscia FROM Zdarzenia_Ruchu WHERE idZdarzenia_Ruchu=";
    zapytanie_sql += std::to_string(idZdarzenia);

    wynik_zapytania = PobierzBaza(con[index], zapytanie_sql.c_str());
    wiersz_zapytania = mysql_fetch_row(wynik_zapytania);

    idPin = atoi(wiersz_zapytania[0]);
    idUrzadzenie = atoi(wiersz_zapytania[1]);

    mysql_free_result(wynik_zapytania); //zwolnij wynik zapytania

    z_ruch.UstawCzyDzialaIWBazie(con[index], false, idZdarzenia);

    //odejmij jeden od wartosci dla pinu wejsc
    ZmienPinPrzekaznikaWBazie(con[idUrzadzenie], idPin, false);
}

void AktualizujZdarzeniaRuchu(MYSQL *con[], std::vector<ZdarzenieRuchu> &z_ruch, int nr_bazy_serwerowni)
{
    MYSQL_RES *wynik_zapytania;
    MYSQL_ROW wiersz_zapytania;
    time_t teraz;
    struct tm *data;
    int prog_jasnosci = PobierzJasnoscZBazy(con[nr_bazy_serwerowni]);

    //Pobierz czujniki ktore sa aktywne w bazie na tym urzadzeniu
    wynik_zapytania = PobierzBaza(con[0], "SELECT idPiny_Wejscia_On_Off FROM Piny_Wejscia_On_Off WHERE czy_teraz_aktywny = 1");

    while ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) //dopoki sa wiersze w tabeli
    {
        if (DEBUG)
            std::cerr << "Aktywuj zdarzenia od czujnika ruchu id " << atoi(wiersz_zapytania[0]) << std::endl;

        AktywujZdarzeniaRuchu(con, 0, atoi(wiersz_zapytania[0]), prog_jasnosci, z_ruch);
    }

    mysql_free_result(wynik_zapytania); //zwolnij wynik zapytania

    //aktywuj wszystkie zdarzenia czasu
    AktywujZdarzeniaCzasu(con, 0, prog_jasnosci, z_ruch);

    time(&teraz);
    data = localtime(&teraz);

    for (unsigned int i = 0; i < z_ruch.size(); ++i)
    {
        //jeśli czas minął lub zdarzenie jest poza czasem aktywności, wyłącz je
        if (z_ruch[i].OdczytCzyDziala() && (z_ruch[i].CzyCzasMinal(teraz) || !z_ruch[i].CzyWPrzedzialeCzasu(data)))
        {
            if (DEBUG)
                std::cerr << "Dezaktywuj zdarzenie " << i << std::endl;
            DezaktywujZdarzenieRuchu(con, 0, i, z_ruch[i]);
        }
    }
}

int PobierzJasnoscZBazy(MYSQL *con)
{
    MYSQL_RES *wynik_zapytania; //struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; //struktura na wiersz tabeli
    int jasnosc;

    wynik_zapytania = PobierzBaza(con, "SELECT wartosc FROM Czujniki_Jasnosci WHERE idCzujniki_Jasnosci = 0");

    wiersz_zapytania = mysql_fetch_row(wynik_zapytania);

    jasnosc = atoi(wiersz_zapytania[0]);

    mysql_free_result(wynik_zapytania);
    return jasnosc;
}

void ZmienPinPrzekaznikaWBazie(MYSQL *con, int idPin_Przekaznik, bool wartosc)
{
    std::string zapytanie_sql;
    const char *str1 = "UPDATE Piny_Wyjscia_On_Off SET wartosc = ";
    const char *str2 = "WHERE recznie=0 AND idPiny_Wyjscia_On_Off = ";

    zapytanie_sql = str1;

    if (wartosc)
        zapytanie_sql += "wartosc + 1 ";
    else
        zapytanie_sql += "CASE WHEN wartosc > 0 THEN wartosc - 1 ELSE 0 END ";

    zapytanie_sql += str2 + std::to_string(idPin_Przekaznik);

    AktualizujBaza(con, zapytanie_sql.c_str());
}

bool SprawdzPinPrzekaznikaWBazie(MYSQL *con, int idPin_Przekaznik)
{
    MYSQL_RES *wynik_zapytania; // struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; // struktura na wiersz tabeli
    int wartosc;
    std::string query = "SELECT wartosc FROM Piny_Wyjscia_On_Off WHERE idPiny_Wyjscia_On_Off = ";

    query.append(std::to_string(idPin_Przekaznik));
    wynik_zapytania = PobierzBaza(con, query.c_str());

    if ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) // jeżeli zwrócono wynik
    {
        wartosc = std::stoi(wiersz_zapytania[0]);
    }
    mysql_free_result(wynik_zapytania); /// zwolnij wynik zapytania

    return (wartosc > 0 ? true : false);
}