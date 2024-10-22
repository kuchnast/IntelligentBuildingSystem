#!/bin/bash
xterm -e /home/pi/InteligentnyBudynek/obsluga_urzadzen_i2c.app /home/pi/InteligentnyBudynek/dane/kotlownia.txt &
sleep 30
xterm -e /home/pi/InteligentnyBudynek/obsluga_arduino.app /home/pi/InteligentnyBudynek/dane/kotlownia.txt &
sleep 2
xterm -e /home/pi/InteligentnyBudynek/obsluga_kotlownia.app /home/pi/InteligentnyBudynek/dane/kotlownia.txt /home/pi/InteligentnyBudynek/dane/serwerownia.txt &