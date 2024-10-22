#include <iostream>
#include <mysql.h>
#include <cstring>

#include "konfiguracja.hh"
#include "komunikacja/baza.hh"

bool InicjujBaza(MYSQL **con, const std::string * dane)
{
    *con = mysql_init(nullptr);
    if (con == nullptr)
    {
        return false;
    }

    if (mysql_real_connect(*con, dane[0].c_str(), dane[1].c_str(), dane[2].c_str(), dane[3].c_str(), 0, nullptr, 0) == nullptr)
    {
        ZakonczBazaZBledem(con);
        return false;
    }

    return true;
}

bool InicjujBaza(MYSQL **con, const char *adres, const char *login, const char *haslo, const char *baza)
{
    *con = mysql_init(nullptr);
    if (*con == nullptr)
    {
        return false;
    }

    if (mysql_real_connect(*con, adres, login, haslo, baza, 0, nullptr, 0) == nullptr)
    {
        ZakonczBazaZBledem(con);
        return false;
    }

    return true;
}

void ZakonczBaza(MYSQL **con)
{
    if (*con != nullptr)
    {
        mysql_close(*con);
        *con = nullptr;
    }
}

void ZakonczBazaZBledem(MYSQL **con)
{
    if (*con != nullptr)
    {
        std::cerr << mysql_error(*con) << std::endl;
        mysql_close(*con);
        *con = nullptr;
    }
}

MYSQL_RES *PobierzBaza(MYSQL *con, const char *zapytanie)
{
    MYSQL_RES *wynik;

    if (mysql_query(con, zapytanie)){
        std::cerr << mysql_error(con) << std::endl;
        throw(SqlException("Błąd wysyłania zapytania pobierania wyników."));
    }

    wynik = mysql_store_result(con); //odbierz wynik zapytania

    if (wynik == nullptr){
        std::cerr << mysql_error(con) << std::endl;
        throw(SqlException("Błąd odbierania wyników zapytania."));
    }

    return wynik;
}

void AktualizujBaza(MYSQL *con, const char *zapytanie)
{
    if (mysql_query(con, zapytanie)){
        std::cerr << mysql_error(con) << std::endl;
        throw(SqlException("Błąd wysyłania zapytania aktualizacji."));
    }
}
