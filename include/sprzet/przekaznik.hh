#pragma once

#include <iostream>
#include <bitset>
#include <vector>
#include <mysql.h>

#include "sprzet/tca.hh"

#define IODIRA 0x00 //kierunek transmisji 0 - output 1 - input
#define IODIRB 0x01
#define GPIOA 0x12  //wartosci na pinach GPIO
#define GPIOB 0x13

using std::bitset;

/**
 * @brief Klasa modeluje pojęcie 16-wyjściowego przekaźnika opartego o układ MCP23018
 * 
 */
class Przekaznik
{
private:

    int m_adres;              ///< adres przekaznika na magistrali I2C (32-39)
    int m_linia;              ///< linia I2C na ktorej znajduje sie przekaznik
    int  m_uchwyt;            ///< uchwyt do urzadzenia
    bitset<16> m_wartosc;     ///< wartosci rejestrow wyjsciowych bit = 1 (ON) bit = 0(OFF)
    bitset<16> m_recznie;     ///< wartosci flagi recznej obslugi pinów bit = 1 (recznie) bit = 0(automatycznie)
    bool m_aktywny;           ///< flaga stanu przekaznika (1) - aktywny (0) - nieaktywny

public:

/**
 * @brief Konstruktor nowego obiektu Przekaznik
 * 
 */
    Przekaznik();

/**
 * @brief Konstruktor nowego obiektu Przekaznik
 * 
 * @param adres adres na lini I2C
 * @param linia numer lini I2C
 */
    Przekaznik(int adres, int linia);

/**
 * @brief Blokuje przekaźnik przed próbami zapisu do niego i wyświetla komunikat o błędzie
 * 
 * @param komunikat Komunikat o przyczynie blokady
 */
    void Blokuj(const char *komunikat);

/**
 * @brief Pobiera uchwyt do urządzenia, ustawia rejestry na wyjściowe, 
 *        ustawia flagę na aktywny oraz aktualizuje wszystkie piny przekaznika.
 * @details W przypadku błędu blokuje przekaźnik. 
 *
 * @param tca ekspander I2C do zmiany lini
 * @return true jeżeli poprawnie zainicjowano
 * @return false jeżeli błąd
 */
    bool Inicjuj(Tca &tca);

/**
 * @brief Pobiera uchwyt do urządzenia, ustawia rejestry na wyjściowe, 
 *        ustawia flagę na aktywny oraz aktualizuje wszystkie piny przekaznika.
 * @details W przypadku błędu blokuje przekaźnik. 
 *
 * @param tca ekspander I2C do zmiany lini
 * @param adres adres na lini I2C
 * @param linia numer lini I2C
 * @return true jeżeli poprawnie zainicjowano
 * @return false jeżeli błąd
 */
    bool Inicjuj(Tca &, int adres, int linia);

    /**
 * @brief Odczytuje flagę danego pinu
 * 
 * @param nr_pinu numer pinu
 * @return true pin ma flagę true
 * @return false pin ma flagę false
 */
    bool OdczytReczne(int nr_pinu) const;

/**
 * @brief Zmienia flage danego pinu
 * 
 * @param nr_pinu numer pinu
 * @param wartosc stan pinu
 */
    void UstawReczne(int nr_pinu, bool wartosc);

/**
 * @brief Odczytuje wartość danego pinu
 * 
 * @param nr_pinu numer pinu
 * @return true jeżeli pin ma stan true
 * @return false jeżeli pin ma stan false
 */
    bool OdczytPin(int nr_pinu) const;

/**
 * @brief Zmienia stan danego pinu, jeżeli nie ma on ustawionej flagi reczny na true
 * 
 * @param nr_pinu numer pinu
 * @param wartosc stan pinu
 */
    void UstawPin(int nr_pinu, bool wartosc);

/**
 * @brief Wysyła aktualizacje pinów do urządzenia, jeżeli jest aktywne
 * 
 * @param tca ekspander I2C
 */
    void AktualizujPiny(Tca &tca);

/**
 * @brief Zmienia wartosc pinu oraz jego flagi i wysyła aktualizacje do urządzenia jeżeli faktycznie coś się zmieni
 * 
 * @param tca ekspander I2C
 * @param nr_pinu numer pinu
 * @param wartosc_pin stan pinu
 * @param wartosc_recznie stan flagi
 */
    void AktualizujPinOrazReczne(Tca &tca, int nr_pinu, bool wartosc_pin, bool wartosc_recznie);

/**
 * @brief Wypisuje wszystkie pola przekaźnika na wybrane wyjście
 * 
 * @param strm strumień wyjściowy
 * @param P przekaźnik do wyświetlenia
 * @return std::ostream& strumień z wejścia
 */
    friend std::ostream &operator<<(std::ostream &strm, const Przekaznik &P);
};

/**
 * @brief Wypisuje wszystkie przekazniki z tablicy na wybrane wyjście
 * 
 * @param strm strumień wyjściowy
 * @param P tablica przekaźników do wyświetlenia
 * @return std::ostream& strumień z wejścia
 */
std::ostream &operator<<(std::ostream &strm, const std::vector<Przekaznik> &P);

/**
 * @brief Początkowa inicjalizacja przekaźników i pobranie informacji z bazy
 * @details Przekaźnik jest inicjalizowany z bazy, odczytywane są piny ręczne, a automatyczne czyszczone na off w bazie i programi 
 *
 * @exception std::runtime_error błąd indeksów w bazie
 * @param con połączenie z bazą
 * @param tca multiplekser I2C
 * @return std::vector<Przekaznik> dynamiczną tablice z zainicjowanymi przekaznikami z bazy
 */
 std::vector<Przekaznik> StartPrzekazniki(MYSQL *con , Tca &tca);

/**
 * @brief Aktualizuje ręczne piny między bazą a programem
 * @details Pobierane są wszystkie piny z bazy, jeżeli w pamięci ma on inną wartość, jest aktualizowany 
 *
 * @param con połączenie z bazą
 * @param tca multiplekser I2C
 * @param przekazniki tablica przekaznikow
 */
 void AktualizujPrzekazniki(MYSQL *con, Tca &tca, std::vector<Przekaznik> &przekazniki);

 /**
 * @brief Wyłącza wszystkie przekazniki, czyści w bazie działające, usuwa zawartość tablicy
 * 
 * @param con połączenie z bazą
 * @param tca multiplekser I2C
 * @param przekazniki tablica przekaznikow
 */
 void ZakonczPrzekazniki(MYSQL *con, Tca &tca, std::vector<Przekaznik> &przekazniki);