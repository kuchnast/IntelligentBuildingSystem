#include <iostream>
#include <mysql.h>
#include <fstream>
#include <string>
#include <wiringPi.h>
#include <chrono>
#include <ratio>
#include <thread>
#include <ctime>
#include <vector>

#include "programy/obsluga_zadan.hh"
#include "komunikacja/baza.hh"
#include "konfiguracja.hh"

using namespace std::chrono;

#define CZAS_ZWLOKI 1000

using std::cerr;
using std::endl;

// Na wejście pierwsza baza to schody, druga to serwerownia

int main(int argc, const char *argv[])
{
    MYSQL *con_scho;
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

    if (!InicjujBaza(&con_scho, dane_logowania[0]))
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



    std::vector<OutputPinTask> OswietlenieZewCzasowe;
    OswietlenieZewCzasowe.push_back(OutputPinTask(50, con_scho));
    OswietlenieZewCzasowe.push_back(OutputPinTask(84, con_serw));
    OswietlenieZewCzasowe.push_back(OutputPinTask(85, con_serw));
    OswietlenieZewCzasowe.push_back(OutputPinTask(86, con_serw));
    Mode f_oswietlenieZewCzasowe = Mode::FIRST_RUN;

    std::vector<OutputPinTaskWithBlock> OswietlenieZewRuch;
    OswietlenieZewRuch.push_back(OutputPinTaskWithBlock(60, con_scho));
    OswietlenieZewRuch.push_back(OutputPinTaskWithBlock(65, con_scho));
    OswietlenieZewRuch.push_back(OutputPinTaskWithBlock(79, con_scho));
  	OswietlenieZewRuch.push_back(OutputPinTaskWithBlock(47, con_serw));
    OswietlenieZewRuch.push_back(OutputPinTaskWithBlock(54, con_serw));
    OswietlenieZewRuch.push_back(OutputPinTaskWithBlock(94, con_serw));
    OswietlenieZewRuch.push_back(OutputPinTaskWithBlock(95, con_serw));
    OswietlenieZewRuch.push_back(OutputPinTaskWithBlock(97, con_serw));
    OswietlenieZewRuch.push_back(OutputPinTaskWithBlock(98, con_serw));
    OswietlenieZewRuch.push_back(OutputPinTaskWithBlock(111, con_serw));
    Mode f_oswietlenieZewRuch = Mode::FIRST_RUN;

    OutputPinTask RadioBasen(119, con_serw);
    Mode f_radio_basen = Mode::FIRST_RUN;

    OutputPinTask RadioBudynek(118, con_serw);
    Mode f_radio_budynek = Mode::FIRST_RUN;

    OutputPinTask BlokadaWejscBasen(39, con_scho);
    Mode f_blokada_wejsc_basen = Mode::FIRST_RUN;

    std::vector<OutputPinTaskWithBlock> OswietlenieBasen;
    OswietlenieBasen.push_back(OutputPinTaskWithBlock(55, con_serw));
    OswietlenieBasen.push_back(OutputPinTaskWithBlock(58, con_serw));
    OswietlenieBasen.push_back(OutputPinTaskWithBlock(59, con_serw));
    OswietlenieBasen.push_back(OutputPinTaskWithBlock(60, con_serw));
    OswietlenieBasen.push_back(OutputPinTaskWithBlock(61, con_serw));
    OswietlenieBasen.push_back(OutputPinTaskWithBlock(72, con_serw));
    OswietlenieBasen.push_back(OutputPinTaskWithBlock(73, con_serw));
    OswietlenieBasen.push_back(OutputPinTaskWithBlock(74, con_serw));
    OswietlenieBasen.push_back(OutputPinTaskWithBlock(75, con_serw));
    Mode f_oswietlenieBasen = Mode::FIRST_RUN;

    DelayOnOffTimer OswietlenieBasenTrybMiganie(getConfigParameterTime(19, con_serw), Mode::OFF);
    DelayOnOffTimer OswietlenieBasenMiganiePraca(5);
    DelayOnOffTimer OswietlenieBasenMiganiePrzerwa(5);
    Mode f_zakonczono_miganie_swiatel_basen = Mode::FIRST_RUN;

    DelayOnOffTimer OknoLacznikPracaOtwieranie(getConfigParameterTime(11, con_scho));
    DelayOnOffTimer OknoLacznikPracaZamykanie(getConfigParameterTime(12, con_scho));
    OutputPinTask OknoLacznikOtwieranie(6, con_serw);
    OutputPinTask OknoLacznikOtwieranieZamykanie(7, con_serw);
    Mode f_okno_lacznik = Mode::FIRST_RUN;

    OutputPinTask WentylatorMagazynekPoscieli(14, con_serw);
    CountdownTimer CyklWentylatorMagazynekPoscieliPraca(getConfigParameterTime(20, con_serw));
    CountdownTimer CyklWentylatorMagazynekPoscieliPrzerwa(getConfigParameterTime(21, con_serw));

    OutputPinTaskWithBlock OswietlenieLacznik(5, con_serw);
    Mode f_oswietlenieLacznik= Mode::FIRST_RUN;

	OutputPinTask PodlewanieSzklarnia(103, con_serw);
  	Mode f_podlewanie_szklarnia = Mode::FIRST_RUN;

	OutputPinTask PodlewanieGrzadka(102, con_serw);
  	Mode f_podlewanie_grzadka = Mode::FIRST_RUN;

	OutputPinTask PodlewanieGrzadkaDodatkowa(101, con_serw);
  	Mode f_podlewanie_grzadka_dodatkowa = Mode::FIRST_RUN;

    int now_sec;

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
            //  Tryb id 1 - działanie czasowego załączenia oświetlenia zewnętrznego
            // ############################################
            if (getConfigParameterMode(1, con_scho))
            {
                if (now_sec > getConfigParameterTime(1, con_scho) || now_sec < getConfigParameterTime(2, con_scho))
                {
                    if (f_oswietlenieZewCzasowe != Mode::ON)
                    {
                        for (auto &o : OswietlenieZewCzasowe)
                            o.Set(true);

                        f_oswietlenieZewCzasowe = Mode::ON;
                        SendData("Zalaczono czasowe oswietlenie zewnetrzne");
                    }
                }
                else
                {
                    if (f_oswietlenieZewCzasowe != Mode::OFF)
                    {
                        for (auto &o : OswietlenieZewCzasowe)
                            o.Set(false);

                        f_oswietlenieZewCzasowe = Mode::OFF;
                        SendData("Wylaczono czasowe oswietlenie zewnetrzne");
                    }
                }
            }
            else if (f_oswietlenieZewCzasowe == Mode::ON)
            {
                for (auto &o : OswietlenieZewCzasowe)
                    o.Set(false);

                f_oswietlenieZewCzasowe = Mode::OFF;
                SendData("Wylaczono czasowe oswietlenie zewnetrzne");
            }

            // ############################################
            //  Tryb id 2 - działanie oświetlenia zewnętrznego na ruch
            // ############################################
            if (getConfigParameterMode(2, con_scho))
            {
                if (now_sec > getConfigParameterTime(3, con_scho) && now_sec < getConfigParameterTime(4, con_scho))
                {
                    if (f_oswietlenieZewRuch != Mode::ON)
                    {
                        for (auto &o : OswietlenieZewRuch)
                            o.BlockAndSet(false);

                        f_oswietlenieZewRuch = Mode::ON;
                        SendData("Wylaczono oswietlenie zewnetrzne aktywne na ruch");
                    }
                }
                else
                {
                    if (f_oswietlenieZewRuch != Mode::OFF)
                    {
                        for (auto &o : OswietlenieZewRuch)
                            o.Unblock();

                        f_oswietlenieZewRuch = Mode::OFF;
                        SendData("Zalaczono oswietlenie zewnetrzne aktywne na ruch");
                    }
                }
            }
            else if (f_oswietlenieZewRuch == Mode::ON)
            {
                for (auto &o : OswietlenieZewRuch)
                    o.Unblock();

                f_oswietlenieZewRuch = Mode::OFF;
                SendData("Zalaczono oswietlenie zewnetrzne aktywne na ruch");
            }

            // ############################################
            //  Tryb id 3 - działanie radia basen
            // ############################################
            if (getConfigParameterMode(3, con_scho))
            {
                if (now_sec > getConfigParameterTime(5, con_scho) && now_sec < getConfigParameterTime(6, con_scho))
                {
                    if (f_radio_basen != Mode::ON)
                    {
                        RadioBasen.Set(true);
                        f_radio_basen = Mode::ON;
                        SendData("Zalaczono radio basen");
                    }
                }
                else
                {
                    if (f_radio_basen != Mode::OFF)
                    {
                        RadioBasen.Set(false);
                        f_radio_basen = Mode::OFF;
                        SendData("Wylaczono radio basen");
                    }
                }
            }
            else if (f_radio_basen == Mode::ON)
            {
                RadioBasen.Set(false);
                f_radio_basen = Mode::OFF;
                SendData("Wylaczono radio basen");
            }

            // ############################################
            //  Tryb id 4 - działanie radia budynek
            // ############################################
            if (getConfigParameterMode(4, con_scho))
            {
                if (now_sec > getConfigParameterTime(7, con_scho) && now_sec < getConfigParameterTime(8, con_scho))
                {
                    if (f_radio_budynek != Mode::ON)
                    {
                        RadioBudynek.Set(true);
                        f_radio_budynek = Mode::ON;
                        SendData("Zalaczono radio na budynku");
                    }
                }
                else
                {
                    if (f_radio_budynek != Mode::OFF)
                    {
                        RadioBudynek.Set(false);
                        f_radio_budynek = Mode::OFF;
                        SendData("Wylaczono radio na budynku");
                    }
                }
            }
            else if (f_radio_budynek == Mode::ON)
            {
                RadioBudynek.Set(false);
                f_radio_budynek = Mode::OFF;
                SendData("Wylaczono radio na budynku");
            }

            // ############################################
            //  Tryb id 5 - działanie otwarcia okna lacznik
            // ############################################
            if (getConfigParameterMode(5, con_scho))
            {
                if (now_sec > getConfigParameterTime(9, con_scho) && now_sec < getConfigParameterTime(10, con_scho))
                {
                    OknoLacznikPracaOtwieranie.changeMode(Mode::ON);

                    if (!OknoLacznikPracaOtwieranie.isFinished())
                    {
                        if (!OknoLacznikPracaOtwieranie.IsRunning())
                        {
                            OknoLacznikOtwieranie.Set(true);
                            OknoLacznikOtwieranieZamykanie.Set(true);
                            OknoLacznikPracaOtwieranie.Set();
                            f_okno_lacznik = Mode::ON;
                            SendData("Rozpocznij otwieranie okna lacznik");
                        }

                        if (OknoLacznikPracaOtwieranie.IsTimeout())
                        {
                            OknoLacznikOtwieranie.Set(false);
                            OknoLacznikOtwieranieZamykanie.Set(false);
                            OknoLacznikPracaZamykanie.changeMode(Mode::OFF);
                            OknoLacznikPracaOtwieranie.taskFinished();
                            SendData("Zakończ otwieranie okna lacznik");
                        }
                    }
                }
                else
                {
                    OknoLacznikPracaZamykanie.changeMode(Mode::ON);

                    if (!OknoLacznikPracaZamykanie.isFinished())
                    {
                        if (!OknoLacznikPracaZamykanie.IsRunning())
                        {
                            OknoLacznikOtwieranieZamykanie.Set(true);
                            OknoLacznikPracaZamykanie.Set();
                            SendData("Rozpocznij zamykanie okna lacznik");
                        }

                        if (OknoLacznikPracaZamykanie.IsTimeout())
                        {
                            OknoLacznikOtwieranieZamykanie.Set(false);
                            OknoLacznikPracaOtwieranie.changeMode(Mode::OFF);
                            f_okno_lacznik = Mode::OFF;
                            OknoLacznikPracaZamykanie.taskFinished();
                            SendData("Zakończ zamykanie okna lacznik");
                        }
                    }
                }
            }
            else if (f_okno_lacznik == Mode::ON)
            {
                OknoLacznikPracaZamykanie.changeMode(Mode::ON);

                if (!OknoLacznikPracaZamykanie.isFinished())
                {
                    if (!OknoLacznikPracaZamykanie.IsRunning())
                    {
                        OknoLacznikOtwieranie.Set(false);
                        OknoLacznikOtwieranieZamykanie.Set(true);
                        OknoLacznikPracaZamykanie.Set();
                        SendData("Rozpocznij zamykanie okna lacznik");
                    }

                    if (OknoLacznikPracaZamykanie.IsTimeout())
                    {
                        OknoLacznikOtwieranieZamykanie.Set(false);
                        OknoLacznikPracaOtwieranie.changeMode(Mode::OFF);
                        f_okno_lacznik = Mode::OFF;
                        OknoLacznikPracaZamykanie.taskFinished();
                        SendData("Zakończ zamykanie okna lacznik");
                    }
                }
            }

            // ############################################
            //  Tryb id 13 - blokada wejście basen na noc
            // ############################################
            if (getConfigParameterMode(13, con_serw))
            {
                if (now_sec > getConfigParameterTime(17, con_serw) || now_sec < getConfigParameterTime(18, con_serw))
                {
                    if (f_blokada_wejsc_basen != Mode::ON)
                    {
                        BlokadaWejscBasen.Set(true);
                        f_blokada_wejsc_basen = Mode::ON;
                        SendData("Zalaczono blokade wejsc basen - tryb noc");
                    }
                }
                else
                {
                    if (f_blokada_wejsc_basen != Mode::OFF)
                    {
                        BlokadaWejscBasen.Set(false);
                        f_blokada_wejsc_basen = Mode::OFF;
                        SendData("Wylaczono blokade wejsc basen - tryb dzien");
                    }
                }
            }
            else if (f_blokada_wejsc_basen == Mode::ON)
            {
                BlokadaWejscBasen.Set(false);
                f_blokada_wejsc_basen = Mode::OFF;
                SendData("Wylaczono blokade wejsc basen - tryb dzien");
            }

            // ############################################
            //  Tryb id 14 - blokada oświetlenia na ruch basen na noc
            // ############################################
            if (getConfigParameterMode(14, con_serw))
            {
                if (now_sec > getConfigParameterTime(17, con_serw) || now_sec < getConfigParameterTime(18, con_serw))
                {
                    if (f_oswietlenieBasen != Mode::ON)
                    {
                        for (auto &o : OswietlenieBasen)
                            o.BlockAndSet(false);

                        f_oswietlenieBasen = Mode::ON;
                        SendData("Wylaczono oswietlenie basenu aktywne na ruch - tryb noc");
                    }
                }
                else
                {
                    if (f_oswietlenieBasen != Mode::OFF)
                    {
                        for (auto &o : OswietlenieBasen)
                            o.Unblock();

                        f_oswietlenieBasen = Mode::OFF;
                        f_zakonczono_miganie_swiatel_basen = Mode::OFF;
                        SendData("Zalaczona oswietlenie basenu aktywne na ruch - tryb dzien");
                    }
                }
            }
            else if (f_oswietlenieBasen == Mode::ON)
            {
                if(f_zakonczono_miganie_swiatel_basen != Mode::ON)
                {
                    for (auto &o : OswietlenieBasen)
                        o.SetAndIgnoreBlock(false);
                    OswietlenieBasenTrybMiganie.Reset();
                    OswietlenieBasenMiganiePrzerwa.Reset();
                    OswietlenieBasenMiganiePraca.Reset();
                    f_zakonczono_miganie_swiatel_basen = Mode::ON;
                    SendData("Zakonczono tryb miganie oswietlenie basen");
                }

                for (auto &o : OswietlenieBasen)
                    o.Unblock();

                f_oswietlenieBasen = Mode::OFF;
                f_zakonczono_miganie_swiatel_basen = Mode::OFF;
                SendData("Zalaczona oswietlenie basenu aktywne na ruch - tryb dzien");
            }

            // ############################################
            //  Tryb id 15 - miganie świateł po blokadzie basenu
            // ############################################
            if (getConfigParameterMode(15, con_serw))
            {
                if (f_oswietlenieBasen == Mode::ON && f_zakonczono_miganie_swiatel_basen != Mode::ON)
                {
                    OswietlenieBasenTrybMiganie.changeMode(Mode::ON);

                    if (!OswietlenieBasenTrybMiganie.isFinished())
                    {
                        if (!OswietlenieBasenTrybMiganie.IsRunning())
                        {
                            SendData("Zalaczono program migania basenu");
                            OswietlenieBasenTrybMiganie.Set();
                        }

                        if (!OswietlenieBasenMiganiePrzerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                        {
                            if (!OswietlenieBasenMiganiePraca.IsRunning())
                            {
                                OswietlenieBasenMiganiePraca.Set();
                                for (auto &o : OswietlenieBasen)
                                    o.SetAndIgnoreBlock(true);
                            }

                            if (OswietlenieBasenMiganiePraca.IsTimeout())
                            {
                                OswietlenieBasenMiganiePraca.Reset();
                                OswietlenieBasenMiganiePrzerwa.Set();
                                for (auto &o : OswietlenieBasen)
                                    o.SetAndIgnoreBlock(false);
                            }
                        }

                        if (!OswietlenieBasenMiganiePraca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                        {
                            if (!OswietlenieBasenMiganiePrzerwa.IsRunning())
                            {
                                OswietlenieBasenMiganiePrzerwa.Set();
                                for (auto &o : OswietlenieBasen)
                                    o.SetAndIgnoreBlock(false);
                            }

                            if (OswietlenieBasenMiganiePrzerwa.IsTimeout())
                            {
                                OswietlenieBasenMiganiePrzerwa.Reset();
                                OswietlenieBasenMiganiePraca.Set();
                                for (auto &o : OswietlenieBasen)
                                    o.SetAndIgnoreBlock(true);
                            }
                        }

                        if (OswietlenieBasenTrybMiganie.IsTimeout())
                        {
                            for (auto &o : OswietlenieBasen)
                                o.SetAndIgnoreBlock(false);
                            OswietlenieBasenMiganiePrzerwa.Reset();
                            OswietlenieBasenMiganiePraca.Reset();
                            OswietlenieBasenTrybMiganie.changeMode(Mode::OFF);
                            f_zakonczono_miganie_swiatel_basen = Mode::ON;
                            SendData("Zakonczono tryb miganie oswietlenie basen");
                        }
                    }
                }
            }
            else if (f_oswietlenieBasen == Mode::ON && f_zakonczono_miganie_swiatel_basen != Mode::ON)
            {
                for (auto &o : OswietlenieBasen)
                    o.SetAndIgnoreBlock(false);
                OswietlenieBasenTrybMiganie.Reset();
                OswietlenieBasenMiganiePrzerwa.Reset();
                OswietlenieBasenMiganiePraca.Reset();
                f_zakonczono_miganie_swiatel_basen = Mode::ON;
                SendData("Zakonczono tryb miganie oswietlenie basen");
            }


            // ############################################
            //  Tryb id 16 - czasowa wentylacja magazynku czystej pościeli
            // ############################################
            if (getConfigParameterMode(16, con_serw))
            {
                if (!CyklWentylatorMagazynekPoscieliPrzerwa.IsRunning()) // jeżeli cykl przerwy jest nieaktywny, praca
                {
                    if (!CyklWentylatorMagazynekPoscieliPraca.IsRunning())
                    {
                        SendData("Załączenie pracy wentylatora magazynku poscieli (ON)");
                        CyklWentylatorMagazynekPoscieliPraca.Set();
                        WentylatorMagazynekPoscieli.Set(true);
                    }

                    if (CyklWentylatorMagazynekPoscieliPraca.IsTimeout())
                    {
                        SendData("Załączenie przerwy wentylatora magazynku poscieli (OFF)");
                        CyklWentylatorMagazynekPoscieliPraca.Reset();
                        CyklWentylatorMagazynekPoscieliPrzerwa.Set();
                        WentylatorMagazynekPoscieli.Set(false);
                    }
                }

                if (!CyklWentylatorMagazynekPoscieliPraca.IsRunning()) // jeżeli cykl pracy jest nieaktywny, przerwa
                {
                    if (!CyklWentylatorMagazynekPoscieliPrzerwa.IsRunning())
                    {
                        SendData("Załączenie przerwy wentylatora magazynku poscieli (OFF)");
                        CyklWentylatorMagazynekPoscieliPrzerwa.Set();
                        WentylatorMagazynekPoscieli.Set(false);
                    }

                    if (CyklWentylatorMagazynekPoscieliPrzerwa.IsTimeout())
                    {
                        SendData("Załączenie pracy wentylatora magazynku poscieli (ON)");
                        CyklWentylatorMagazynekPoscieliPrzerwa.Reset();
                        CyklWentylatorMagazynekPoscieliPraca.Set();
                        WentylatorMagazynekPoscieli.Set(true);
                    }
                }
            }

            // ############################################
            //  Tryb id 19 - działanie oświetlenia lacznik na ruch
            // ############################################
            if (getConfigParameterMode(19, con_serw))
            {
                if (now_sec > getConfigParameterTime(27, con_serw) && now_sec < getConfigParameterTime(26, con_serw))
                {
                    if (f_oswietlenieLacznik != Mode::ON)
                    {
                        OswietlenieLacznik.BlockAndSet(false);
                        f_oswietlenieLacznik = Mode::ON;
                        SendData("Wylaczono oswietlenie lacznik aktywne na ruch");
                    }
                }
                else
                {
                    if (f_oswietlenieLacznik != Mode::OFF)
                    {
                        OswietlenieLacznik.Unblock();
                        f_oswietlenieLacznik = Mode::OFF;
                        SendData("Zalaczono oswietlenie lacznik aktywne na ruch");
                    }
                }
            }
            else if (f_oswietlenieLacznik == Mode::ON)
            {
                OswietlenieLacznik.Unblock();
                f_oswietlenieLacznik = Mode::OFF;
                SendData("Zalaczono oswietlenie lacznik aktywne na ruch");
            }

		  // ############################################
		  //  Tryb id 6 - działanie podlewania szklarnia
		  // ############################################
		  if (getConfigParameterMode(6, con_scho))
		  {
			if (now_sec > getConfigParameterTime(13, con_scho) && now_sec < getConfigParameterTime(14, con_scho))
			{
			  if (f_podlewanie_szklarnia != Mode::ON)
			  {
				PodlewanieSzklarnia.Set(true);
				f_podlewanie_szklarnia = Mode::ON;
				SendData("Zalaczono podlewanie szklarnia");
			  }
			}
			else
			{
			  if (f_podlewanie_szklarnia != Mode::OFF)
			  {
				PodlewanieSzklarnia.Set(false);
				f_podlewanie_szklarnia = Mode::OFF;
				SendData("Wylaczono podlewanie szklarnia");
			  }
			}
		  }
		  else if (f_podlewanie_szklarnia == Mode::ON)
		  {
			PodlewanieSzklarnia.Set(false);
			f_podlewanie_szklarnia = Mode::OFF;
			SendData("Wylaczono podlewanie szklarnia");
		  }

		  // ############################################
		  //  Tryb id 7 - działanie podlewania grzadka
		  // ############################################
		  if (getConfigParameterMode(7, con_scho))
		  {
			if (now_sec > getConfigParameterTime(13, con_scho) && now_sec < getConfigParameterTime(14, con_scho))
			{
			  if (f_podlewanie_grzadka != Mode::ON)
			  {
				PodlewanieGrzadka.Set(true);
				f_podlewanie_grzadka = Mode::ON;
				SendData("Zalaczono podlewanie grzadka");
			  }
			}
			else
			{
			  if (f_podlewanie_grzadka != Mode::OFF)
			  {
				PodlewanieGrzadka.Set(false);
				f_podlewanie_grzadka = Mode::OFF;
				SendData("Wylaczono podlewanie grzadka");
			  }
			}
		  }
		  else if (f_podlewanie_grzadka == Mode::ON)
		  {
			PodlewanieGrzadka.Set(false);
			f_podlewanie_grzadka = Mode::OFF;
			SendData("Wylaczono podlewanie grzadka");
		  }

		  // ############################################
		  //  Tryb id 8 - działanie podlewania grzadka dodatkowa
		  // ############################################
		  if (getConfigParameterMode(8, con_scho))
		  {
			if (now_sec > getConfigParameterTime(13, con_scho) && now_sec < getConfigParameterTime(14, con_scho))
			{
			  if (f_podlewanie_grzadka_dodatkowa != Mode::ON)
			  {
				PodlewanieGrzadkaDodatkowa.Set(true);
				f_podlewanie_grzadka_dodatkowa = Mode::ON;
				SendData("Zalaczono podlewanie grzadka dodatkowa");
			  }
			}
			else
			{
			  if (f_podlewanie_grzadka_dodatkowa != Mode::OFF)
			  {
				PodlewanieGrzadkaDodatkowa.Set(false);
				f_podlewanie_grzadka_dodatkowa = Mode::OFF;
				SendData("Wylaczono podlewanie grzadka dodatkowa");
			  }
			}
		  }
		  else if (f_podlewanie_grzadka_dodatkowa == Mode::ON)
		  {
			PodlewanieGrzadkaDodatkowa.Set(false);
			f_podlewanie_grzadka_dodatkowa = Mode::OFF;
			SendData("Wylaczono podlewanie grzadka dodatkowa");
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
