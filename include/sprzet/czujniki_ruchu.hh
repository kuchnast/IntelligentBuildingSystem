#pragma once

#include <iostream>
#include <vector>
#include <bitset>
#include <mysql.h>

#include "sprzet/tca.hh"

#define IODIRA 0x00 //kierunek transmisji 0 - output 1 - input
#define IODIRB 0x01
#define GPIOA 0x12 //wartosci na pinach GPIO
#define GPIOB 0x13

using std::bitset;

/**
 * @brief Klasa emuluje pojęcie grupy czujników ruchu obsługiwanej układem MCP23017
 * 
 */
class CzujnikiRuchu{
private:
    int m_adres;                  ///< adres grupy czujnikow na magistrali I2C (32-39)
    int m_linia;                  ///< linia I2C na ktorej znajduje sie grupa
    int m_uchwyt;                 ///< uchwyt do urzadzenia
    bitset<16> m_aktywne_piny;    ///< piny na ktorych sledzony jest ruch (1) - aktywny (0) - nieaktywny
    bitset<16> m_polaryzacja;     ///< polaryzacja danego pinu (0) -{HIGH - ruch LOW - brak ruchu} (1) - odwrotnie
    bitset<16> m_czy_dziala;      ///< flaga czy jest ruch na danym pinie (0) - ruch (1) - brak ruchu
    bool m_aktywny;               ///< flaga stanu grupy (1) - aktywna (0) - nieaktywna

public:
    CzujnikiRuchu() = delete;

/**
 * @brief Konstruktor nowego obiektu Czujniki Ruchu
 * 
 * @param adres adres grupy
 * @param linia linia I2C
 */
    CzujnikiRuchu(int adres, int linia);

/**
 * @brief Blokuje grupe czujników i wyświetla komunikat
 * 
 * @param komunikat komunikat o powodzie blokady
 */
    void Blokuj(std::string komunikat);

/**
 * @brief Inicjuje grupe czujników, pobiera uchwyt i ustawia kierunek komunikacji
 * 
 * @param tca ekspander I2C
 * @return true jeżeli poprawnie zainicjowano
 * @return false jeżeli błąd
 */
    bool Inicjuj(Tca &tca);

/**
 * @brief Zwraca stan flagi aktywności pinu
 * 
 * @param nr_pinu numer pinu
 * @return true aktywny 
 * @return false nieaktywny
 */
    bool OdczytAktywnyPin(int nr_pinu) const;

/**
 * @brief Ustawia stan flagi aktywności pinu
 * 
 * @param nr_pinu numer pinu
 * @param wartosc stan pinu
 */
    void UstawAktywnyPin(int nr_pinu, bool wartosc);

/**
 * @brief Zwraca stan flagi polaryzacji pinu
 * 
 * @param nr_pinu numer pinu
 * @return true odwrocona
 * @return false normalna
 */
    bool OdczytPolaryzacjaPin(int nr_pinu) const;

/**
 * @brief Ustawia stan flagi polaryzacji pinu
 * 
 * @param nr_pinu numer pinu
 * @param wartosc stan pinu
 */
    void UstawPolaryzacjaPin(int nr_pinu, bool wartosc);

/**
 * @brief Zwraca stan flagi czy_dziala pinu
 * 
 * @param nr_pinu numer pinu
 * @return true jeżeli czujnik wykrywa ruch
 * @return false jeżeli brak ruchu
 */
    bool OdczytCzyDziala(int nr_pinu) const;

/**
 * @brief Zwraca całą tablice flag czy_dziala
 * 
 * @return const bitset<16>& stany flagi czy_działą grupy czujników
 */
    const bitset<16> &OdczytCzyDziala() const;

/**
 * @brief Ustawia wszystkie piny czy_dziala nową tablicą
 * 
 * @param czy_dziala tablica wartości kolejnych pinów
 */
    void UstawCzyDziala(bitset<16> &czy_dziala);

    /**
 * @brief Odczytuje aktualny stan piów czujnika, porównuje go z flagami i zwraca piny na których jest ruch w zmiennej czy_dziala
 * 
 * @param tca ekspander I2C 
 * @return bitset<16> tablice pinów na których stan zmienił się względem poprzedniego
 */
    bitset<16> SprawdzRuch(Tca &tca) const;

/**
 * @brief Wypisuje wszystkie pola czujników ruchu na wybrane wyjście
 * 
 * @param strm strumień wyjściowy
 * @param C czujniki ruchu do wyświetlenia
 * @return std::ostream& strumień z wejścia
 */
    friend std::ostream &operator<<(std::ostream &strm, const CzujnikiRuchu &C);
};

/**
 * @brief Wypisuje całą zawartość tablicy czujników ruchu na wybrane wyjście
 * 
 * @param strm strumień wyjściowy
 * @param C tablica czujników ruchu do wyświetlenia
 * @return std::ostream& strumień z wejścia
 */
std::ostream &operator<<(std::ostream &strm, const std::vector<CzujnikiRuchu> &C);

/**
 * @brief Początkowa inicjalizacja czujników ruchu i pobranie informacji z bazy
 * @details Czujnik jest inicjalizowan z bazy, odczytywane są flagi, czyszczone flagi czy_aktywny
 *
 * @param con połączenie z bazą
 * @param tca multiplekser I2C
 * @return std::vector<CzujnikiRuchu> dynamiczną tablice z zainicjowanymi czujnikami ruchu z bazy
 */
std::vector<CzujnikiRuchu> StartCzujnikiRuchu(MYSQL *con, Tca &tca);

/**
 * @brief Aktualizuje flagi pinów między bazą a programem
 * @details Pobierane są wszystkie piny z bazy, jeżeli w pamięci ma on inną wartość, jest aktualizowany 
 *
 * @param con połączenie z bazą
 * @param tca multiplekser I2C
 * @param ruch czujniki ruchu 
 */
void AktualizujFlagiPinyCzujnikiRuchu(MYSQL *con, Tca &tca, std::vector<CzujnikiRuchu> &ruch);

/**
 * @brief Sprawdza ruch na pinach oraz ustawia flagę czy_teraz_aktywny w bazie danych 
 * 
 * @param con połączenie z bazą
 * @param tca multiplekser I2C
 * @param ruch czujniki ruchu
 */
void AktualizujRuch(MYSQL *con, Tca &tca, std::vector<CzujnikiRuchu> &ruch);
