#!/bin/bash
xterm -e /home/avena/InteligentnyBudynek/obsluga_urzadzen_i2c.app /home/avena/InteligentnyBudynek/dane/serwerownia.txt &
sleep 30
xterm -e /home/avena/InteligentnyBudynek/obsluga_arduino.app /home/avena/InteligentnyBudynek/dane/serwerownia.txt &
sleep 2
xterm -e /home/avena/InteligentnyBudynek/obsluga_zdarzen_ruchu.app /home/avena/InteligentnyBudynek/dane/serwerownia.txt /home/avena/InteligentnyBudynek/dane/schody.txt &
sleep 2
xterm -e /home/avena/InteligentnyBudynek/obsluga_basen.app /home/avena/InteligentnyBudynek/dane/serwerownia.txt /home/avena/InteligentnyBudynek/dane/kotlownia.txt &
sleep 2
xterm -e /home/avena/InteligentnyBudynek/obsluga_solary.app /home/avena/InteligentnyBudynek/dane/serwerownia.txt /home/avena/InteligentnyBudynek/dane/kotlownia.txt &
