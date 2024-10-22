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

enum class F_PIECE_GAZOWE
{
    NONE = 0,
    PIEC_1 = 1,
    PIEC_2 = 2,
    OBA_PIECE = 3
};

// Na wejście pierwsza baza to kotlownia, druga to serwerownia

int main(int argc, const char *argv[])
{
    MYSQL *con_kotl;
    MYSQL *con_serw;
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

    if (!InicjujBaza(&con_kotl, dane_logowania[0]))
    {
        cerr << "Nie udało się połączyć z bazą " << dane_logowania[0][3] << " na tym urządzeniu." << endl;
        return -1;
    }

    if (!InicjujBaza(&con_serw, dane_logowania[1]))
    {
        cerr << "Nie udało się połączyć z bazą " << dane_logowania[1][3] << " na tym urządzeniu." << endl;
        return -1;
    }
    /**************************** END SETUP ****************************************/

    /**************************** BEGIN LOOP ****************************************/

    SendData("");
        cerr << "główna pętla programu" << endl;

    OutputPinTask PompaCWU(144, con_kotl);
    OutputPinTask PompaCyrkulacji(65, con_serw);
    OutputPinTask PompaPiecEko(146, con_kotl);

    TemperatureTask TempSprzeglo(11, con_kotl);
    TemperatureTask TempCwu(6, con_serw);

    DelayOnOffTimer SprzegloZwlokaOnOff(getConfigParameterTime(1, con_kotl));
    DelayOnOffTimer PiecEkoZwlokaOnOff(getConfigParameterTime(13, con_kotl));

    CountdownTimer CyklDogrzewaniaCyrkulacjiPraca(getConfigParameterTime(3, con_kotl));
    CountdownTimer CyklDogrzewaniaCyrkulacjiPrzerwa(getConfigParameterTime(4, con_kotl));
    CountdownTimer CyklPoZagrzaniuCyrkulacjiPraca(getConfigParameterTime(5, con_kotl));
    CountdownTimer CyklPoZagrzaniuCyrkulacjiPrzerwa(getConfigParameterTime(6, con_kotl));

    TemperatureTask TemmpPiecEko1(12, con_kotl);
    TemperatureTask TemmpPiecEko2(13, con_kotl);

    OutputPinTask Pompa8(135, con_kotl);
    CountdownTimer CyklPompa8Praca(getConfigParameterTime(7, con_kotl));
    CountdownTimer CyklPompa8Przerwa(getConfigParameterTime(8, con_kotl));
    CountdownTimer Pompa8Wylaczenie(getConfigParameterTime(9, con_kotl));

    OutputPinTask Pompa9(136, con_kotl);
    CountdownTimer CyklPompa9Praca(getConfigParameterTime(10, con_kotl));
    CountdownTimer CyklPompa9Przerwa(getConfigParameterTime(11, con_kotl));
    CountdownTimer Pompa9Wylaczenie(getConfigParameterTime(12, con_kotl));

    bool cykl_pracy_dmuchawa_piec_eko(false);
    OutputPinTask DmuchawaPiecEko(149, con_kotl);
    InputPinTask DmuchawaPiecEkoOn(11, con_kotl);
    InputPinTask DmuchawaPiecEkoOff(10, con_kotl);
    DelayOnOffTimer DmuchawaPiecEkoZwlokaOnOff(getConfigParameterTime(15, con_kotl));
    WatchdogTimer WatchdogDmuchawaPiecEko(getConfigParameterTime(14, con_kotl));

    bool cykl_pracy_podajnik_dmuchawa(false);
    bool cykl_podajnik_zwloka(false);
    OutputPinTask DmuchawaPodajnik(148, con_kotl);
    OutputPinTask Podajnik(147, con_kotl);
    InputPinTask PodajnikDmuchawaOn(9, con_kotl);
    InputPinTask PodajnikDmuchawaOff(12, con_kotl);
    DelayOnOffTimer PodajnikDmuchawaZwlokaOnOff(getConfigParameterTime(15, con_kotl));
    WatchdogTimer WatchdogPodajnikDmuchawa(getConfigParameterTime(14, con_kotl));
    CountdownTimer CyklPodajnikPraca(getConfigParameterTime(17, con_kotl));
    CountdownTimer CyklPodajnikPrzerwa(getConfigParameterTime(18, con_kotl));

    F_PIECE_GAZOWE f_piece_aktywne = F_PIECE_GAZOWE::NONE;
    TemperatureTask TempBasen(7, con_serw);
    OutputPinTask ZasilaniePiecGazowy1(150, con_kotl);
    OutputPinTask ZasilaniePiecGazowy2(151, con_kotl);
    OutputPinTask PiecGazowy1(159, con_kotl);
    OutputPinTask PiecGazowy2(158, con_kotl);
    CountdownTimer ZalaczDrugiPiecTimer(getConfigParameterTime(19, con_kotl));
    DelayOnOffTimer PieceGazoweZwlokaOnOff(getConfigParameterTime(20, con_kotl));
    CountdownTimer PracaPiecGazowy1(1);
    CountdownTimer PracaPiecGazowy2(1);

    OutputPinTask PompaWymiennikBasen(139, con_kotl);
    DelayOnOffTimer WymiennikBasenZwlokaOnOff(getConfigParameterTime(21, con_kotl));

    ZasilaniePiecGazowy1.Set(true);
    ZasilaniePiecGazowy2.Set(true);

    OutputPinTask PompaStudnia(157, con_kotl);
    CountdownTimer CyklStudniPraca(getConfigParameterTime(22, con_kotl));
    CountdownTimer CyklStudniPrzerwa(getConfigParameterTime(23, con_kotl));

    OutputPinTask Pompa4(131, con_kotl);
    CountdownTimer CyklPompa4Praca(getConfigParameterTime(24, con_kotl));
    CountdownTimer CyklPompa4Przerwa(getConfigParameterTime(25, con_kotl));
    CountdownTimer Pompa4Wylaczenie(getConfigParameterTime(26, con_kotl));

    OutputPinTask Pompa5(132, con_kotl);
    CountdownTimer CyklPompa5Praca(getConfigParameterTime(27, con_kotl));
    CountdownTimer CyklPompa5Przerwa(getConfigParameterTime(28, con_kotl));
    CountdownTimer Pompa5Wylaczenie(getConfigParameterTime(29, con_kotl));

    OutputPinTask PlukanieFiltraWody(155, con_kotl);
    CountdownTimer CyklPlukanieFiltraWodyPraca(getConfigParameterTime(30, con_kotl));
    CountdownTimer CyklPlukanieFiltraWodyPrzerwa(getConfigParameterTime(31, con_kotl));

    OutputPinTask PlukanieHydrofor(154, con_kotl);
    CountdownTimer CyklPlukanieHydroforPraca(getConfigParameterTime(32, con_kotl));
    CountdownTimer CyklPlukanieHydroforPrzerwa(getConfigParameterTime(33, con_kotl));

    OutputPinTask OdwodnienieBudynku(32, con_serw);
    CountdownTimer CyklOdwodnienieBudynkuPraca(getConfigParameterTime(34, con_kotl));
    CountdownTimer CyklOdwodnienieBudynkuPrzerwa(getConfigParameterTime(35, con_kotl));

    for (;;)
    {
        //SendData("");
        //    std::cout << "Loop\n";
        try
        {

            // ############################################
            //  Tryb id 1 - praca pompy CWU
            // ############################################
            if (getConfigParameterMode(1, con_kotl))
            {
                /**
                 * @brief Jeżeli temperatura sprzęgła większa od minimalnej oraz temperatura zbiornika mniejsza niż zadana, załącz sprzęgło
                 *
                 */
                if (TempSprzeglo.Get() >= getConfigParameterTemp(1, con_kotl) && TempCwu.Get() < getConfigParameterTemp(3, con_kotl))
                {
                    SprzegloZwlokaOnOff.changeMode(Mode::ON);

                    if (!SprzegloZwlokaOnOff.isFinished())
                    {
                        if (!SprzegloZwlokaOnOff.IsRunning())
                        {
                            SprzegloZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie załączenia sprzęgła");
                        }

                        if (SprzegloZwlokaOnOff.IsTimeout())
                        {
                            PompaCWU.Set(true);
                            SprzegloZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie załączenia sprzęgła");
                        }
                    }
                }
                else
                {
                    SprzegloZwlokaOnOff.changeMode(Mode::OFF);

                    if (!SprzegloZwlokaOnOff.isFinished())
                    {
                        if (!SprzegloZwlokaOnOff.IsRunning())
                        {
                            SprzegloZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie wyłączenia sprzęgła");
                        }

                        if (SprzegloZwlokaOnOff.IsTimeout())
                        {
                            PompaCWU.Set(false);
                            SprzegloZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie wyłączenia sprzęgła");
                        }
                    }
                }
            }

            // ############################################
            //  Tryb id 2 - praca cyrkulacji
            // ############################################
            if (getConfigParameterMode(2, con_kotl))
            {
                /**
                 * @brief Jeżeli temperatura CWU większa od minimalnej temperatury zbiornika oraz temperatura CWU mniejsza od zadanej temperatury zbiornika, uruchom cykl dogrzewania pompy cyrkulacji
                 *
                 */
                if (TempCwu.Get() >= getConfigParameterTemp(2, con_kotl) && TempCwu.Get() < getConfigParameterTemp(3, con_kotl))
                {
                    if (CyklPoZagrzaniuCyrkulacjiPraca.IsRunning() || CyklPoZagrzaniuCyrkulacjiPrzerwa.IsRunning())
                    {
                        SendData("Reset odliczania cyklu po zagrzaniu pompa cyrkulacji");
                        CyklPoZagrzaniuCyrkulacjiPraca.Reset();
                        CyklPoZagrzaniuCyrkulacjiPrzerwa.Reset();
                    }

                    if (!CyklDogrzewaniaCyrkulacjiPrzerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                    {

                        if (!CyklDogrzewaniaCyrkulacjiPraca.IsRunning())
                        {
                            SendData("Załączenie cyklu dogrzewania pompa cyrkulacji (ON)");
                            CyklDogrzewaniaCyrkulacjiPraca.Set();
                            PompaCyrkulacji.Set(true);
                        }

                        if (CyklDogrzewaniaCyrkulacjiPraca.IsTimeout())
                        {
                            SendData("Załączenie cyklu dogrzewania pompa cyrkulacji (OFF)");
                            CyklDogrzewaniaCyrkulacjiPraca.Reset();
                            CyklDogrzewaniaCyrkulacjiPrzerwa.Set();
                            PompaCyrkulacji.Set(false);
                        }
                    }

                    if (!CyklDogrzewaniaCyrkulacjiPraca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                    {

                        if (!CyklDogrzewaniaCyrkulacjiPrzerwa.IsRunning())
                        {
                            SendData("Załączenie cyklu dogrzewania pompa cyrkulacji (ON)");
                            CyklDogrzewaniaCyrkulacjiPrzerwa.Set();
                            PompaCyrkulacji.Set(false);
                        }

                        if (CyklDogrzewaniaCyrkulacjiPrzerwa.IsTimeout())
                        {
                            SendData("Załączenie cyklu dogrzewania pompa cyrkulacji (ON)");

                            CyklDogrzewaniaCyrkulacjiPrzerwa.Reset();
                            CyklDogrzewaniaCyrkulacjiPraca.Set();
                            PompaCyrkulacji.Set(true);
                        }
                    }
                }
                /**
                 * @brief Jeżeli temperatura CWU większa od temperatury zadanej zbiornika, uruchom cykl po zagrzaniu
                 *
                 */
                else if (TempCwu.Get() > getConfigParameterTemp(3, con_kotl))
                {
                    if (CyklDogrzewaniaCyrkulacjiPraca.IsRunning() || CyklDogrzewaniaCyrkulacjiPrzerwa.IsRunning())
                    {
                        SendData("Reset odliczania cyklu dogrzewania pompa cyrkulacji");
                        CyklDogrzewaniaCyrkulacjiPraca.Reset();
                        CyklDogrzewaniaCyrkulacjiPrzerwa.Reset();
                    }

                        if (!CyklPoZagrzaniuCyrkulacjiPrzerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                        {
                            if (!CyklPoZagrzaniuCyrkulacjiPraca.IsRunning())
                            {
                                SendData("Załączenie cyklu po zagrzaniu pompa cyrkulacji (ON)");
                                CyklPoZagrzaniuCyrkulacjiPraca.Set();
                                PompaCyrkulacji.Set(true);
                            }

                            if (CyklPoZagrzaniuCyrkulacjiPraca.IsTimeout())
                            {
                                SendData("Załączenie cyklu po zagrzaniu pompa cyrkulacji (OFF)");
                                CyklPoZagrzaniuCyrkulacjiPraca.Reset();
                                CyklPoZagrzaniuCyrkulacjiPrzerwa.Set();
                                PompaCyrkulacji.Set(false);
                            }
                        }

                    if (!CyklPoZagrzaniuCyrkulacjiPraca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                    {
                        if (!CyklPoZagrzaniuCyrkulacjiPrzerwa.IsRunning())
                        {
                            SendData("Załączenie cyklu po zagrzaniu pompa cyrkulacji (OFF)");
                            CyklPoZagrzaniuCyrkulacjiPrzerwa.Set();
                            PompaCyrkulacji.Set(false);
                        }

                        if (CyklPoZagrzaniuCyrkulacjiPrzerwa.IsTimeout())
                        {
                            SendData("Załączenie cyklu po zagrzaniu pompa cyrkulacji (ON)");
                            CyklPoZagrzaniuCyrkulacjiPrzerwa.Reset();
                            CyklPoZagrzaniuCyrkulacjiPraca.Set();
                            PompaCyrkulacji.Set(true);
                        }
                    }
                }
                else if (CyklDogrzewaniaCyrkulacjiPraca.IsRunning() || CyklDogrzewaniaCyrkulacjiPrzerwa.IsRunning() || CyklPoZagrzaniuCyrkulacjiPraca.IsRunning() || CyklPoZagrzaniuCyrkulacjiPrzerwa.IsRunning())
                {
                    SendData("Wyłączenie cyklów cyrkulacji oraz pompy cyrkulacji, gdy temperatura zbiornika mniejsza od minimalnej");
                    PompaCyrkulacji.Set(false);
                    CyklDogrzewaniaCyrkulacjiPraca.Reset();
                    CyklDogrzewaniaCyrkulacjiPrzerwa.Reset();
                    CyklPoZagrzaniuCyrkulacjiPraca.Reset();
                    CyklPoZagrzaniuCyrkulacjiPrzerwa.Reset();
                }
            }

            // ############################################
            //  Tryb id 3 - praca pompa 8 - podloga parter piwnica B1
            // ############################################
            if (getConfigParameterMode(3, con_kotl))
            {
                /**
                 * @brief Jeżeli temperatura sprzęgła większa od minimalnej załącz cykl grzania, jeżeli mniejsza - odliczaj wyłączenie cyklu
                 *
                 */
                if (TempSprzeglo.Get() >= getConfigParameterTemp(1, con_kotl) || Pompa8Wylaczenie.IsRunning())
                {       

                    if (!CyklPompa8Przerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                    {
                        

                        if (!CyklPompa8Praca.IsRunning())
                        {
                            SendData("Załączenie cyklu pompa 8 (ON)");
                            CyklPompa8Praca.Set();
                            Pompa8.Set(true);
                        }

                        if (CyklPompa8Praca.IsTimeout())
                        {
                            SendData("Załączenie cyklu pompa 8 (OFF)");
                            CyklPompa8Praca.Reset();
                            CyklPompa8Przerwa.Set();
                            Pompa8.Set(false);
                        }
                    }

                    if (!CyklPompa8Praca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                    {

                        if (!CyklPompa8Przerwa.IsRunning())
                        {
                            SendData("Załączenie cyklu pompa 8 (OFF)");
                            CyklPompa8Przerwa.Set();
                            Pompa8.Set(false);
                        }

                        if (CyklPompa8Przerwa.IsTimeout())
                        {
                            SendData("Załączenie cyklu pompa 8 (ON)");
                            CyklPompa8Przerwa.Reset();
                            CyklPompa8Praca.Set();
                            Pompa8.Set(true);
                        }
                    }
                }

                if (TempSprzeglo.Get() < getConfigParameterTemp(1, con_kotl))
                {
                    if (CyklPompa8Praca.IsRunning() || CyklPompa8Przerwa.IsRunning())
                    {
                        if (!Pompa8Wylaczenie.IsRunning())
                        {
                            SendData("Rozpoczęcie odliczania wyłączenia cyklu pompa 8");
                            Pompa8Wylaczenie.Set();
                        }

                        if (Pompa8Wylaczenie.IsTimeout())
                        {
                            SendData("Wyłączenie cyklu pompa 8");
                            Pompa8Wylaczenie.Reset();
                            CyklPompa8Praca.Reset();
                            CyklPompa8Przerwa.Reset();
                            Pompa8.Set(false);
                        }
                    }
                }
                else if (Pompa8Wylaczenie.IsRunning())
                {
                    SendData("Reset odliczania wyłączenia pompa 8");
                    Pompa8Wylaczenie.Reset();
                }
            }

            // ############################################
            //  Tryb id 4 - praca pompa 9 - grzejnik B1
            // ############################################
            if (getConfigParameterMode(4, con_kotl))
            {
                /**
                 * @brief Jeżeli temperatura sprzęgła większa od minimalnej załącz cykl grzania, jeżeli mniejsza - odliczaj wyłączenie cyklu
                 *
                 */
                if (TempSprzeglo.Get() >= getConfigParameterTemp(1, con_kotl) || Pompa9Wylaczenie.IsRunning())
                {

                    if (!CyklPompa9Przerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                    {

                        if (!CyklPompa9Praca.IsRunning())
                        {
                            SendData("Załączenie cyklu pompa 9 (ON)");
                            CyklPompa9Praca.Set();
                            Pompa9.Set(true);
                        }

                        if (CyklPompa9Praca.IsTimeout())
                        {
                            SendData("Załączenie cyklu pompa 9 (OFF)");
                            CyklPompa9Praca.Reset();
                            CyklPompa9Przerwa.Set();
                            Pompa9.Set(false);
                        }
                    }

                    if (!CyklPompa9Praca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                    {

                        if (!CyklPompa9Przerwa.IsRunning())
                        {
                            SendData("Załączenie cyklu pompa 9 (OFF)");
                            CyklPompa9Przerwa.Set();
                            Pompa9.Set(false);
                        }

                        if (CyklPompa9Przerwa.IsTimeout())
                        {
                            SendData("Załączenie cyklu pompa 9 (ON)");
                            CyklPompa9Przerwa.Reset();
                            CyklPompa9Praca.Set();
                            Pompa9.Set(true);
                        }
                    }
                }

                if (TempSprzeglo.Get() < getConfigParameterTemp(1, con_kotl))
                {
                    if (CyklPompa9Praca.IsRunning() || CyklPompa9Przerwa.IsRunning())
                    {
                        if (!Pompa9Wylaczenie.IsRunning())
                        {
                            SendData("Rozpoczęcie odliczania wyłączenia cyklu pompa 9");
                            Pompa9Wylaczenie.Set();
                        }

                        if (Pompa9Wylaczenie.IsTimeout())
                        {
                            SendData("Wyłączenie cyklu pompa 9");
                            Pompa9Wylaczenie.Reset();
                            CyklPompa9Praca.Reset();
                            CyklPompa9Przerwa.Reset();
                            Pompa9.Set(false);
                        }
                    }
                }
                else if (Pompa9Wylaczenie.IsRunning())
                {
                    SendData("Reset odliczania wyłączenia pompa 9");
                    Pompa9Wylaczenie.Reset();
                }
            }

            // ############################################
            //  Tryb id 5 - praca pompy pieca Eko
            // ############################################
            if (getConfigParameterMode(5, con_kotl))
            {
                /**
                 * @brief Jeżeli temperatura pieca eko większa od minimalnej, załącz pompe pieca eko
                 *
                 */
                if ((TemmpPiecEko1.Get() >= getConfigParameterTemp(4, con_kotl) && f_piece_aktywne == F_PIECE_GAZOWE::NONE) || TemmpPiecEko1.Get() > getConfigParameterTemp(7, con_kotl))
                {
                    PiecEkoZwlokaOnOff.changeMode(Mode::ON);

                    if (!PiecEkoZwlokaOnOff.isFinished())
                    {
                        if (!PiecEkoZwlokaOnOff.IsRunning())
                        {
                            PiecEkoZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie załączenia pompa piec eko");
                        }

                        if (PiecEkoZwlokaOnOff.IsTimeout())
                        {
                            PompaPiecEko.Set(true);
                            PiecEkoZwlokaOnOff.taskFinished();

                            SendData("Zakończ odliczanie załączenia pompa piec eko");
                        }
                    }
                }
                else
                {
                    PiecEkoZwlokaOnOff.changeMode(Mode::OFF);

                    if (!PiecEkoZwlokaOnOff.isFinished())
                    {
                        if (!PiecEkoZwlokaOnOff.IsRunning())
                        {
                            PiecEkoZwlokaOnOff.Set();

                            SendData("Rozpocznij odliczanie wyłączenia pompa piec eko");
                        }

                        if (PiecEkoZwlokaOnOff.IsTimeout())
                        {
                            PompaPiecEko.Set(false);
                            PiecEkoZwlokaOnOff.taskFinished();

                            SendData("Zakończ odliczanie wyłączenia pompa piec eko");
                        }
                    }
                }
            }

            // ############################################
            //  Tryb id 6 - praca dmuchawy pieca Eko załączana ręcznie przyciskami on/off
            // ############################################
            if (getConfigParameterMode(6, con_kotl))
            {
                /**
                 * @brief Jeżeli temperatura pieca eko mniejsza od zadanej, załącz dmuchawe. Gdy brak wzrostu temperatury mimo pracy - automatyczne wyłączenie
                 *        Przycisk On - załączenie cyklu i natychmiastowy start dmuchawy (jeżeli warunek spełniony), dalsza praca ze zwłoką
                 *        Przycisk Off - wyłączenie cyklu
                 *        Watchdog - jeżeli brak wzrostu temperatury do zadanej w danym czasie, wyłącz cykl
                 */

                if (cykl_pracy_dmuchawa_piec_eko == false && DmuchawaPiecEkoOn.Get()) // Załączenie programu
                {
                    SendData("Załączenie programu dmuchawa piec eko");
                    cykl_pracy_dmuchawa_piec_eko = true;
                    if (TemmpPiecEko1.Get() < getConfigParameterTemp(5, con_kotl))
                    {
                        SendData("Pierwsze załączenie dmuchawa piec eko bez zwłoki");
                        DmuchawaPiecEko.Set(true);
                        DmuchawaPiecEkoZwlokaOnOff.changeMode(Mode::ON);
                        DmuchawaPiecEkoZwlokaOnOff.taskFinished();
                        WatchdogDmuchawaPiecEko.Feed();
                    }
                }

                if (cykl_pracy_dmuchawa_piec_eko) // Praca w zależności od temperatury i monitorowanie watchdoga
                {
                    if (TemmpPiecEko1.Get() < getConfigParameterTemp(5, con_kotl))
                    {
                        DmuchawaPiecEkoZwlokaOnOff.changeMode(Mode::ON);

                        if (!DmuchawaPiecEkoZwlokaOnOff.isFinished())
                        {
                            if (!DmuchawaPiecEkoZwlokaOnOff.IsRunning())
                            {
                                DmuchawaPiecEkoZwlokaOnOff.Set();

                                SendData("Rozpocznij odliczanie załączenia dmuchawa piec eko");
                            }

                            if (DmuchawaPiecEkoZwlokaOnOff.IsTimeout())
                            {
                                DmuchawaPiecEko.Set(true);
                                DmuchawaPiecEkoZwlokaOnOff.taskFinished();
                                WatchdogDmuchawaPiecEko.Feed();

                                SendData("Zakończ odliczanie załączenia dmuchawa piec eko");
                            }
                        }
                        else // Praca watchdoga, tylko po załączeniu, reset starej wartości czasu wykonywany w funkcji załączającej
                        {
                            if (WatchdogDmuchawaPiecEko.IsTimeout())
                            {
                                SendData("Wyłączenie przez watchdoga dmuchawa piec eko");
                                DmuchawaPiecEko.Set(false);
                                cykl_pracy_dmuchawa_piec_eko = false;
                            }
                        }
                    }
                    else
                    {
                        DmuchawaPiecEkoZwlokaOnOff.changeMode(Mode::OFF);

                        if (!DmuchawaPiecEkoZwlokaOnOff.isFinished())
                        {
                            if (!DmuchawaPiecEkoZwlokaOnOff.IsRunning())
                            {
                                DmuchawaPiecEkoZwlokaOnOff.Set();
                                SendData("Rozpocznij odliczanie wyłączenia dmuchawa piec eko");
                            }

                            if (DmuchawaPiecEkoZwlokaOnOff.IsTimeout())
                            {
                                DmuchawaPiecEko.Set(false);
                                DmuchawaPiecEkoZwlokaOnOff.taskFinished();
                                SendData("Zakończ odliczanie wyłączenia dmuchawa piec eko");
                            }
                        }
                    }

                    if (DmuchawaPiecEkoOff.Get())
                    {
                        SendData("Wyłączenie przez przycisk OFF dmuchawa piec eko");
                        DmuchawaPiecEko.Set(false);
                        cykl_pracy_dmuchawa_piec_eko = false;
                    }
                }
            }

            // ############################################
            //  Tryb id 7 - praca podajnika i jego dmuchawy załączana ręcznie przyciskami on/off
            // ############################################
            if (getConfigParameterMode(7, con_kotl))
            {
                /**
                 * @brief Jeżeli temperatura pieca eko mniejsza od zadanej, załącz dmuchawe podajnika i cykl podajnika. Gdy brak wzrostu temperatury mimo pracy - automatyczne wyłączenie
                 *        Przycisk On - załączenie cyklu oraz natychmiastowy start dmuchawy i podajnika (jeżeli warunek spełniony), dalsza praca ze zwłoką
                 *        Przycisk Off - wyłączenie cyklu
                 *        Watchdog - jeżeli brak wzrostu temperatury do zadanej w danym czasie, wyłącz cykl
                 */

                if (cykl_pracy_podajnik_dmuchawa == false && PodajnikDmuchawaOn.Get()) // Załączenie programu przyciskiem
                {
                    SendData("Załączenie programu dmuchawa podajnika i podajnik");
                    cykl_pracy_podajnik_dmuchawa = true;
                    if (TemmpPiecEko1.Get() < getConfigParameterTemp(6, con_kotl))
                    {
                        SendData("Pierwsze załączenie dmuchawa podajnika i podajnik bez zwłoki");
                        DmuchawaPodajnik.Set(true);
                        PodajnikDmuchawaZwlokaOnOff.changeMode(Mode::ON);
                        PodajnikDmuchawaZwlokaOnOff.taskFinished();
                        WatchdogPodajnikDmuchawa.Feed();
                        cykl_podajnik_zwloka = true;
                    }
                }

                if (cykl_pracy_podajnik_dmuchawa) // Praca dmuchawy i cykl podajnika w zależności od temperatury i monitorowanie watchdoga
                {
                    if (TemmpPiecEko1.Get() < getConfigParameterTemp(6, con_kotl))
                    {
                        // DMUCHAWA
                        PodajnikDmuchawaZwlokaOnOff.changeMode(Mode::ON);

                        if (!PodajnikDmuchawaZwlokaOnOff.isFinished())
                        {
                            if (!PodajnikDmuchawaZwlokaOnOff.IsRunning())
                            {
                                PodajnikDmuchawaZwlokaOnOff.Set();
                                SendData("Rozpocznij odliczanie załączenia dmuchawa podajnika i podajnik");
                            }

                            if (PodajnikDmuchawaZwlokaOnOff.IsTimeout())
                            {
                                DmuchawaPodajnik.Set(true);
                                PodajnikDmuchawaZwlokaOnOff.taskFinished();
                                WatchdogPodajnikDmuchawa.Feed();
                                cykl_podajnik_zwloka = true;
                                SendData("Zakończ odliczanie załączenia dmuchawa podajnika i podajnik");
                            }
                        }
                        else // Praca watchdoga, tylko po załączeniu, reset starej wartości czasu wykonywany w funkcji załączającej
                        {
                            // WATCHDOG
                            if (WatchdogPodajnikDmuchawa.IsTimeout())
                            {
                                SendData("Wyłączenie przez watchdoga dmuchawa podajnika i podajnik");
                                DmuchawaPodajnik.Set(false);
                                Podajnik.Set(false);
                                cykl_pracy_podajnik_dmuchawa = false;
                                cykl_podajnik_zwloka = false;
                            }
                        }
                    }
                    else
                    {
                        PodajnikDmuchawaZwlokaOnOff.changeMode(Mode::OFF);

                        if (!PodajnikDmuchawaZwlokaOnOff.isFinished())
                        {
                            if (!PodajnikDmuchawaZwlokaOnOff.IsRunning())
                            {
                                PodajnikDmuchawaZwlokaOnOff.Set();
                                SendData("Rozpocznij odliczanie wyłączenia dmuchawa podajnika i podajnik");
                            }

                            if (PodajnikDmuchawaZwlokaOnOff.IsTimeout())
                            {
                                DmuchawaPodajnik.Set(false);
                                PodajnikDmuchawaZwlokaOnOff.taskFinished();
                                cykl_podajnik_zwloka = false;
                                SendData("Zakończ odliczanie wyłączenia dmuchawa podajnika i podajnik");
                            }
                        }
                    }

                    // PODAJNIK
                    if (cykl_podajnik_zwloka)
                    {
                        if (!CyklPodajnikPrzerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                        {
                            if (!CyklPodajnikPraca.IsRunning())
                            {
                                SendData("Podajnik działa w cyklu (ON)");
                                CyklPodajnikPraca.Set();
                                Podajnik.Set(true);
                            }

                            if (CyklPodajnikPraca.IsTimeout())
                            {
                                SendData("Podajnik działa w cyklu (OFF)");
                                CyklPodajnikPraca.Reset();
                                CyklPodajnikPrzerwa.Set();
                                Podajnik.Set(false);
                            }
                        }

                        if (!CyklPodajnikPraca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                        {

                            if (!CyklPodajnikPrzerwa.IsRunning())
                            {
                                SendData("Podajnik działa w cyklu (OFF)");
                                CyklPodajnikPrzerwa.Set();
                                Podajnik.Set(false);
                            }

                            if (CyklPodajnikPrzerwa.IsTimeout())
                            {
                                SendData("Podajnik działa w cyklu (ON)");
                                CyklPodajnikPrzerwa.Reset();
                                CyklPodajnikPraca.Set();
                                Podajnik.Set(true);
                            }
                        }
                    }
                    else if (CyklPodajnikPrzerwa.IsRunning() || CyklPodajnikPrzerwa.IsRunning()) // jeżeli zmieniono wartość zmiennej cyklu podajnika, przerwał on swoją pracę, jednak trzeba zadbać o zresetowanie i wyłączenie 
                    {
                        CyklPodajnikPrzerwa.Reset();
                        CyklPodajnikPraca.Reset();
                        Podajnik.Set(false);
                    }

                    if (PodajnikDmuchawaOff.Get())
                    {
                        SendData("Wyłączenie przez przycisk OFF dmuchawa podajnika i podajnik");
                        DmuchawaPodajnik.Set(false);
                        Podajnik.Set(false);
                        cykl_pracy_podajnik_dmuchawa = false;
                        cykl_podajnik_zwloka = false;
                    }
                }
            }

            // ############################################
            //  Tryb id 9 - praca piece gazowe
            // ############################################
            if (getConfigParameterMode(9, con_kotl))
            {
                /**
                 * @brief Jeżeli temperatura basenu lub solarów mniejsza od zadanej, załącz piec gazowy
                 *        Jeżeli po czasie dalej temperatura niższa, załącz drugi piec gazowy i pracuj aż do uzyskania temperatury
                 *
                 */
                if (TempCwu.Get() < getConfigParameterTemp(7, con_serw) || TempBasen.Get() < getConfigParameterTemp(6, con_serw))
                {
                    PieceGazoweZwlokaOnOff.changeMode(Mode::ON);

                    if (!PieceGazoweZwlokaOnOff.isFinished())   // odliczanie załączenia programu pieców
                    {
                        if (!PieceGazoweZwlokaOnOff.IsRunning())
                        {
                            PieceGazoweZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie załączenia piecow gazowych");
                        }

                        if (PieceGazoweZwlokaOnOff.IsTimeout())
                        {
                            if (getWorkingParameter(1, con_kotl) > getWorkingParameter(2, con_kotl)) 
                            {
                                PiecGazowy2.Set(true);
                                PracaPiecGazowy2.Set();

                                SendData("Załączenie pieca 2");
                                f_piece_aktywne = F_PIECE_GAZOWE::PIEC_2;
                            }
                            else
                            {
                                PiecGazowy1.Set(true);
                                PracaPiecGazowy1.Set();

                                SendData("Załączenie pieca 1");
                                f_piece_aktywne = F_PIECE_GAZOWE::PIEC_1;
                            }
                            ZalaczDrugiPiecTimer.Set();
                            PieceGazoweZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie załączenia piecow gazowych");
                        }
                    }
                    else if (ZalaczDrugiPiecTimer.IsTimeout()) // odliczanie załączenia drugiego pieca
                    {
                        if (f_piece_aktywne == F_PIECE_GAZOWE::PIEC_1)
                        {
                            SendData("Potrzebny dodatkowy piec. Załączenie pieca 2");
                            PiecGazowy2.Set(true);
                            PracaPiecGazowy2.Set();
                        }
                        else
                        {
                            SendData("Potrzebny dodatkowy piec. Załączenie pieca 1");
                            PiecGazowy1.Set(true);
                            PracaPiecGazowy1.Set();
                        }

                        ZalaczDrugiPiecTimer.Reset();
                        f_piece_aktywne = F_PIECE_GAZOWE::OBA_PIECE;
                    }
                }
                else
                {
                    PieceGazoweZwlokaOnOff.changeMode(Mode::OFF);

                    if (!PieceGazoweZwlokaOnOff.isFinished())   // odliczanie wyłączenia programu pieców
                    {
                        if (!PieceGazoweZwlokaOnOff.IsRunning())
                        {
                            PieceGazoweZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie wyłączenia piecow gazowych");
                        }

                        if (PieceGazoweZwlokaOnOff.IsTimeout())
                        {
                            if (f_piece_aktywne == F_PIECE_GAZOWE::PIEC_1)
                            {
                                incraseWorkingParameter(1, con_kotl, PracaPiecGazowy1.getElapsedSec());
                            }
                            else if (f_piece_aktywne == F_PIECE_GAZOWE::PIEC_2)
                            {
                                incraseWorkingParameter(2, con_kotl, PracaPiecGazowy2.getElapsedSec());
                            }
                            else if (f_piece_aktywne == F_PIECE_GAZOWE::OBA_PIECE)
                            {
                                incraseWorkingParameter(1, con_kotl, PracaPiecGazowy1.getElapsedSec());
                                incraseWorkingParameter(2, con_kotl, PracaPiecGazowy2.getElapsedSec());
                            }

                            PiecGazowy1.Set(false);
                            PiecGazowy2.Set(false);
                            PieceGazoweZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie wyłączenia piecow gazowych");
                        }
                    }
                }
            }

            // ############################################
            //  Tryb id 10 - praca pompy wymiennika basen z c.o.
            // ############################################
            if (getConfigParameterMode(10, con_kotl))
            {
                /**
                 * @brief Jeżeli temperatura sprzęgła większa od minimalnej oraz temperatura basenu mniejsza niż zadana, załącz wymiennik
                 *
                 */
                if (TempSprzeglo.Get() >= getConfigParameterTemp(1, con_kotl) && TempBasen.Get() < getConfigParameterTemp(6, con_serw))
                {
                    WymiennikBasenZwlokaOnOff.changeMode(Mode::ON);

                    if (!WymiennikBasenZwlokaOnOff.isFinished())
                    {
                        if (!WymiennikBasenZwlokaOnOff.IsRunning())
                        {
                            WymiennikBasenZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie załączenia pompy wymiennika basen z c.o.");
                        }

                        if (WymiennikBasenZwlokaOnOff.IsTimeout())
                        {
                            PompaWymiennikBasen.Set(true);
                            WymiennikBasenZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie załączenia pompy wymiennika basen z c.o.");
                        }
                    }
                }
                else
                {
                    WymiennikBasenZwlokaOnOff.changeMode(Mode::OFF);

                    if (!WymiennikBasenZwlokaOnOff.isFinished())
                    {
                        if (!WymiennikBasenZwlokaOnOff.IsRunning())
                        {
                            WymiennikBasenZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie wyłączenia pompy wymiennika basen z c.o.");
                        }

                        if (WymiennikBasenZwlokaOnOff.IsTimeout())
                        {
                            PompaWymiennikBasen.Set(false);
                            WymiennikBasenZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie wyłączenia pompy wymiennika basen z c.o.");
                        }
                    }
                }
            }

            // ############################################
            //  Tryb id 10 - praca czasowa studni
            // ############################################
            if (getConfigParameterMode(11, con_kotl))
            {
                if (!CyklStudniPrzerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                {
                    if (!CyklStudniPraca.IsRunning())
                    {
                        SendData("Załączenie pracy pompy studni (ON)");
                        CyklStudniPraca.Set();
                        PompaStudnia.Set(true);
                    }

                    if (CyklStudniPraca.IsTimeout())
                    {
                        SendData("Załączenie przerwy pompy studni (OFF)");
                        CyklStudniPraca.Reset();
                        CyklStudniPrzerwa.Set();
                        PompaStudnia.Set(false);
                    }
                }

                if (!CyklStudniPraca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                {
                    if (!CyklStudniPrzerwa.IsRunning())
                    {
                        SendData("Załączenie przerwy pompy studni (OFF)");
                        CyklStudniPrzerwa.Set();
                        PompaStudnia.Set(false);
                    }

                    if (CyklStudniPrzerwa.IsTimeout())
                    {
                        SendData("Załączenie pracy pompy studni (ON)");
                        CyklStudniPrzerwa.Reset();
                        CyklStudniPraca.Set();
                        PompaStudnia.Set(true);
                    }
                }
            }

            // ############################################
            //  Tryb id 12 - praca pompa 4 - grzejniki parter B2
            // ############################################
            if (getConfigParameterMode(12, con_kotl))
            {
                /**
                 * @brief Jeżeli temperatura sprzęgła większa od minimalnej załącz cykl grzania, jeżeli mniejsza - odliczaj wyłączenie cyklu
                 *
                 */
                if (TempSprzeglo.Get() >= getConfigParameterTemp(1, con_kotl) || Pompa4Wylaczenie.IsRunning())
                {

                    if (!CyklPompa4Przerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                    {

                        if (!CyklPompa4Praca.IsRunning())
                        {
                            SendData("Załączenie cyklu pompa 4 (ON)");
                            CyklPompa4Praca.Set();
                            Pompa4.Set(true);
                        }

                        if (CyklPompa4Praca.IsTimeout())
                        {
                            SendData("Załączenie cyklu pompa 4 (OFF)");
                            CyklPompa4Praca.Reset();
                            CyklPompa4Przerwa.Set();
                            Pompa4.Set(false);
                        }
                    }

                    if (!CyklPompa4Praca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                    {

                        if (!CyklPompa4Przerwa.IsRunning())
                        {
                            SendData("Załączenie cyklu pompa 4 (OFF)");
                            CyklPompa4Przerwa.Set();
                            Pompa4.Set(false);
                        }

                        if (CyklPompa4Przerwa.IsTimeout())
                        {
                            SendData("Załączenie cyklu pompa 4 (ON)");
                            CyklPompa4Przerwa.Reset();
                            CyklPompa4Praca.Set();
                            Pompa4.Set(true);
                        }
                    }
                }

                if (TempSprzeglo.Get() < getConfigParameterTemp(1, con_kotl))
                {
                    if (CyklPompa4Praca.IsRunning() || CyklPompa4Przerwa.IsRunning())
                    {
                        if (!Pompa4Wylaczenie.IsRunning())
                        {
                            SendData("Rozpoczęcie odliczania wyłączenia cyklu pompa 4");
                            Pompa4Wylaczenie.Set();
                        }

                        if (Pompa4Wylaczenie.IsTimeout())
                        {
                            SendData("Wyłączenie cyklu pompa 4");
                            Pompa4Wylaczenie.Reset();
                            CyklPompa4Praca.Reset();
                            CyklPompa4Przerwa.Reset();
                            Pompa4.Set(false);
                        }
                    }
                }
                else if (Pompa4Wylaczenie.IsRunning())
                {
                    SendData("Reset odliczania wyłączenia pompa 4");
                    Pompa4Wylaczenie.Reset();
                }
            }

            // ############################################
            //  Tryb id 13 - praca pompa 5 - podłoga parter B2
            // ############################################
            if (getConfigParameterMode(12, con_kotl))
            {
                /**
                 * @brief Jeżeli temperatura sprzęgła większa od minimalnej załącz cykl grzania, jeżeli mniejsza - odliczaj wyłączenie cyklu
                 *
                 */
                if (TempSprzeglo.Get() >= getConfigParameterTemp(1, con_kotl) || Pompa5Wylaczenie.IsRunning())
                {

                    if (!CyklPompa5Przerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                    {

                        if (!CyklPompa5Praca.IsRunning())
                        {
                            SendData("Załączenie cyklu pompa 5 (ON)");
                            CyklPompa5Praca.Set();
                            Pompa5.Set(true);
                        }

                        if (CyklPompa5Praca.IsTimeout())
                        {
                            SendData("Załączenie cyklu pompa 5 (OFF)");
                            CyklPompa5Praca.Reset();
                            CyklPompa5Przerwa.Set();
                            Pompa5.Set(false);
                        }
                    }

                    if (!CyklPompa5Praca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                    {

                        if (!CyklPompa5Przerwa.IsRunning())
                        {
                            SendData("Załączenie cyklu pompa 5 (OFF)");
                            CyklPompa5Przerwa.Set();
                            Pompa5.Set(false);
                        }

                        if (CyklPompa5Przerwa.IsTimeout())
                        {
                            SendData("Załączenie cyklu pompa 5 (ON)");
                            CyklPompa5Przerwa.Reset();
                            CyklPompa5Praca.Set();
                            Pompa5.Set(true);
                        }
                    }
                }

                if (TempSprzeglo.Get() < getConfigParameterTemp(1, con_kotl))
                {
                    if (CyklPompa5Praca.IsRunning() || CyklPompa5Przerwa.IsRunning())
                    {
                        if (!Pompa5Wylaczenie.IsRunning())
                        {
                            SendData("Rozpoczęcie odliczania wyłączenia cyklu pompa 5");
                            Pompa5Wylaczenie.Set();
                        }

                        if (Pompa5Wylaczenie.IsTimeout())
                        {
                            SendData("Wyłączenie cyklu pompa 5");
                            Pompa5Wylaczenie.Reset();
                            CyklPompa5Praca.Reset();
                            CyklPompa5Przerwa.Reset();
                            Pompa5.Set(false);
                        }
                    }
                }
                else if (Pompa5Wylaczenie.IsRunning())
                {
                    SendData("Reset odliczania wyłączenia pompa 5");
                    Pompa5Wylaczenie.Reset();
                }
            }


            // ############################################
            //  Tryb id 14 - płukanie filtra wody
            // ############################################
            if (getConfigParameterMode(14, con_kotl))
            {
                if (!CyklPlukanieFiltraWodyPrzerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                {
                    if (!CyklPlukanieFiltraWodyPraca.IsRunning())
                    {
                        SendData("Załączenie pracy plukania pompy wody (ON)");
                        CyklPlukanieFiltraWodyPraca.Set();
                        PlukanieFiltraWody.Set(true);
                    }

                    if (CyklPlukanieFiltraWodyPraca.IsTimeout())
                    {
                        SendData("Załączenie przerwy plukania pompy wody (OFF)");
                        CyklPlukanieFiltraWodyPraca.Reset();
                        CyklPlukanieFiltraWodyPrzerwa.Set();
                        PlukanieFiltraWody.Set(false);
                    }
                }

                if (!CyklPlukanieFiltraWodyPraca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                {
                    if (!CyklPlukanieFiltraWodyPrzerwa.IsRunning())
                    {
                        SendData("Załączenie przerwy plukania pompy wody (OFF)");
                        CyklPlukanieFiltraWodyPrzerwa.Set();
                        PlukanieFiltraWody.Set(false);
                    }

                    if (CyklPlukanieFiltraWodyPrzerwa.IsTimeout())
                    {
                        SendData("Załączenie pracy plukania pompy wody (ON)");
                        CyklPlukanieFiltraWodyPrzerwa.Reset();
                        CyklPlukanieFiltraWodyPraca.Set();
                        PlukanieFiltraWody.Set(true);
                    }
                }
            }

            // ############################################
            //  Tryb id 15 - płukanie hydrofor
            // ############################################
            if (getConfigParameterMode(15, con_kotl))
            {
                if (!CyklPlukanieHydroforPrzerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                {
                    if (!CyklPlukanieHydroforPraca.IsRunning())
                    {
                        SendData("Załączenie pracy plukania hydroforu (ON)");
                        CyklPlukanieHydroforPraca.Set();
                        PlukanieHydrofor.Set(true);
                    }

                    if (CyklPlukanieHydroforPraca.IsTimeout())
                    {
                        SendData("Załączenie przerwy plukania hydroforu (OFF)");
                        CyklPlukanieHydroforPraca.Reset();
                        CyklPlukanieHydroforPrzerwa.Set();
                        PlukanieHydrofor.Set(false);
                    }
                }

                if (!CyklPlukanieHydroforPraca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                {
                    if (!CyklPlukanieHydroforPrzerwa.IsRunning())
                    {
                        SendData("Załączenie przerwy plukania hydroforu (OFF)");
                        CyklPlukanieHydroforPrzerwa.Set();
                        PlukanieHydrofor.Set(false);
                    }

                    if (CyklPlukanieHydroforPrzerwa.IsTimeout())
                    {
                        SendData("Załączenie pracy plukania hydroforu (ON)");
                        CyklPlukanieHydroforPrzerwa.Reset();
                        CyklPlukanieHydroforPraca.Set();
                        PlukanieHydrofor.Set(true);
                    }
                }
            }

            // ############################################
            //  Tryb id 16 - odwodnienie budynku
            // ############################################
            if (getConfigParameterMode(16, con_kotl))
            {
                if (!CyklOdwodnienieBudynkuPrzerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                {
                    if (!CyklOdwodnienieBudynkuPraca.IsRunning())
                    {
                        SendData("Załączenie pracy odwodnienie cudynku (ON)");
                        CyklOdwodnienieBudynkuPraca.Set();
                        OdwodnienieBudynku.Set(true);
                    }

                    if (CyklOdwodnienieBudynkuPraca.IsTimeout())
                    {
                        SendData("Załączenie przerwy odwodnienie cudynku (OFF)");
                        CyklOdwodnienieBudynkuPraca.Reset();
                        CyklOdwodnienieBudynkuPrzerwa.Set();
                        OdwodnienieBudynku.Set(false);
                    }
                }

                if (!CyklOdwodnienieBudynkuPraca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                {
                    if (!CyklOdwodnienieBudynkuPrzerwa.IsRunning())
                    {
                        SendData("Załączenie przerwy odwodnienie cudynku (OFF)");
                        CyklOdwodnienieBudynkuPrzerwa.Set();
                        OdwodnienieBudynku.Set(false);
                    }

                    if (CyklOdwodnienieBudynkuPrzerwa.IsTimeout())
                    {
                        SendData("Załączenie pracy odwodnienie cudynku (ON)");
                        CyklOdwodnienieBudynkuPrzerwa.Reset();
                        CyklOdwodnienieBudynkuPraca.Set();
                        OdwodnienieBudynku.Set(true);
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
