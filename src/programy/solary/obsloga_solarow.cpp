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

    OutputPinTask PompaSolarBasen(77, con_serw);
    TemperatureTask TempBasen(7, con_serw);
    DelayOnOffTimer PompaSolarBasenZwlokaOnOff(getConfigParameterTime(1, con_serw));

    OutputPinTask PompaSolar(78, con_serw);
    TemperatureTask TempSolary(8, con_serw);
    DelayOnOffTimer PompaSolarZwlokaOnOff(getConfigParameterTime(3, con_serw));

    OutputPinTask PompaSolarCWU(79, con_serw);
    TemperatureTask TempCwu(6, con_serw);
    DelayOnOffTimer PompaSolarCWUZwlokaOnOff(getConfigParameterTime(2, con_serw));

    OutputPinTask Pompa10(138, con_kotl);
    OutputPinTask Pompa11(139, con_kotl);
    OutputPinTask PompaCWU(144, con_kotl);
    DelayOnOffTimer SolarPrzegrzanieZwlokaOnOff(getConfigParameterTime(4, con_serw));

    OutputPinTask Pompa5(132, con_kotl);
    DelayOnOffTimer SolarPracaZimowaZwlokaOnOff(getConfigParameterTime(16, con_serw));

    float roznica_temp = getConfigParameterTemp(5, con_serw);

    SendData("");
    cerr << "główna pętla programu" << endl;

    for (;;)
    {
        // SendData("");
        //     std::cout << "Loop\n";
        try
        {

            // ############################################
            //  Tryb id 1 - praca pompy solarów do grzania cwu
            // ############################################
            if (getConfigParameterMode(1, con_serw))
            {
                /**
                 * @brief Jeżeli pompa solarów działa i temperatura solarów większa niż cwu + histereza i mniejsza niż zadana załącz grzanie
                 *
                 */
                if ((PompaSolar.Get() > 0) && (TempCwu.Get() + roznica_temp < TempSolary.Get()) && (TempCwu.Get() < getConfigParameterTemp(4, con_serw)))
                {
                    PompaSolarCWUZwlokaOnOff.changeMode(Mode::ON);

                    if (!PompaSolarCWUZwlokaOnOff.isFinished())
                    {
                        if (!PompaSolarCWUZwlokaOnOff.IsRunning())
                        {
                            PompaSolarCWUZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie załączenia pompy solarow CWU");
                        }

                        if (PompaSolarCWUZwlokaOnOff.IsTimeout())
                        {
                            PompaSolarCWU.Set(true);
                            PompaSolarCWUZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie pompy solarow CWU");
                        }
                    }
                }
                else
                {
                    PompaSolarCWUZwlokaOnOff.changeMode(Mode::OFF);

                    if (!PompaSolarCWUZwlokaOnOff.isFinished())
                    {
                        if (!PompaSolarCWUZwlokaOnOff.IsRunning())
                        {
                            PompaSolarCWUZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie wyłączenia pompy solarow CWU");
                        }

                        if (PompaSolarCWUZwlokaOnOff.IsTimeout())
                        {
                            PompaSolarCWU.Set(false);
                            PompaSolarCWUZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie wyłączenia pompy solarow CWU");
                        }
                    }
                }
            }

            // ############################################
            //  Tryb id 2 - praca pompy solarów do grzania basenu
            // ############################################
            if (getConfigParameterMode(2, con_serw))
            {
                /**
                 * @brief Jeżeli pompa solarów działa i temperatura solarów większa niż basen + histereza i mniejsza niż zadana załącz grzanie
                 *
                 */
                if ((PompaSolar.Get() > 0) && (TempBasen.Get() + roznica_temp < TempSolary.Get()) && (TempBasen.Get() < getConfigParameterTemp(3, con_serw)))
                {
                    PompaSolarBasenZwlokaOnOff.changeMode(Mode::ON);

                    if (!PompaSolarBasenZwlokaOnOff.isFinished())
                    {
                        if (!PompaSolarBasenZwlokaOnOff.IsRunning())
                        {
                            PompaSolarBasenZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie załączenia pompy solarow basen");
                        }

                        if (PompaSolarBasenZwlokaOnOff.IsTimeout())
                        {
                            PompaSolarBasen.Set(true);
                            PompaSolarBasenZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie pompy solarow basen");
                        }
                    }
                }
                else
                {
                    PompaSolarBasenZwlokaOnOff.changeMode(Mode::OFF);

                    if (!PompaSolarBasenZwlokaOnOff.isFinished())
                    {
                        if (!PompaSolarBasenZwlokaOnOff.IsRunning())
                        {
                            PompaSolarBasenZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie wyłączenia pompy solarow basen");
                        }

                        if (PompaSolarBasenZwlokaOnOff.IsTimeout())
                        {
                            PompaSolarBasen.Set(false);
                            PompaSolarBasenZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie wyłączenia pompy solarow basen");
                        }
                    }
                }
            }

            // ############################################
            //  Tryb id 4 - praca pompy solarów
            // ############################################
            if (getConfigParameterMode(4, con_serw))
            {
                /**
                 * @brief Jeżeli temperatura solarów większa od zadanej załącz pompe solarów
                 *
                 */
                if (TempSolary.Get() > getConfigParameterTemp(1, con_serw))
                {
                    PompaSolarZwlokaOnOff.changeMode(Mode::ON);

                    if (!PompaSolarZwlokaOnOff.isFinished())
                    {
                        if (!PompaSolarZwlokaOnOff.IsRunning())
                        {
                            PompaSolarZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie załączenia pompy solarow");
                        }

                        if (PompaSolarZwlokaOnOff.IsTimeout())
                        {
                            PompaSolar.Set(true);
                            PompaSolarZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie pompy solarow");
                        }
                    }
                }
                else
                {
                    PompaSolarZwlokaOnOff.changeMode(Mode::OFF);

                    if (!PompaSolarZwlokaOnOff.isFinished())
                    {
                        if (!PompaSolarZwlokaOnOff.IsRunning())
                        {
                            PompaSolarZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie wyłączenia pompy solarow");
                        }

                        if (PompaSolarZwlokaOnOff.IsTimeout())
                        {
                            PompaSolar.Set(false);
                            PompaSolarZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie wyłączenia pompy solarow");
                        }
                    }
                }
            }

            // ############################################
            //  Tryb id 4 - załączenie programu przegrzania solarów
            // ############################################
            if (getConfigParameterMode(5, con_serw))
            {
                /**
                 * @brief Jeżeli temperatura solarów większa od maksymalnej załącz pompe cwu, wymiennik i podłogę basen z kotłowni
                 *
                 */
                if (TempSolary.Get() > getConfigParameterTemp(2, con_serw))
                {
                    SolarPrzegrzanieZwlokaOnOff.changeMode(Mode::ON);

                    if (!SolarPrzegrzanieZwlokaOnOff.isFinished())
                    {
                        if (!SolarPrzegrzanieZwlokaOnOff.IsRunning())
                        {
                            SolarPrzegrzanieZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie załączenia pomp przegrzania solarów");
                        }

                        if (SolarPrzegrzanieZwlokaOnOff.IsTimeout())
                        {
                            PompaCWU.Set(true);
                            Pompa10.Set(true);
                            Pompa11.Set(true);
                            SolarPrzegrzanieZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie załączenia pomp przegrzania solarów");
                        }
                    }
                }
                else
                {
                    SolarPrzegrzanieZwlokaOnOff.changeMode(Mode::OFF);

                    if (!SolarPrzegrzanieZwlokaOnOff.isFinished())
                    {
                        if (!SolarPrzegrzanieZwlokaOnOff.IsRunning())
                        {
                            SolarPrzegrzanieZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie wyłączenia pomp przegrzania solarów");
                        }

                        if (SolarPrzegrzanieZwlokaOnOff.IsTimeout())
                        {
                            PompaCWU.Set(false);
                            Pompa10.Set(false);
                            Pompa11.Set(false);
                            SolarPrzegrzanieZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie wyłączenia pomp przegrzania solarów");
                        }
                    }
                }
            }

            // ############################################
            //  Tryb id 12 - praca solarów w okresie zimowym
            // ############################################
            if (getConfigParameterMode(12, con_serw))
            {
                /**
                 * @brief Jeżeli temperatura solarów większa od min i cwu załącz grzanie cwu w trybie 1
                 *        Jeżeli temperatura cwu większa od maksymalnej w okresie zimowym, załącz pompę wymiennika z kotłowni oraz pompę odbierającą ciepło z wymiennika
                 *
                 */
                if (TempCwu.Get() > getConfigParameterTemp(8, con_serw))
                {
                    SolarPracaZimowaZwlokaOnOff.changeMode(Mode::ON);

                    if (!SolarPracaZimowaZwlokaOnOff.isFinished())
                    {
                        if (!SolarPracaZimowaZwlokaOnOff.IsRunning())
                        {
                            SolarPracaZimowaZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie załączenia pomp pracy zimowej solarów");
                        }

                        if (SolarPracaZimowaZwlokaOnOff.IsTimeout())
                        {
                            PompaCWU.Set(true);
                            Pompa5.Set(true);
                            SolarPracaZimowaZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie załączenia pomp pracy zimowej solarów");
                        }
                    }
                }
                else
                {
                    SolarPracaZimowaZwlokaOnOff.changeMode(Mode::OFF);

                    if (!SolarPracaZimowaZwlokaOnOff.isFinished())
                    {
                        if (!SolarPracaZimowaZwlokaOnOff.IsRunning())
                        {
                            SolarPracaZimowaZwlokaOnOff.Set();
                            SendData("Rozpocznij odliczanie wyłączenia pomp pracy zimowej solarów");
                        }

                        if (SolarPracaZimowaZwlokaOnOff.IsTimeout())
                        {
                            PompaCWU.Set(false);
                            Pompa5.Set(false);
                            SolarPracaZimowaZwlokaOnOff.taskFinished();
                            SendData("Zakończ odliczanie wyłączenia pomp pracy zimowej solarów");
                        }
                    }
                }
            }
        }
        catch (const SqlException &e)
        {
            std::cerr << e.what() << '\n';
            std::cerr << "Restarting connection..." << '\n';
            ZakonczBaza(&con_serw);
            ZakonczBaza(&con_kotl);
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
