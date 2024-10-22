#ifndef CZUJNIKI_TEMPERATURY_HH
#define CZUJNIKI_TEMPERATURY_HH

#define PROG_BLEDU_DS 10

#include <iostream>
#include <vector>
#include <string>

#include "komunikacja/komunikacja_z_arduino.hh"



class CzujnikTemperatury{
private:
    int m_id;               ///< id czujnika temperatury
    int m_pin;              ///< pin arduino z czujnikiem
    const char *m_adres;    ///< adres czujnika temperatury
    float m_temp;           ///< temperatura czujnika
    int m_liczba_bledow;    ///< ilość błędów odczytu pod rząd
    bool m_czy_blad;        ///< flaga błędu - (0) brak błędu (1) błąd i zaniechanie dalszych odczytów

public:
/**
 * @brief Usunięty konstruktor domyślny nowego obiektu Czujnik Temperatury
 * 
 */
    CzujnikTemperatury() = delete;
    

/**
 * @brief Konstruktor nowego obiektu Czujnik Temperatury
 * 
 * @param id identyfikator czujnika
 * @param pin pin arduino z czujnikiem
 * @param adres adres czujnika hex[2*8]
 */
    CzujnikTemperatury(int id, int pin, const char * adres) : m_id(id), m_pin(pin), m_adres(adres), m_temp(0), m_liczba_bledow(0), m_czy_blad(false) {}

/**
 * @brief Inicjalizacja czujnika na arduino
 * 
 * @param sa łącze do arduino
 * 
 * @return true poprawnie zainicjalizowano
 * @return false wystąpił błąd
 */
    bool InicjalizujCzujnik(SerialArduino & sa);

/**
 * @brief Zwraca identyfikator czujnika
 * 
 * @return int identifikator czujnika
 */
   int ZwrocId() const;

/**
 * @brief Wysyła zapytanie do arduino o czujnik i zapisuje jego temperature
 * 
 * @return true poprawnie pobrano wartość
 * @return false wystąpił błąd, zwiększenie licznika błędów
 */
   bool AktualizujCzujnik(SerialArduino & sa);

/**
 * @brief Zwraca stan flagi błędu
 * 
 * @return true błąd czujnika, brak dalszych odczytów
 * @return false brak błędu
 */
   bool CzyBlad() const;

/**
 * @brief Zwiększa licznik błędów. Gdy osiągnie 10, ustawia stały błąd
 * 
 * @return true 10 błąd czujnika, brak dalszych odczytów
 * @return false zwiększono
 */
   bool ZwiekszLicznikBledow();

/**
 * @brief Zwraca ostatnio odczytaną temperature
 * 
 * @return float przechowywana temperatura
 */
   float ZwrocTemperature() const;

/**
 * @brief Ustawia flagę błędu blokując czujnik i ustawiając błąd w bazie
 * 
 */
   void DezaktywujCzujnikWBazie(MYSQL *con);
};

/**
 * @brief Pobiera z bazy dane czujników temperatury i inicializuje je
 * @details Jeżeli czujnik nie będzie prawidłowo zainicjalizowany lub podpięty, zwracamy odrazu błąd do bazy.
 * 
 * @param con łącze do bazy
 * @param sa łącze do arduino
 * @return std::vector<CzujnikTemperatury> dynamiczna tablica z zainicjalizowanymi czujnikami temperatury
 */
std::vector<CzujnikTemperatury> StartCzujnikiTemperatury(MYSQL *con, SerialArduino & sa);

/**
 * @brief Odczytuje i aktualizuje temperature danego czujnika w bazie
 * 
 * @param con łącze do bazy
 * @param sa łącze do arduino
 * @param C czujnik temperatury
 * @return true operacja zakończona powodzeniem
 * @return false błąd (aktywna flaga czy_blad lub błąd odczytu)
 */
bool AktualizujCzujnikTemperatury(MYSQL *con, SerialArduino & sa, CzujnikTemperatury &C);

#endif