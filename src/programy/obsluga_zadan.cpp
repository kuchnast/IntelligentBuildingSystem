#include "programy/obsluga_zadan.hh"

void CountdownTimer::Set(std::chrono::system_clock::time_point tp)
{
    m_run_time_point = std::make_unique<std::chrono::system_clock::time_point>(tp);
}

int CountdownTimer::getDuration() const
{
    return m_duration.count();
}

int CountdownTimer::getElapsedSec() const
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - *m_run_time_point).count();
}

bool CountdownTimer::IsTimeout() const
{
    if (m_run_time_point)
    {
        if (getElapsedSec() >= m_duration.count())
        {
            return true;
        }
    }
    return false;
}

void CountdownTimer::Reset()
{
    m_run_time_point.reset();
}

bool CountdownTimer::IsRunning() const
{
    return (m_run_time_point != nullptr ? true : false);
}

void CountdownTimer::Print(std::ostream &stream) const
{
    if (m_run_time_point != nullptr)
    {
        time_t tt = std::chrono::system_clock::to_time_t(*m_run_time_point);
        stream << "Timer set at " << ctime(&tt) << std::endl;
        stream << "Duration: " << m_duration.count() << std::endl;
        stream << "Time left: " << m_duration.count() - std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - *m_run_time_point).count() << std::endl;
    }
    else
    {
        stream << "Timer not set." << std::endl;
        stream << "Duration: " << m_duration.count() << std::endl;
    }
}


std::ostream &operator<<(std::ostream &stream, const CountdownTimer &timer)
{
    if (timer.m_run_time_point != nullptr)
    {
        time_t tt = std::chrono::system_clock::to_time_t(*timer.m_run_time_point);
        stream << "Timer set at " << ctime(&tt) << std::endl;
        stream << "Duration: " << timer.m_duration.count() << std::endl;
        stream << "Time left: " << timer.m_duration.count() - std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - *timer.m_run_time_point).count() << std::endl;
    }
    else
    {
        stream << "Timer not set." << std::endl;
        stream << "Duration: " << timer.m_duration.count() << std::endl;
    }

    return stream;
}

void DelayOnOffTimer::changeMode(Mode mode)
{
    if (m_current_mode == FIRST_RUN)
    {
        this->Reset();
        this->Set(std::chrono::system_clock::now() - std::chrono::seconds(this->getDuration()));
        m_current_mode = mode;
        m_is_finished = false;
    }
    else if (m_current_mode != mode)
    {
        this->Reset();
        m_current_mode = mode;
        m_is_finished = false;
    }
}

bool DelayOnOffTimer::isFinished() const
{
    return m_is_finished;
}

void DelayOnOffTimer::taskFinished()
{
    m_is_finished = true;
}

std::ostream &operator<<(std::ostream &stream, const DelayOnOffTimer &timer)
{
    timer.Print(stream);
    stream << "Curent mode: " << (timer.m_current_mode == Mode::ON ? "ON" : "OFF") << std::endl;
    stream << "Is finished flag: " << (timer.m_is_finished == true ? "true" : "false") << std::endl;

    return stream;
}

void WatchdogTimer::Feed()
{
    CountdownTimer::Set();
}

bool WatchdogTimer::IsTimeout()
{
    return CountdownTimer::IsTimeout();
}

template <class T>
bool Watchdog<T>::Feed(T value)
{
    if (!m_prev_value)
    {
        m_prev_value = std::make_unique<T>(value);
        WatchdogTimer::Feed();
        return true;
    }
    else
    {
        bool test = m_Test_condition(value, *m_prev_value);
        *m_prev_value = value;
        if (test)
        {
            WatchdogTimer::Feed();
            return true;
        }
    }

    return false;
}

template <class T>
bool Watchdog<T>::IsTimeout()
{
    return WatchdogTimer::IsTimeout();
}

template <class T>
void Watchdog<T>::ResetCounting()
{
    WatchdogTimer::Feed();
}

bool OutputPinTask::Get()
{
    MYSQL_RES *wynik_zapytania; // struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; // struktura na wiersz tabeli
    int wartosc;
    std::string query = "SELECT wartosc FROM Piny_Wyjscia_On_Off WHERE idPiny_Wyjscia_On_Off = ";

    query.append(std::to_string(m_id));
    wynik_zapytania = PobierzBaza(m_con, query.c_str());

    if ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) // jeżeli zwrócono wynik
    {
        wartosc = std::stoi(wiersz_zapytania[0]);
    }
    mysql_free_result(wynik_zapytania); /// zwolnij wynik zapytania

    return wartosc;
}

bool OutputPinTask::Set(bool state)
{
    if (state == m_is_set)
        return true;

    MYSQL_RES *wynik_zapytania; // struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; // struktura na wiersz tabeli
    int recznie;

    std::string query = "SELECT recznie FROM Piny_Wyjscia_On_Off WHERE idPiny_Wyjscia_On_Off = ";
    query.append(std::to_string(m_id));

    wynik_zapytania = PobierzBaza(m_con, query.c_str());

    if ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) // jeżeli zwrócono wynik
    {
        recznie = std::stoi(wiersz_zapytania[0]);
    }
    mysql_free_result(wynik_zapytania); /// zwolnij wynik zapytania

    if (recznie) // cant change value
        return false;

    if (state == false)
        query = "UPDATE Piny_Wyjscia_On_Off SET wartosc = CASE WHEN wartosc > 0 THEN wartosc - 1 ELSE 0 END WHERE idPiny_Wyjscia_On_Off = ";
    else
        query = "UPDATE Piny_Wyjscia_On_Off SET wartosc = wartosc + 1 WHERE idPiny_Wyjscia_On_Off = ";

    query.append(std::to_string(m_id));
    AktualizujBaza(m_con, query.c_str());

    m_is_set = state;
    return true;
}

bool OutputPinTask::isSet()
{
    return m_is_set;
}

void OutputPinTaskWithBlock::SetAndIgnoreBlock(bool state)
{
    if (state == m_is_set)
        return;

    std::string query;

    if (state)
        query = "UPDATE Piny_Wyjscia_On_Off SET wartosc = 1 WHERE idPiny_Wyjscia_On_Off =  ";
    else
        query = "UPDATE Piny_Wyjscia_On_Off SET wartosc = 0 WHERE idPiny_Wyjscia_On_Off =  ";

    query.append(std::to_string(m_id));
    AktualizujBaza(m_con, query.c_str());

    m_is_set = state;
}

void OutputPinTaskWithBlock::Block()
{
    if (is_blocked == true)
        return;

    std::string query = "UPDATE Piny_Wyjscia_On_Off SET recznie = 1 WHERE idPiny_Wyjscia_On_Off =  ";
    query.append(std::to_string(m_id));

    AktualizujBaza(m_con, query.c_str());

    is_blocked = true;
}

void OutputPinTaskWithBlock::BlockAndSet(bool state)
{
    if (is_blocked == true)
        return;

    std::string query;

    if (state)
        query = "UPDATE Piny_Wyjscia_On_Off SET wartosc = 1, recznie = 1 WHERE idPiny_Wyjscia_On_Off =  ";
    else
        query = "UPDATE Piny_Wyjscia_On_Off SET wartosc = 0, recznie = 1 WHERE idPiny_Wyjscia_On_Off =  ";

    query.append(std::to_string(m_id));
    AktualizujBaza(m_con, query.c_str());

    is_blocked = true;
    m_is_set = state;
}

void OutputPinTaskWithBlock::Unblock()
{
    if (is_blocked == false)
        return;

    std::string query = "UPDATE Piny_Wyjscia_On_Off SET recznie = 0 WHERE idPiny_Wyjscia_On_Off =  ";
    query.append(std::to_string(m_id));

    AktualizujBaza(m_con, query.c_str());

    is_blocked = false;
}

void OutputPinTaskWithBlock::UnblockAndSet(bool state)
{
    Unblock();
    Set(state);
}

bool OutputPinTaskWithBlock::CheckIfBlocked()
{
    return is_blocked;
}


bool InputPinTask::Get()
{
    MYSQL_RES *wynik_zapytania; // struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; // struktura na wiersz tabeli
    int wartosc;
    std::string query = "SELECT czy_teraz_aktywny FROM Piny_Wejscia_On_Off WHERE idPiny_Wejscia_On_Off = ";

    query.append(std::to_string(m_id));
    wynik_zapytania = PobierzBaza(m_con, query.c_str());

    if ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) // jeżeli zwrócono wynik
    {
        wartosc = std::stoi(wiersz_zapytania[0]);
    }
    mysql_free_result(wynik_zapytania); /// zwolnij wynik zapytania

    return wartosc;
}

float TemperatureTask::Get()
{
    MYSQL_RES *wynik_zapytania; // struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; // struktura na wiersz tabeli
    float temp;
    std::string query = "SELECT temperatura FROM Czujniki_Temperatury WHERE idCzujniki_temperatury = ";

    query.append(std::to_string(m_id));
    wynik_zapytania = PobierzBaza(m_con, query.c_str());

    if ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) // jeżeli zwrócono wynik
    {
        temp = std::stof(wiersz_zapytania[0]);
    }
    mysql_free_result(wynik_zapytania); /// zwolnij wynik zapytania

    return temp;
}


int getConfigParameterTime(int id, MYSQL *con)
{
    MYSQL_RES *wynik_zapytania; // struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; // struktura na wiersz tabeli
    int value;
    std::string query = "SELECT wartosc FROM Konfiguracja_Czas WHERE id = ";

    query.append(std::to_string(id));
    wynik_zapytania = PobierzBaza(con, query.c_str());

    if ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) // jeżeli zwrócono wynik
    {
        value = std::stoi(wiersz_zapytania[0]);
    }
    mysql_free_result(wynik_zapytania); /// zwolnij wynik zapytania

    return value;
}

float getConfigParameterTemp(int id, MYSQL *con)
{
    MYSQL_RES *wynik_zapytania; // struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; // struktura na wiersz tabeli
    float value;
    std::string query = "SELECT wartosc FROM Konfiguracja_Temp WHERE id = ";

    query.append(std::to_string(id));
    wynik_zapytania = PobierzBaza(con, query.c_str());

    if ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) // jeżeli zwrócono wynik
    {
        value = std::stof(wiersz_zapytania[0]);
    }
    mysql_free_result(wynik_zapytania); /// zwolnij wynik zapytania

    return value;
}

bool getConfigParameterMode(int id, MYSQL *con)
{
    MYSQL_RES *wynik_zapytania; // struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; // struktura na wiersz tabeli
    int value;
    std::string query = "SELECT wartosc FROM Konfiguracja_Tryb WHERE id = ";

    query.append(std::to_string(id));
    wynik_zapytania = PobierzBaza(con, query.c_str());

    if ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) // jeżeli zwrócono wynik
    {
        value = std::stoi(wiersz_zapytania[0]);
    }
    mysql_free_result(wynik_zapytania); /// zwolnij wynik zapytania

    return (value != 0 ? true : false);
}

int nowToSec()
{
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    auto bt = *std::localtime(&tt);

    return (bt.tm_hour * 60 + bt.tm_min) * 60 + bt.tm_sec;
}

void setWorkingParameter(int id, MYSQL *con, int value)
{
    std::string query;

    query = "UPDATE Parametry_Robocze SET value = ";
    query.append(std::to_string(value));
    query += " WHERE id = ";
    query.append(std::to_string(id));

    AktualizujBaza(con, query.c_str());
}

int getWorkingParameter(int id, MYSQL *con)
{
    MYSQL_RES *wynik_zapytania; // struktura na liczbe wierszy w tabeli
    MYSQL_ROW wiersz_zapytania; // struktura na wiersz tabeli
    std::string query;
    int value = 0;

    query = "SELECT value FROM Parametry_Robocze WHERE id = ";
    query.append(std::to_string(id));

    wynik_zapytania = PobierzBaza(con, query.c_str());
    if ((wiersz_zapytania = mysql_fetch_row(wynik_zapytania))) // jeżeli zwrócono wynik
    {
        value = std::stoi(wiersz_zapytania[0]);
    }
    mysql_free_result(wynik_zapytania); /// zwolnij wynik zapytania

    return value;
}

int incraseWorkingParameter(int id, MYSQL *con, int value)
{
    int val = value + getWorkingParameter(id, con);

    std::string debug = "Incrase from " + std::to_string(val - value) + " to " + std::to_string(val) + " for filtr " + std::to_string(id);
    SendData(debug.c_str());

    setWorkingParameter(id, con, val);

    return val;
}
