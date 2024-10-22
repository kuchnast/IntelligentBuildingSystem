#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#include "komunikacja/baza.hh"
#include "komunikacja/komunikacja_z_arduino.hh"
#include "sprzet/czujniki_temperatury.hh"
#include "konfiguracja.hh"

#define CZAS_ZWLOKI 5000

using std::cerr;
using std::endl;

int main(int argc, const char *argv[])
{
    MYSQL *con;
    std::string dane_logowania[4];
    SerialArduino sa;
    std::vector<CzujnikTemperatury> cz_temp;

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
        cerr << "Inicjalizacja połączenia z bazą i komunikat gdy coś się nie powiedzie" << endl;
    
    if (!InicjujBaza(&con, dane_logowania))
    {
            cerr << "Nie udało się połączyć z bazą na tym urządzeniu." << endl;
            return -1;
    }

    if (DEBUG)
        cerr << "Inicjalizacja połączenia z arduino" << endl;

    int i_blad = 5; // kilkukrotna próba inicializacji z 1s opóźnieniem
    while(!sa.Open())
    {
        cerr << "Błąd łączenia z arduino" << endl;
        if(--i_blad < 0)
        {
            cerr << "Nie można nawiązać komunikacji z arduino." << endl;
            return -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    if (DEBUG)
        cerr << "Inicjalizacja czujników temperatury" << endl;
    cz_temp = StartCzujnikiTemperatury(con,sa);

    /**************************** END SETUP ****************************************/

    /**************************** BEGIN LOOP ****************************************/
for(;;)
{
    if (DEBUG)
        cerr << "Pętla" << endl;

    for(auto & C : cz_temp)
    {
        bool st = AktualizujCzujnikTemperatury(con, sa, C);
        if (DEBUG)
            cerr << "Czujnik id = " << C.ZwrocId() << " Temp = " << C.ZwrocTemperature() << " CzyBłąd = " << C.CzyBlad() <<  " Stan = " << st << endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(CZAS_ZWLOKI));
}
    /**************************** END LOOP ****************************************/
    return 0;
}