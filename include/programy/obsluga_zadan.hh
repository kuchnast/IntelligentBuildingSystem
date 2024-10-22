#pragma once

#include <chrono>
#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <functional>

#include "komunikacja/baza.hh"
#include "konfiguracja.hh"

enum Mode
{
    OFF,
    ON,
    FIRST_RUN
};

class CountdownTimer
{
private:
    std::unique_ptr<std::chrono::system_clock::time_point> m_run_time_point;
    std::chrono::duration<int> m_duration;

public:
    CountdownTimer(int duration) : m_run_time_point(nullptr), m_duration(duration) {}

    void Set(std::chrono::system_clock::time_point tp = std::chrono::system_clock::now());

    int getDuration() const;

    int getElapsedSec() const;

    bool IsTimeout() const;

    void Reset();

    bool IsRunning() const;

    void Print(std::ostream &stream = std::cout) const;

    friend std::ostream &operator<<(std::ostream &stream, const CountdownTimer &timer);
};

class DelayOnOffTimer : public CountdownTimer
{
private:
    Mode m_current_mode;
    bool m_is_finished;


public:
    DelayOnOffTimer(int duration) : CountdownTimer(duration), m_current_mode(Mode::FIRST_RUN), m_is_finished(false) {}

    DelayOnOffTimer(int duration, Mode current_mode) : CountdownTimer(duration), m_current_mode(current_mode), m_is_finished(false) {}

    void changeMode(Mode mode);

    bool isFinished() const;

    void taskFinished();

    friend std::ostream &operator<<(std::ostream &stream, const DelayOnOffTimer &timer);
};

class WatchdogTimer : private CountdownTimer
{

public:
    WatchdogTimer(int max_delay) : CountdownTimer(max_delay) {}

    void Feed();

    bool IsTimeout();
};

template <class T>
class Watchdog : private WatchdogTimer
{
private:
    std::unique_ptr<T> m_prev_value;
    std::function<bool(T, T)> m_Test_condition;

public:
    Watchdog(int max_delay, std::function<bool(T, T)> TestCondition) : WatchdogTimer(max_delay), m_Test_condition(TestCondition) {}

    bool Feed(T value);

    bool IsTimeout();

    void ResetCounting();
};

class OutputPinTask
{
protected:
    int m_id;
    MYSQL *m_con;
    bool m_is_set;

public:
    OutputPinTask(int id, MYSQL *con) : m_id(id), m_con(con), m_is_set(false) {}

    bool Get();

    bool Set(bool state);

    bool isSet();
};

class OutputPinTaskWithBlock : public OutputPinTask
{
protected:
    bool is_blocked;

public:
    OutputPinTaskWithBlock(int id, MYSQL *con) : OutputPinTask(id, con), is_blocked(false) {}

    void SetAndIgnoreBlock(bool state);

    void Block();

    void BlockAndSet(bool state);

    void Unblock();

    void UnblockAndSet(bool state);

    bool CheckIfBlocked();
};

class InputPinTask
{
private:
    int m_id;
    MYSQL *m_con;

public:
    InputPinTask(int id, MYSQL *con) : m_id(id), m_con(con) {}

    bool Get();
};

class TemperatureTask
{
private:
    int m_id;
    MYSQL *m_con;

public:
    TemperatureTask(int id, MYSQL *con) : m_id(id), m_con(con) {}

    float Get();
};

int getConfigParameterTime(int id, MYSQL *con);

float getConfigParameterTemp(int id, MYSQL *con);

bool getConfigParameterMode(int id, MYSQL *con);

int nowToSec();

void setWorkingParameter(int id, MYSQL *con, int value);

int getWorkingParameter(int id, MYSQL *con);

int incraseWorkingParameter(int id, MYSQL *con, int value);
