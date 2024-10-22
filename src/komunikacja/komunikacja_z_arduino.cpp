#include <iostream>
#include "komunikacja/komunikacja_z_arduino.hh"
#include "konfiguracja.hh"

bool SerialArduino::Open()
{
    m_serial_port.SetTimeout(500);

    try
    {
        m_serial_port.Open();
    }
    catch(const Exception& e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }

    return true;
}

void SerialArduino::Close()
{
    m_serial_port.Close();
}


bool SerialArduino::Write(const std::string &buf)
{
    try
    {
        m_serial_port.Write(buf);
    }
    catch (std::exception &e)
    {
        std::cerr << e.what();
        return false;
    }
    catch (...)
    {
        std::exception_ptr p = std::current_exception();
        std::cerr << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
        return false;
    }

    return true;
}

bool SerialArduino::Read(std::string &buf)
{
    std::string temp;
    buf.clear();
    int counter = TIMEOUT_COUNT;

    try
    {
        do
        {
            m_serial_port.Read(temp);
            buf.append(temp);
        }
        while((temp.back() != '\n') && (--counter > 0));

    }
    catch (std::exception &e)
    {
        std::cerr << e.what();
        return false;
    }
    catch (...)
    {
        std::exception_ptr p = std::current_exception();
        std::cerr << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
        return false;
    }

    return true;
}

void convertToASCII(const std::string &letter)
{
    for (const auto & c : letter)
    {
        if(c =='\n')
            std::cout << "\\n";
        else
            std::cout << c;

        std::cout << '[' <<  int(c) << "] ";
    }
    std::cout << std::endl;
}