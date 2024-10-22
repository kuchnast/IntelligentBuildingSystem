#include <iostream>
#include <vector>
#include <mysql.h>
#include <sstream>

#include "konfiguracja.hh"
#include "komunikacja/baza.hh"
#include "sprzet/czujniki_temperatury.hh"
#include "komunikacja/komunikacja_z_arduino.hh"

using std::vector;

bool CzujnikTemperatury::InicjalizujCzujnik(SerialArduino & sa)
{
    std::string bufor;

    bufor.append("initDSid");
    bufor.append(std::to_string(m_id)); 
    bufor.append("pin");
    bufor.append(std::to_string(m_pin));
    bufor.append("address");
    bufor.append(m_adres);
    bufor.append("\n");

    if(!sa.Write(bufor))
        return false;
    
    if(!sa.Read(bufor))
        return false;

    return bufor.find("ok") != std::string::npos;
}

int CzujnikTemperatury::ZwrocId() const
{
    return m_id;
}

bool CzujnikTemperatury::AktualizujCzujnik(SerialArduino & sa)
{
    if(CzyBlad())
        return false;

    std::string bufor; 
    bufor.append("readDSid");
    bufor.append(std::to_string(m_id));
    bufor.append("\n");

    if(!sa.Write(bufor))
    {
        if(DEBUG_ARDUINO)
        {
            convertToASCII(bufor);
            std::cout << "AktualizujCzujnik: Write Error\n";
        }
        return false;
    }
        
    
    if(!sa.Read(bufor))
    {
        if(DEBUG_ARDUINO)
        {
            convertToASCII(bufor);
            std::cout << "AktualizujCzujnik: Read Error\n";
        }
        return false;
    }       

    auto pos = bufor.find("ok");
    if((pos == std::string::npos) || (bufor.find("id" + std::to_string(m_id))) == std::string::npos)
    {
        if(DEBUG_ARDUINO)
        {
            convertToASCII(bufor);
            std::cout << std::endl << "AktualizujCzujnik: Check Error (";
            std::cout << (pos == std::string::npos ? "No ok)\n" : "No id)\n");
        }
        return false;
    }

    try
    {
        this->m_temp = std::stof(bufor);
    }
    catch(...)
    {
        if(DEBUG_ARDUINO)
            std::cout << bufor.c_str() << std::endl << "AktualizujCzujnik: Stof Error\n";
        return false;
    }
    
    return true;
}

bool CzujnikTemperatury::CzyBlad() const
{
    return m_czy_blad;
}

bool CzujnikTemperatury::ZwiekszLicznikBledow()
{
    if(++m_liczba_bledow >= 10)
        return true;

    return false;
}

float CzujnikTemperatury::ZwrocTemperature() const
{
    return m_temp;
}

void CzujnikTemperatury::DezaktywujCzujnikWBazie(MYSQL *con)
{
    //std::string temp = "UPDATE Czujniki_Temperatury SET czy_blad = 1 WHERE idCzujniki_Temperatury = " + std::to_string(m_id);
    //AktualizujBaza(con, temp.c_str());
}

vector<CzujnikTemperatury> StartCzujnikiTemperatury(MYSQL *con, SerialArduino & sa)
{
    std::vector<CzujnikTemperatury> czujniki_temperatury;
    MYSQL_RES *wynik_zapytania; //struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; //struktura na wiersz tabeli


    AktualizujBaza(con, "UPDATE Czujniki_Temperatury SET czy_blad = 0 WHERE aktywny = 1"); //usun błędy w bazie dla aktywnych pinów

    wynik_zapytania = PobierzBaza(con, "SELECT idCzujniki_Temperatury, pin, adres FROM Czujniki_Temperatury WHERE aktywny = 1"); 

    while ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) //dopoki sa wiersze w tabeli
    {
        if(DEBUG_ARDUINO)
            std::cerr << "WIERSZ Z BAZY: id: " << atoi(wiersz_zapytania[0]) << " pin: " << atoi(wiersz_zapytania[1]) << " adres: " << wiersz_zapytania[2] << std::endl;
        CzujnikTemperatury czujnik(atoi(wiersz_zapytania[0]), atoi(wiersz_zapytania[1]), wiersz_zapytania[2]);
        czujniki_temperatury.push_back(czujnik);
    }
    mysql_free_result(wynik_zapytania); ///zwolnij wynik zapytania pinow

    for(size_t i = 0; i < czujniki_temperatury.size(); ++i)
    {
        if(czujniki_temperatury[i].InicjalizujCzujnik(sa) == false)
        {
            czujniki_temperatury[i].DezaktywujCzujnikWBazie(con);
        }
    }

    return czujniki_temperatury; //zwroc tablice wczytanych czujników
}

bool AktualizujCzujnikTemperatury(MYSQL *con, SerialArduino & sa, CzujnikTemperatury &C)
{
    if (C.AktualizujCzujnik(sa))
    {
        auto t = std::time(nullptr);
        auto now = std::localtime(&t);
        std::ostringstream zapytanie_sql;
        zapytanie_sql.precision(2);
        zapytanie_sql << "UPDATE Czujniki_Temperatury SET temperatura = " << std::fixed << C.ZwrocTemperature();
        zapytanie_sql << " ,ostatni_odczyt = '" << std::to_string(now->tm_hour) << ':' << std::to_string(now->tm_min) << ':' << std::to_string(now->tm_sec);
        zapytanie_sql << "' WHERE idCzujniki_Temperatury = " << std::to_string(C.ZwrocId());

        if(DEBUG_ARDUINO)
            std::cout << zapytanie_sql.str();

        AktualizujBaza(con, zapytanie_sql.str().c_str());
        return true;
    }
        
    if(C.ZwiekszLicznikBledow())
        C.DezaktywujCzujnikWBazie(con);

    return false;
}
