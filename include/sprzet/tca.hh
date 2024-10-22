#pragma once

#define REJESTR_TCA 0xAA
#define TCA_ADRES 0x70

class Tca
{
private:
    int m_adres;
    int m_uchwyt;
    int m_linia;

public:
/**
 * @brief Konstruktor nowego obiektu Tca
 * 
 */
    Tca();
/**
 * @brief Ustawia adres, pobiera uchwyt i ustawia początkowe wartości dla lini 0
 * 
 * @param adres adres I2C
 * @return true jeżeli prawidłowo zainicjowano
 * @return false jeżeli błąd
 */
    bool Inicjuj(int adres = TCA_ADRES);

/**
 * @brief Obecnie ustawiona linia I2C
 * 
 * @return int numer obecnie ustawionej lini
 */
    int ObecnaLinia();

/**
 * @brief Zmienia numer lini I2C
 * 
 * @exception std::out_of_range błędna wartość lini
 * @param linia linia do zmiany
 */
    void ZmienLinie(int linia);

/**
 * @brief Wyświetla parametry struktury
 * 
 * @param strm strumień wyjściowy
 * @param tca ekspaneder I2C
 * @return std::ostream& strumień z wejścia
 */
    friend std::ostream &operator<<(std::ostream &strm, const Tca &tca);
};