#include <iostream>
#include <mysql.h>
#include <fstream>
#include <string>
#include <wiringPi.h>
#include <chrono>
#include <ratio>
#include <thread>
#include <ctime>

#include "programy/obsluga_zadan.hh"
#include "komunikacja/baza.hh"
#include "konfiguracja.hh"

using namespace std::chrono;

#define CZAS_ZWLOKI 1000

using std::cerr;
using std::endl;

// Na wejście pierwsza baza to serwerownia, druga to kotlownia

int main(int argc, const char *argv[])
{
    MYSQL *con_serw;
    MYSQL *con_kotl;
    std::string dane_logowania[2][4];

    /**************************** BEGIN SETUP ****************************************/
    SendData("");
    cerr << "Wczytywanie ścieżek do plików z danymi" << endl;

    if (argc >= 3)
    {
        for (int i = 0; i < 2; ++i)
        {
            std::ifstream plik(argv[i + 1]);
            if (plik.is_open())
            {
                for (int j = 0; j < 4; ++j)
                    getline(plik, dane_logowania[i][j]);
                plik.close();
            }
            else
            {
                cerr << "Błąd otwierania pliku:" << argv[i + 1] << " z danymi." << endl;
                return -1;
            }
        }
    }
    else
    {
        cerr << "Brak ścieżki do plikow z danymi logowania do baz." << endl;
        return -1;
    }

    SendData("");
    cerr << "inicjalizacja połączenia z bazą i komunikat gdy coś się nie powiedzie" << endl;

    if (!InicjujBaza(&con_serw, dane_logowania[0]))
    {
        cerr << "Nie udało się połączyć z bazą " << dane_logowania[0][3] << " na tym urządzeniu." << endl;
        return -1;
    }

    if (!InicjujBaza(&con_kotl, dane_logowania[1]))
    {
        cerr << "Nie udało się połączyć z bazą " << dane_logowania[1][3] << " na tym urządzeniu." << endl;
        return -1;
    }
    /**************************** END SETUP ****************************************/

    /**************************** BEGIN LOOP ****************************************/

    InputPinTask WylewnicaOn(15, con_serw);
    InputPinTask WylewnicaOff(14, con_serw);
    OutputPinTask Wylewnica(37, con_serw);
    CountdownTimer WylewnicaTimer(getConfigParameterTime(5, con_serw));

    InputPinTask MasazWodnyOn(13, con_serw);
    InputPinTask MasazWodnyOff(12, con_serw);
    OutputPinTask MasazWodny(38, con_serw);
    CountdownTimer MasazWodnyTimer(getConfigParameterTime(6, con_serw));

    InputPinTask MasazPowietrznyOn(11, con_serw);
    InputPinTask MasazPowietrznyOff(10, con_serw);
    OutputPinTask MasazPowietrzny1(39, con_serw);
    OutputPinTask MasazPowietrzny2(56, con_serw);
    CountdownTimer MasazPowietrznyTimer(getConfigParameterTime(7, con_serw));
    CountdownTimer ZmianaMasazPowietrznyTimer(getConfigParameterTime(28, con_serw));

    InputPinTask PrzeciwfalaOn(9, con_serw);
    InputPinTask PrzeciwfalaOff(8, con_serw);
    OutputPinTask Przeciwfala(57, con_serw);
    CountdownTimer PrzeciwfalaTimer(getConfigParameterTime(8, con_serw));

    OutputPinTask PompaFiltr1(35, con_serw);
    OutputPinTask PompaFiltr2(36, con_serw);
    CountdownTimer PompaFiltrMaxCzasPracy(getConfigParameterTime(9, con_serw));
    CountdownTimer CyklPompaFiltrPraca(getConfigParameterTime(12, con_serw));
    CountdownTimer CyklPompaFiltrPrzerwa(getConfigParameterTime(13, con_serw));

    OutputPinTask PompaSolarnaBasen(77, con_serw);
    OutputPinTask PompaWymiennikBasen(139, con_kotl);

    OutputPinTask RekuperacjaBasenu(145, con_kotl);
    CountdownTimer CyklPompaRekuperacjaPraca(getConfigParameterTime(14, con_serw));
    CountdownTimer CyklPompaRekuperacjaPrzerwa(getConfigParameterTime(15, con_serw));

    OutputPinTask PompaFiltr1WDzien(35, con_serw);
    OutputPinTask PompaFiltr2WDzien(36, con_serw);
    CountdownTimer CyklPompaFiltryWDzienPraca(getConfigParameterTime(22, con_serw));
    CountdownTimer CyklPompaFiltryWDzienPrzerwa(getConfigParameterTime(23, con_serw));

    enum class F_ATRAKCJE
    {
        WYLEWNICA,
        MASAZ_WODNY,
        MASAZ_POWIETRZNY,
        PRZECIWFALA
    };

    enum class F_FILTR
    {
        NONE,
        FILTR_1,
        FILTR_2
    };

    int now_sec;
    bool f_atrakcje_aktywne = false;
    bool f_cykl_filtrow = false;
    bool f_praca_w_dzien_filtra = false;
    F_FILTR f_filtry_aktywne = F_FILTR::NONE;
    uint8_t f_zalacz_filtry = 0;
    bool f_filtry_w_dzien_aktywne = false;

    SendData("");
    cerr << "główna pętla programu" << endl;

    for (;;)
    {
        // SendData("");
        //     std::cout << "Loop\n";
        try
        {
            now_sec = nowToSec();

            // ############################################
            //  Praca basen atrakcje oraz filtr w dzień
            // ############################################
            if ((now_sec > getConfigParameterTime(10, con_serw) && now_sec < getConfigParameterTime(11, con_serw)) || getConfigParameterMode(17, con_serw))
            {
                if (!f_atrakcje_aktywne)
                    f_atrakcje_aktywne = true;

                if (!f_praca_w_dzien_filtra)
                    f_praca_w_dzien_filtra = true;


                // ############################################
                //  Tryb id 6 - praca basen wylewnica
                // ############################################
                if (getConfigParameterMode(6, con_serw))
                {
                    if (WylewnicaOn.Get())
                    {
                        WylewnicaTimer.Set();
                        Wylewnica.Set(true);
                        SendData("Zalaczenie wylewnicy (ON)");
                        f_zalacz_filtry |= (1 << (uint8_t)F_ATRAKCJE::WYLEWNICA);
                    }

                    if (WylewnicaOff.Get())
                    {
                        WylewnicaTimer.Reset();
                        Wylewnica.Set(false);
                        SendData("Wylaczenie wylewnicy (OFF)");
                        f_zalacz_filtry &= ~(1 << (uint8_t)F_ATRAKCJE::WYLEWNICA);
                    }

                    if (WylewnicaTimer.IsTimeout())
                    {
                        WylewnicaTimer.Reset();
                        Wylewnica.Set(false);
                        SendData("Wylaczenie wylewnicy (OFF)");
                        f_zalacz_filtry &= ~(1 << (uint8_t)F_ATRAKCJE::WYLEWNICA);
                    }
                }

                // ############################################
                //  Tryb id 7 - praca basen MasazWodny
                // ############################################
                if (getConfigParameterMode(7, con_serw))
                {
                    if (MasazWodnyOn.Get())
                    {
                        MasazWodnyTimer.Set();
                        MasazWodny.Set(true);
                        SendData("Zalaczenie masaz wodny (ON)");
                        f_zalacz_filtry |= (1 << (uint8_t)F_ATRAKCJE::MASAZ_WODNY);
                    }

                    if (MasazWodnyOff.Get())
                    {
                        MasazWodnyTimer.Reset();
                        MasazWodny.Set(false);
                        SendData("Wylaczenie masaz wodny (OFF)");
                        f_zalacz_filtry &= ~(1 << (uint8_t)F_ATRAKCJE::MASAZ_WODNY);
                    }

                    if (MasazWodnyTimer.IsTimeout())
                    {
                        MasazWodnyTimer.Reset();
                        MasazWodny.Set(false);
                        SendData("Wylaczenie masaz wodny (OFF)");
                        f_zalacz_filtry &= ~(1 << (uint8_t)F_ATRAKCJE::MASAZ_WODNY);
                    }
                }

                // ############################################
                //  Tryb id 8 - praca basen MasazPowietrzny
                // ############################################
                if (getConfigParameterMode(8, con_serw))
                {
                    if (MasazPowietrznyOn.Get())
                    {
                        MasazPowietrznyTimer.Set();
                        MasazPowietrzny1.Set(true);
                        MasazPowietrzny2.Set(false);
                        ZmianaMasazPowietrznyTimer.Set();
                        SendData("Zalaczenie masaz powietrzny 1 (ON)");
                        f_zalacz_filtry |= (1 << (uint8_t)F_ATRAKCJE::MASAZ_POWIETRZNY);
                    }

                    if (ZmianaMasazPowietrznyTimer.IsTimeout())
                    {
                        if (MasazPowietrzny1.isSet())
                        {
                            MasazPowietrzny1.Set(false);
                            MasazPowietrzny2.Set(true);
                            SendData("Zmiana na masaz powietrzny 2 (OFF/ON)");
                        }
                        else
                        {
                            MasazPowietrzny1.Set(true);
                            MasazPowietrzny2.Set(false);
                            SendData("Zmiana na masaz powietrzny 1 (ON/OFF)");
                        }

                        ZmianaMasazPowietrznyTimer.Set();
                    }

                    if (MasazPowietrznyOff.Get())
                    {
                        MasazPowietrznyTimer.Reset();
                        ZmianaMasazPowietrznyTimer.Reset();
                        MasazPowietrzny1.Set(false);
                        MasazPowietrzny2.Set(false);
                        SendData("Wylaczenie masaz powietrzny 1 i 2 (OFF)");
                        f_zalacz_filtry &= ~(1 << (uint8_t)F_ATRAKCJE::MASAZ_POWIETRZNY);
                    }

                    if (MasazPowietrznyTimer.IsTimeout())
                    {
                        MasazPowietrznyTimer.Reset();
                        ZmianaMasazPowietrznyTimer.Reset();
                        MasazPowietrzny1.Set(false);
                        MasazPowietrzny2.Set(false);
                        SendData("Wylaczenie masaz powietrzny 1 i 2 (OFF)");
                        f_zalacz_filtry &= ~(1 << (uint8_t)F_ATRAKCJE::MASAZ_POWIETRZNY);
                    }
                }

                // ############################################
                //  Tryb id 9 - praca basen Przeciwfala
                // ############################################
                if (getConfigParameterMode(9, con_serw))
                {
                    if (PrzeciwfalaOn.Get())
                    {
                        PrzeciwfalaTimer.Set();
                        Przeciwfala.Set(true);
                        SendData("Zalaczenie przeciwfala (ON)");
                        f_zalacz_filtry |= (1 << (uint8_t)F_ATRAKCJE::PRZECIWFALA);
                    }

                    if (PrzeciwfalaOff.Get())
                    {
                        PrzeciwfalaTimer.Reset();
                        Przeciwfala.Set(false);
                        SendData("Wylaczenie przeciwfala (OFF)");
                        f_zalacz_filtry &= ~(1 << (uint8_t)F_ATRAKCJE::PRZECIWFALA);
                    }

                    if (PrzeciwfalaTimer.IsTimeout())
                    {
                        PrzeciwfalaTimer.Reset();
                        Przeciwfala.Set(false);
                        SendData("Wylaczenie przeciwfala (OFF)");
                        f_zalacz_filtry &= ~(1 << (uint8_t)F_ATRAKCJE::PRZECIWFALA);
                    }
                }
            }
            else if(f_atrakcje_aktywne)
            {
                WylewnicaTimer.Reset();
                Wylewnica.Set(false);
                SendData("Wylaczenie wylewnicy (OFF)");

                MasazWodnyTimer.Reset();
                MasazWodny.Set(false);
                SendData("Wylaczenie masaz wodny (OFF)");

                MasazPowietrznyTimer.Reset();
                ZmianaMasazPowietrznyTimer.Reset();
                MasazPowietrzny1.Set(false);
                MasazPowietrzny2.Set(false);
                SendData("Wylaczenie masaz powietrzny (OFF)");

                PrzeciwfalaTimer.Reset();
                Przeciwfala.Set(false);
                SendData("Wylaczenie przeciwfala (OFF)");

                f_atrakcje_aktywne = false;
                f_praca_w_dzien_filtra = false;
                f_zalacz_filtry = 0;
            }

            // ############################################
            //  Tryb id 10 - praca basen filtry czasowe
            // ############################################
            if (getConfigParameterMode(10, con_serw))
            {
                if (!CyklPompaFiltrPrzerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                {

                    if (!CyklPompaFiltrPraca.IsRunning())
                    {
                        SendData("Załączenie cyklu czasowej filtracji basenu (ON)");
                        CyklPompaFiltrPraca.Set();
                        f_cykl_filtrow = true;
                    }

                    if (CyklPompaFiltrPraca.IsTimeout())
                    {
                        SendData("Załączenie cyklu czasowej filtracji basenu (OFF)");
                        CyklPompaFiltrPraca.Reset();
                        CyklPompaFiltrPrzerwa.Set();
                        f_cykl_filtrow = false;
                    }
                }

                if (!CyklPompaFiltrPraca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                {

                    if (!CyklPompaFiltrPrzerwa.IsRunning())
                    {
                        SendData("Załączenie cyklu czasowej filtracji basenu (OFF)");
                        CyklPompaFiltrPrzerwa.Set();
                        f_cykl_filtrow = false;
                    }

                    if (CyklPompaFiltrPrzerwa.IsTimeout())
                    {
                        SendData("Załączenie cyklu czasowej filtracji basenu (ON)");
                        CyklPompaFiltrPrzerwa.Reset();
                        CyklPompaFiltrPraca.Set();
                        f_cykl_filtrow = true;
                    }
                }
            }

            // ############################################
            //  Tryb id 18 - praca basen oba filtry w dzien
            // ############################################
            if (getConfigParameterMode(18, con_serw))
            {
                if (!f_filtry_w_dzien_aktywne)
                    f_filtry_w_dzien_aktywne = true;

                if ((now_sec > getConfigParameterTime(24, con_serw) && now_sec < getConfigParameterTime(25, con_serw))) {
                    if (!CyklPompaFiltryWDzienPrzerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                    {

                        if (!CyklPompaFiltryWDzienPraca.IsRunning()) {
                            SendData("Załączenie cyklu czasowej filtracji basenu (ON)");
                            CyklPompaFiltryWDzienPraca.Set();
                            PompaFiltr1WDzien.Set(true);
                            PompaFiltr2WDzien.Set(true);
                        }

                        if (CyklPompaFiltryWDzienPraca.IsTimeout()) {
                            SendData("Załączenie cyklu czasowej filtracji basenu (OFF)");
                            CyklPompaFiltryWDzienPraca.Reset();
                            CyklPompaFiltryWDzienPrzerwa.Set();
                            PompaFiltr1WDzien.Set(false);
                            PompaFiltr2WDzien.Set(false);
                        }
                    }

                    if (!CyklPompaFiltryWDzienPraca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                    {

                        if (!CyklPompaFiltryWDzienPrzerwa.IsRunning()) {
                            SendData("Załączenie cyklu czasowej filtracji basenu (OFF)");
                            CyklPompaFiltryWDzienPrzerwa.Set();
                            PompaFiltr1WDzien.Set(false);
                            PompaFiltr2WDzien.Set(false);
                        }

                        if (CyklPompaFiltryWDzienPrzerwa.IsTimeout()) {
                            SendData("Załączenie cyklu czasowej filtracji basenu (ON)");
                            CyklPompaFiltryWDzienPrzerwa.Reset();
                            CyklPompaFiltryWDzienPraca.Set();
                            PompaFiltr1WDzien.Set(true);
                            PompaFiltr2WDzien.Set(true);
                        }
                    }
                }
                else if(f_filtry_w_dzien_aktywne)
                {
                    CyklPompaFiltryWDzienPrzerwa.Reset();
                    CyklPompaFiltryWDzienPraca.Reset();
                    PompaFiltr1WDzien.Set(false);
                    PompaFiltr2WDzien.Set(false);
                    f_filtry_w_dzien_aktywne = false;
                }
            }
            else if(f_filtry_w_dzien_aktywne)
            {
                CyklPompaFiltryWDzienPrzerwa.Reset();
                CyklPompaFiltryWDzienPraca.Reset();
                PompaFiltr1WDzien.Set(false);
                PompaFiltr2WDzien.Set(false);
                f_filtry_w_dzien_aktywne = false;
            }

            // ############################################
            //  Tryb id 3 - praca pompy filtrujące basen gdy wymienniki, atrakcje
            // ############################################
            if (f_praca_w_dzien_filtra || f_zalacz_filtry || f_cykl_filtrow || getConfigParameterMode(3, con_serw) || PompaSolarnaBasen.Get() || PompaWymiennikBasen.Get())
            {
                if (f_filtry_aktywne == F_FILTR::NONE)
                {
                    if (getWorkingParameter(1, con_serw) > getWorkingParameter(2, con_serw)) // start the pump that has a shorter run time
                    {
                        PompaFiltr2.Set(true);
                        SendData("Zalaczenie pompy filtra 2");
                        f_filtry_aktywne = F_FILTR::FILTR_2;
                    }
                    else
                    {
                        PompaFiltr1.Set(true);
                        SendData("Zalaczenie pompy filtra 1");
                        f_filtry_aktywne = F_FILTR::FILTR_1;
                    }
                    PompaFiltrMaxCzasPracy.Set();
                }
                else if (PompaFiltrMaxCzasPracy.IsTimeout())
                {
                    if (f_filtry_aktywne == F_FILTR::FILTR_1)
                    {
                        SendData("Osiagnieto limit czasu. Zalaczenie pompy filtra 2");
                        incraseWorkingParameter(1, con_serw, PompaFiltrMaxCzasPracy.getElapsedSec());
                        PompaFiltrMaxCzasPracy.Set();
                        PompaFiltr1.Set(false);
                        PompaFiltr2.Set(true);
                        f_filtry_aktywne = F_FILTR::FILTR_2;
                    }
                    else
                    {
                        SendData("Osiagnieto limit czasu. Zalaczenie pompy filtra 1");
                        incraseWorkingParameter(2, con_serw, PompaFiltrMaxCzasPracy.getElapsedSec());
                        PompaFiltrMaxCzasPracy.Set();
                        PompaFiltr2.Set(false);
                        PompaFiltr1.Set(true);
                        f_filtry_aktywne = F_FILTR::FILTR_1;
                    }
                }
            }
            else if (f_filtry_aktywne != F_FILTR::NONE)
            {
                if (f_filtry_aktywne == F_FILTR::FILTR_1)
                {
                    SendData("Wylaczenie pompy filtra 1");
                    incraseWorkingParameter(1, con_serw, PompaFiltrMaxCzasPracy.getElapsedSec());
                    PompaFiltr1.Set(false);
                }
                else
                {
                    SendData("Wylaczenie pompy filtra 2");
                    incraseWorkingParameter(2, con_serw, PompaFiltrMaxCzasPracy.getElapsedSec());
                    PompaFiltr2.Set(false);
                }

                PompaFiltrMaxCzasPracy.Reset();
                f_filtry_aktywne = F_FILTR::NONE;
            }

            // ############################################
            //  Tryb id 11 - praca czasowa rekuperacja basenu
            // ############################################
            if (getConfigParameterMode(11, con_serw))
            {
                if (!CyklPompaRekuperacjaPrzerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                {

                    if (!CyklPompaRekuperacjaPraca.IsRunning())
                    {
                        SendData("Załączenie cyklu ON rekuperacji basenu");
                        CyklPompaRekuperacjaPraca.Set();
                        RekuperacjaBasenu.Set(true);
                    }

                    if (CyklPompaRekuperacjaPraca.IsTimeout())
                    {
                        SendData("Załączenie cyklu OFF rekuperacji basenu");
                        CyklPompaRekuperacjaPraca.Reset();
                        CyklPompaRekuperacjaPrzerwa.Set();
                        RekuperacjaBasenu.Set(false);
                    }
                }

                if (!CyklPompaRekuperacjaPraca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                {

                    if (!CyklPompaRekuperacjaPrzerwa.IsRunning())
                    {
                        SendData("Załączenie cyklu OFF rekuperacji basenu");
                        CyklPompaRekuperacjaPrzerwa.Set();
                        RekuperacjaBasenu.Set(false);
                    }

                    if (CyklPompaRekuperacjaPrzerwa.IsTimeout())
                    {
                        SendData("Załączenie cyklu ON rekuperacji basenu");
                        CyklPompaRekuperacjaPrzerwa.Reset();
                        CyklPompaRekuperacjaPraca.Set();
                        RekuperacjaBasenu.Set(true);

                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
        catch (...)
        {
            std::exception_ptr p = std::current_exception();
            cerr << (p ? p.__cxa_exception_type()->name() : "null") << endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(CZAS_ZWLOKI));
    }

    /**************************** END LOOP ****************************************/

    cerr << "Kończenie pracy programu." << endl;
    return -1;
}
