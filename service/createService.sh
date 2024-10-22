#!/bin/bash
    chmod 644 InteligentnyBudynek.service
    ln -svf /home/pi/InteligentnyBudynek/service/InteligentnyBudynek.service /etc/systemd/system/InteligentnyBudynek.service
    systemctl daemon-reload
    systemctl enable InteligentnyBudynek.service
    systemctl start InteligentnyBudynek.service
