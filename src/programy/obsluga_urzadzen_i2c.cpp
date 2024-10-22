#include <iostream>
#include <mysql.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <fstream>
#include <string>

#include "komunikacja/baza.hh"
#include "sprzet/przekaznik.hh"
#include "sprzet/tca.hh"
#include "sprzet/czujniki_ruchu.hh"
#include "konfiguracja.hh"

#define CZAS_ZWLOKI 200

using std::cerr;
using std::endl;
using std::vector;

int main(int argc, const char *argv[])
{
    MYSQL *con;
    vector<Przekaznik> przekazniki;
    vector<CzujnikiRuchu> czujniki_ruchu;
    Tca tca;
    std::string dane_logowania[4];

    /**************************** BEGIN SETUP ****************************************/

    if (DEBUG)
        cerr << "Wczytywanie ścieżki do pliku z danymi" << endl;
    if (argc >= 2)
    {
        std::ifstream plik(argv[1]);
        if (plik.is_open())
        {
            for(int i = 0; i < 4; ++i)
                getline(plik,dane_logowania[i]);
            plik.close();
        }
        else
        {
            cerr << "Błąd otwierania pliku z danymi." << endl;
            return -1;
        }
    }
    else
    {
        cerr << "Brak ścieżki do pliku z danymi logowania do bazy." << endl;
        return -1;
    }

    if (DEBUG)
        cerr << "inicjalizacja wiringPi i sprawdzenie czy poprawnie ustawiono" << endl;
    if (wiringPiSetup() == -1)
    {
        cerr << "Błąd inicjalizacji wiringPi" << endl;
        return -1;
    }

    if (DEBUG)
        cerr << "inicjalizacja połączenia z bazą i komunikat gdy coś się nie powiedzie" << endl;
    
    if (!InicjujBaza(&con, dane_logowania))
    {
            cerr << "Nie udało się połączyć z bazą na tym urządzeniu." << endl;
            return -1;
    }

    if (DEBUG)
        cerr << "inicjalizacja multipleksera I2C" << endl;

    if (!tca.Inicjuj(TCA_ADRES))
    {
        cerr << "Błąd inicjalizacji multipleksera I2C." << endl;
        return -1;
    }
    
    if(DEBUG) 
        cerr << endl << tca << endl << endl;

    if (DEBUG) 
        cerr << "Próba inicjalizacji urzadzen I2C" << endl;

    try
    {
        if(DEBUG) 
            cerr << "Inicjalizacji przekaźników" <<endl;

        przekazniki = StartPrzekazniki(con, tca);

        if(DEBUG) 
            cerr << endl << przekazniki << endl << endl;

        if(DEBUG) 
            cerr << "Inicjalizacji czujników ruchu" <<endl;

        czujniki_ruchu = StartCzujnikiRuchu(con, tca);

        if(DEBUG) 
            cerr << endl << czujniki_ruchu << endl << endl;
    }

    catch (std::exception &e)
    {
        cerr << "Wyjątek: " << e.what() << endl;
        return -1;
    }

    catch (...)
    {
        std::exception_ptr p = std::current_exception();
        cerr << (p ? p.__cxa_exception_type()->name() : "null") << endl;
        return -1;
    }

    /**************************** END SETUP ****************************************/

    /**************************** BEGIN LOOP ****************************************/

    if (DEBUG)
        cerr << "główna pętla programu" << endl;

    for (;;)
    {
        try
        {
            //if(DEBUG) cerr<<"aktualizuj czujniki ruchu z tego urzadzenia" << endl;
            AktualizujRuch(con, tca, czujniki_ruchu);

            //if(DEBUG) cerr<<"aktualizuj przekazniki tego urzadzenia" << endl;
            AktualizujPrzekazniki(con, tca, przekazniki);

            delay(CZAS_ZWLOKI);
        }
        catch (std::exception &e)
        {
            cerr << e.what();
            break;
        }
        catch (...)
        {
            std::exception_ptr p = std::current_exception();
            cerr << (p ? p.__cxa_exception_type()->name() : "null") << endl;
        }
    }

    /**************************** END LOOP ****************************************/

    ZakonczPrzekazniki(con, tca, przekazniki);
    cerr << "Kończenie pracy programu." << endl;
    return -1;
}
