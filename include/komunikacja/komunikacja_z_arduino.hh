#pragma once
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include "komunikacja/SerialPort.hpp"

#define TIMEOUT_COUNT 50 // * 100ms = 5s

using namespace mn::CppLinuxSerial;

// Arduino jes na porcie /dev/ttyACM0
class SerialArduino
{
private:
    SerialPort m_serial_port;

public:
    SerialArduino() : m_serial_port("/dev/ttyACM0", BaudRate::B_115200) {}

    SerialArduino(SerialPort serial) : m_serial_port(serial) {}

    SerialArduino(const std::string &device, BaudRate baudRate) : m_serial_port(device, baudRate) {}

    bool Open();

    void Close();

    bool Write(const std::string &buf);

    bool Read(std::string &buf);
};

void convertToASCII(const std::string &letter);
