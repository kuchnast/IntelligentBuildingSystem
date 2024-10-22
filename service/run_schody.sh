#!/bin/bash 
xterm -e /home/pi/InteligentnyBudynek/obsluga_urzadzen_i2c.app /home/pi/InteligentnyBudynek/dane/schody.txt &
sleep 30
xterm -e /home/pi/InteligentnyBudynek/obsluga_zdarzen_ruchu.app /home/pi/InteligentnyBudynek/dane/schody.txt /home/pi/InteligentnyBudynek/dane/serwerownia.txt &
sleep 2
xterm -e /home/pi/InteligentnyBudynek/obsluga_zdarzenia_czasowe.app /home/pi/InteligentnyBudynek/dane/schody.txt /home/pi/InteligentnyBudynek/dane/serwerownia.txt &
