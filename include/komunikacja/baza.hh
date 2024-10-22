#pragma once

#include <iostream>
#include <mysql.h>
#include <string>
#include <exception>

class SqlException : public std::exception
{
public:
    /** Constructor (C strings).
     *  @param message C-style string error message.
     *                 The string contents are copied upon construction.
     *                 Hence, responsibility for deleting the char* lies
     *                 with the caller.
     */
    explicit SqlException(const char *message)
        : msg_(message) {}

    /** Constructor (C++ STL strings).
     *  @param message The error message.
     */
    explicit SqlException(const std::string &message)
        : msg_(message) {}

    /** Destructor.
     * Virtual to allow for subclassing.
     */
    virtual ~SqlException() noexcept {}

    /** Returns a pointer to the (constant) error description.
     *  @return A pointer to a const char*. The underlying memory
     *          is in posession of the Exception object. Callers must
     *          not attempt to free the memory.
     */
    virtual const char *what() const noexcept
    {
        return msg_.c_str();
    }

protected:
    /** Error message.
     */
    std::string msg_;
};

/**
 * @brief Funkcja inicjuje wskaźnik do bazy oraz ustanawia połączenie
 * 
 * @param con połączenie z bazą
 * @param dane adres, login, hasło, nazwa bazy w tablicy
 * @return true jeżeli poprawnie otwarto bazę
 * @return false jeżeli błąd
 */
bool InicjujBaza(MYSQL **con, const std::string * dane);

/**
 * @brief Funkcja inicjuje wskaźnik do bazy oraz ustanawia połączenie
 * 
 * @param con połączenie z bazą
 * @param adres adres bazy
 * @param login login do bazy
 * @param haslo hasło do bazy
 * @param baza nazwa bazy do otwarcia
 * @return true jeżeli poprawnie otwarto bazę
 * @return false jeżeli błąd
 */
bool InicjujBaza(MYSQL **con, const char* adres, const char* login, const char* haslo, const char* baza);

/**
 * @brief Kończy połączenie z bazą, jeżli jej wskaźnik jest prawidłowy
 * 
 * @param con połączenie z bazą
 */
void ZakonczBaza(MYSQL **con);

/**
 * @brief Kończy połączenie z bazą oraz wyświetla błąd, jeśli jej wskaźnik jest prawidłowy
 * 
 * @param con połączenie z bazą
 */
void ZakonczBazaZBledem(MYSQL **con);

/**
 * @brief Wysyła zapytanie SQL do bazy oraz zwraca wynik zapytania
 * 
 * @exception std::runtime_error błąd wysyłania zapytania lub pobierania danych
 * @param con połączenie z bazą
 * @param zapytanie zapytanie SQL
 * @return MYSQL_RES* 
 */
MYSQL_RES *PobierzBaza(MYSQL *con, const char *zapytanie);

/**
 * @brief Wysyła zapytanie SQL do bazy
 * 
 * @param con połączenie z bazą
 * @param zapytanie zapytanie SQL
 */
void AktualizujBaza(MYSQL *con, const char *zapytanie);