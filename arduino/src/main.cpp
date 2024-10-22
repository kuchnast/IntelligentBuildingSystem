#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "DS.hpp"
#include "LinkedList.hpp"
#include "PoziomWody.hpp"

#define BUF_SIZE 256

bool initDS();
bool readDS();
bool initPW();
bool readPW();
bool stringToDSAddress(const String &s, DeviceAddress *address);
void (*resetFunc)(void) = 0;

int freeRam();

LinkedList<oneWirePin> lista_oneWire;
LinkedList<DS> lista_ds;
LinkedList<PoziomWody> lista_poziomWody;

String inputString = "";
bool stringComplete = false;

void setup()
{
  Serial.begin(115200, SERIAL_8N1);
  inputString.reserve(BUF_SIZE);
}

void loop()
{

  if (stringComplete)
  {
    if (inputString.indexOf("initDS") >= 0)
    {
      if (initDS())
      {
        Serial.write("ok");
      }
      else
      {
        Serial.write("error");
      }
    }
    else if (inputString.indexOf("initPW") >= 0)
    {
      if (initPW())
      {
        Serial.write("ok");
      }
      else
      {
        Serial.write("error");
      }
    }
    else if (inputString.indexOf("readDS") >= 0)
    {
      if (readDS())
      {
        Serial.write("ok");
      }
      else
      {
        Serial.write("error");
      }
    }
    else if (inputString.indexOf("readPW") >= 0)
    {
      if (readPW())
      {
        Serial.write("ok");
      }
      else
      {
        Serial.write("error");
      }
    }
    else if (inputString.indexOf("reset") >= 0)
    {
      resetFunc();
    }
    else
    {
      Serial.write("unknownComand");
    }

    Serial.write("GET:");
    Serial.write(inputString.c_str());
    inputString = "";
    stringComplete = false;
  }
}

void serialEvent()
{
  while (Serial.available())
  {
    char inChar = (char)Serial.read();
    if (inChar == '\n')
    {
      stringComplete = true;
    }
    inputString += inChar;
  }
}

// return true if correct, false if error
bool initDS()
{
  int i, j;

  i = inputString.indexOf("id");
  if (i == -1)
  {
    return false;
  }

  i += 2;
  j = i;
  while (isDigit(inputString[j]))
  {
    ++j;
  }
  int id = inputString.substring(i, j).toInt();

  i = inputString.indexOf("pin");
  if (i == -1)
  {
    return false;
  }

  i += 3;
  j = i;
  while (isDigit(inputString[j]))
  {
    ++j;
  }
  int pin = inputString.substring(i, j).toInt();

  i = inputString.indexOf("address");
  if (i == -1)
  {
    return false;
  }

  i += 7;
  j = i + 16;

  DeviceAddress adres;
  if (stringToDSAddress(inputString.substring(i, j), &adres) == false)
  {
    return false;
  }

  oneWirePin *t = new oneWirePin(pin);
  Node<oneWirePin> *ow = lista_oneWire.search(t);
  DS *ds;

  if (ow == nullptr)
  {
    t->Init();
    lista_oneWire.addFront(t);
    ds = new DS(id, lista_oneWire.getFront(), adres);
  }
  else
  {
    ds = new DS(id, ow->getElement(), adres);
    delete t;
  }

  lista_ds.addFront(ds);

  return true;
}

bool stringToDSAddress(const String &s, DeviceAddress *address)
{
  if (s.length() != 16)
  {
    return false;
  }

  for (int i = 0; i < 16; ++i)
  {
    int hex;
    if (s[i] >= '0' && s[i] <= '9')
      hex = s[i] - '0';
    else if (s[i] >= 'A' && s[i] <= 'F')
      hex = s[i] - 'A' + 10;
    else if (s[i] >= 'a' && s[i] <= 'f')
      hex = s[i] - 'a' + 10;
    else
    {
      Serial.write("char ");
      return false;
    }

    if (i % 2 == 0)
    {
      (*address)[i / 2] = 16 * hex;
    }
    else
    {
      (*address)[i / 2] += hex;
    }
  }

  return true;
}

bool readDS()
{
  int i, j;

  i = inputString.indexOf("id");
  if (i == -1)
    return false;

  i += 2;
  j = i;
  while (isDigit(inputString[j]))
  {
    ++j;
  }
  int id = inputString.substring(i, j).toInt();
  DS *ds = new DS(id);
  auto node = lista_ds.search(ds);
  delete ds;

  if (node == nullptr)
    return false;

  ds = node->getElement();

  if (!ds->checkDSOnline())
    return false;

  ds->requestTempThisDevice();
  auto s = String(ds->readDSTemp(), 2);
  Serial.print(s.c_str());

  return true;
}

bool initPW()
{
  int i, j;

  i = inputString.indexOf("pin");
  if (i == -1)
  {
    return false;
  }

  i += 3;
  j = i;
  while (isDigit(inputString[j]))
  {
    ++j;
  }
  int pin = inputString.substring(i, j).toInt();

  PoziomWody *t = new PoziomWody(pin);
  lista_poziomWody.addFront(t);

  return true;
}

bool readPW()
{
  int i, j;

  i = inputString.indexOf("pin");
  if (i == -1)
    return false;

  i += 3;
  j = i;
  while (isDigit(inputString[j]))
  {
    ++j;
  }
  int pin = inputString.substring(i, j).toInt();
  PoziomWody *pw = new PoziomWody(pin);
  auto node = lista_poziomWody.search(pw);
  delete pw;

  if (node == nullptr)
    return false;

  pw = node->getElement();
  Serial.print(pw->Read());

  return true;
}

int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}