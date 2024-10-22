#pragma once

#include <iostream>
#include <vector>
#include <mysql.h>
#include <time.h>

enum class TRYB_PRACY
{
    BEZ_CZASU_DATY,
    AKT_GDY_CZAS_I_RUCH,
    AKT_GDY_CZAS
};

class ZdarzenieRuchu
{
private:
    int m_prog;              ///< prog jasnosci(0-10)
    int m_czas_dzialania;    ///< czas dzialania w sekundach
    time_t m_czas_aktywacji; ///< czas aktywacji zdarzenia
    struct tm m_akt_od;      ///< dolny prog aktywacji zdarzenia
    struct tm m_akt_do;      ///< gorny prog aktywacji zdarzenia
    bool m_czy_dziala;       ///< flaga czy zdarzenie jest obecnie aktywne
    bool m_aktywny;          ///< flaga stanu zdarzenia (1) - aktywny (0) - nieaktywny
    TRYB_PRACY m_tryb_pracy; ///< (0) - nie sprawdzaj czasu i daty (1) - aktywuj gdy ruch i czas w zadanym przedziale (2) - aktywuj gdy czas w zadanym przedziale

public:

/**
 * @brief Konstruktor nowego obiektu Zdarzenie Ruchu
 * 
 */
    ZdarzenieRuchu();

/**
 * @brief Inicjuje zdarzenie ruchu, ustawia i sprawdza poprawność parametrów
 * 
 * @param prog próg jasności
 * @param czas_dzialania czas działąnia w sekundach
 * @param akt_od godziny aktywacji od
 * @param akt_do godziny aktywacji do
 * @return true jeżeli poprawnie zainicjowano
 * @return false jeżeli błąd wartości
 */
    bool Inicjuj(int prog, int czas_dzialania, struct tm &akt_od, struct tm &akt_do, TRYB_PRACY tryb_pracy);

/**
 * @brief Blokada zdarzenia ruchu
 * 
 * @param komunikat komunikat do wyświetlenia
 */
    void Blokuj(const char * komunikat);

/**
 * @brief Zwraca wartość flagi czy działa
 * 
 * @return true zdarzenie aktywne
 * @return false zdarzenie nieaktywne
 */
    bool OdczytCzyDziala() const;

/**
 * @brief Zwraca obecny tryb pracy
 * 
 * @return TRYB_PRACY tryb pracy zdarzenia
 */
    TRYB_PRACY OdczytTrybPracy() const;

    /**
 * @brief Ustawia flagę czy działą
 * 
 * @param czy_dziala stan flagi
 */
    void UstawCzyDziala(bool czy_dziala);

/**
 * @brief Ustawia flagę czy działą w programie oraz w bazie
 * 
 * @param con połączenie z bazą
 * @param czy_dziala stan flagi
 */
    void UstawCzyDzialaIWBazie(MYSQL *con, bool czy_dziala, int id_zdarzenia);

/**
 * @brief Ustawia czas aktywacji zdarzenia
 * 
 * @param teraz obecny czas
 */
    void UstawCzasAktywacji(time_t &teraz);

/**
 * @brief Sprawdza czy czas minął
 * 
 * @param teraz obecny czas
 * @return true jeżeli czas zdarzenia się skończył
 * @return false jeżeli zdarzenie dalej powinno być aktywne
 */
    bool CzyCzasMinal(time_t &teraz) const;

/**
 * @brief Sprawdza czy obecny próg jasności jest niższy od progu zdarzenia lub równy 0
 * 
 * @param prog obecny próg jasności
 * @return true jeżeli zdarzenie powinno być aktywne
 * @return false jeżeli nie
 */
    bool CzyWPrzedzialeProgu(int prog) const;

/**
 * @brief Sprawdza czy przedział godzinowy czasu się zgadza z czasem działąnia zdarzenia
 * 
 * @param teraz struktura czasu 
 * @return true jeżeli zdarzenie powinno być aktywne
 * @return false jeżeli nie
 */
    bool CzyWPrzedzialeCzasu(struct tm *teraz) const;

/**
 * @brief Wypisuje pole obiektu TRYB_PRACY
 * 
 * @param strm strumień wejściowy
 * @param T objekt TRYB_PRACY
 * @return std::ostream& strumien z wejścia
 */
    friend std::ostream &operator<<(std::ostream &strm, const TRYB_PRACY &T);

/**
 * @brief Wypisuje pola obiektu zdarzenia ruchu
 * 
 * @param strm strumień wejściowy
 * @param Z objekt zdarzenia ruchu
 * @return std::ostream& strumien z wejścia
 */
    friend std::ostream &operator<<(std::ostream &strm, const ZdarzenieRuchu &Z);
/**
 * @brief Wypisuje tablice obiektów zdarzeń ruchu
 * 
 * @param strm strumień wejściowy
 * @param Z tablica obiektów zdarzeń ruchu
 * @return std::ostream& strumien z wejścia
 */
    friend std::ostream &operator<<(std::ostream &strm, const std::vector<ZdarzenieRuchu> &Z);
};

/**
 * @brief Pobranie z bazy informacji o zdarzeniach ruchu, wyzerowanie flag czy_dziala
 * 
 * @param con połączenie z bazą
 * @param tabela nazwa wczytywanej tabeli
 * @return std::vector<ZdarzenieRuchu> 
 */
std::vector<ZdarzenieRuchu> StartZdarzeniaRuchu(MYSQL *con);

/**
 * @brief Odczytuje wszystkie aktywne zdarzenia w bazie odnoszące się do tego czujnika ruchu
 *        Zostają one załączone lub aktualizowane w programie i odpowiedniej bazie.
 * @param con tablica połączeń do baz
 * @param index indeks bazy tego urządzenia
 * @param idPinRuchu id pinu ruchu do aktywacji
 * @param prog_jasnosci obecna wartość progu jasności
 * @param z_ruch tablica zdarzeń ruchu
 */
void AktywujZdarzeniaRuchu(MYSQL *con[], int index, int idPinRuchu, int prog_jasnosci, std::vector<ZdarzenieRuchu> &z_ruch);

/**
 * @brief Odczytuje wszystkie aktywne zdarzenia czasu w bazie (id pinu ruchu = -1)
 *        Zostają one załączone lub aktualizowane w programie i odpowiedniej bazie.
 * @param con tablica połączeń do baz
 * @param index indeks bazy tego urządzenia
 * @param prog_jasnosci obecna wartość progu jasności
 * @param z_ruch tablica zdarzeń ruchu
 */
void AktywujZdarzeniaCzasu(MYSQL *con[], int index, int prog_jasnosci, std::vector<ZdarzenieRuchu> &z_ruch);

/**
 * @brief Zmienia w bazie i programie flagę czy_dziala zdarzenia.
 *        Dodatkowo sprawdza we wszystkich bazach czy jest inne aktywne zdarzenie dotyczące tego pinu przekaźnika.
 *        Jeżeli nie, wyłącza go.
 * 
 * @param con tablica połączeń z baż
 * @param index indeks bazy tego urządzenia
 * @param idZdarzenia id obecnego zdarzenia ruchu
 * @param z_ruch obiekt obecnego zdarzenia ruchu
 */
void DezaktywujZdarzenieRuchu(MYSQL *con[], int index, int idZdarzenia, ZdarzenieRuchu &z_ruch);

/**
 * @brief Aktualizuje zdarzenia ruchu na podstawie pinów czy_teraz_aktywny w tablicy czujników, sprawdza czy zadrzenia powinny być włączone,
 *        wyłączone czy dalej aktywne. Jeżeli jako któreś połączenie z bazą będzie równe NULL, nie będzie ono brane pod uwage przy zmianie pinów przekaźników. 
 * 
 * @param con tablica wskaźników połączenia z bazą (0 serwerownia) (1 schody) (2 kotłownia)
 * @param z_ruch tablica zdarzeń ruchu
 * @param nr_bazy_serwerowni indeks łącza do tabeli z czujnika jasności
 */
void AktualizujZdarzeniaRuchu(MYSQL *con[],  std::vector<ZdarzenieRuchu> &z_ruch, int nr_bazy_serwerowni);

/**
 * @brief Pobieraz z bazy wartość progu jasności
 * 
 * @param con wskaznik na baze z czujnikiem jasnosci
 * @return int wartość jasności
 */
int PobierzJasnoscZBazy(MYSQL *con);

/**
 * @brief Zmienia wartość pinu przekaznika w bazie. Polega to na zwiększeniu o 1 jeżeli wartosc to true
 *        lub zmniejszeniu o 1 jeżeli wartość to false i nie jest już równa 0
 * 
 * @param con wskaznik na baze
 * @param idPin_Przekaznik id pinu przekaznika
 * @param wartosc true - zwiększenie o 1, false - zmniejszenie o 1, ale nie mniej niż 0
 */
void ZmienPinPrzekaznikaWBazie(MYSQL *con, int idPin_Przekaznik, bool wartosc);

/**
 * @brief Sprawdzenie, czy udało ustawić się pin w bazie
 *
 * @param con wskaznik na baze
 * @param idPin_Przekaznik id pinu przekaznika
 * @return true pin został poprawnie ustawiony
 * @return false pin nie został ustawiony lub jest zablokowany
 */
bool SprawdzPinPrzekaznikaWBazie(MYSQL *con, int idPin_Przekaznik);