#include <iostream>
#include <mysql.h>
#include <fstream>
#include <string>
#include <wiringPi.h>

#include "komunikacja/baza.hh"
#include "programy/zdarzenia_ruchu/zdarzenie_ruchu.hh"
#include "konfiguracja.hh"

#define CZAS_ZWLOKI 300

using std::cerr;
using std::endl;
using std::vector;

//Na wejście pierwsza baza to ta na urzadzeniu, dodatkowa to ta druga

int main(int argc, const char *argv[])
{
    MYSQL *con[2];
    vector<ZdarzenieRuchu> zdarzenia_ruchu;
    std::string dane_logowania[2][4];
    int nr_bazy_serwerowni;

    /**************************** BEGIN SETUP ****************************************/
    if (DEBUG)
        cerr << "Wczytywanie ścieżek do plików z danymi" << endl;
        
    if (argc >= 2 + 1)
    {
        for(int i = 0; i < 2; ++i)
        {
            std::ifstream plik(argv[i+1]);
            if (plik.is_open())
            {
                for(int j = 0; j < 4; ++j)
                    getline(plik,dane_logowania[i][j]);
                plik.close();
            }
            else
            {
                cerr << "Błąd otwierania pliku:" << argv[i + 1] <<" z danymi." << endl;
                return -1;
            }
        }
    }
    else
    {
        cerr << "Brak ścieżki do plikow z danymi logowania do baz." << endl;
        return -1;
    }

    if (DEBUG)
        cerr << "inicjalizacja połączenia z bazą i komunikat gdy coś się nie powiedzie" << endl;

    for(int i = 0; i < 2; ++i)
    {
        if (!InicjujBaza(&con[i], dane_logowania[i]))
        {
                cerr << "Nie udało się połączyć z bazą " << dane_logowania[i][3] << " na tym urządzeniu." << endl;
                return -1;
        }
    }

    if (DEBUG)
        cerr << "Wybranie bazy w ktorej jest czujnik jasnosci : " ;

    if(dane_logowania[0][3] == "Serwerownia")
        nr_bazy_serwerowni = 0;
    else
        nr_bazy_serwerowni = 1;

    if (DEBUG)
        cerr << nr_bazy_serwerowni << endl;

    if (DEBUG) 
        cerr << "Próba inicjalizacji zdarzeń ruchu" << endl;

    try
    {
        if(DEBUG)
        {
            cerr << "Inicjalizacji zdarzeń ruchu" <<endl;
        }

            zdarzenia_ruchu = StartZdarzeniaRuchu(con[0]);
        
        if(DEBUG) 
            cerr << endl << zdarzenia_ruchu << endl << endl;
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
            //if(DEBUG) cerr<<"aktualizuj zdarzenia ruchu związane z czujnikami ruchu tego urzadzenia" << endl;
            AktualizujZdarzeniaRuchu(con, zdarzenia_ruchu, nr_bazy_serwerowni);

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

    cerr << "Kończenie pracy programu." << endl;
    return -1;
}
