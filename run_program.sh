#!/bin/bash

echo "Noise Alert Program is now running!"

gnome-terminal -- mosquitto -v
gnome-terminal -- bash -c 'chmod +x ./bin/broker_recovery && ./bin/broker_recovery; exec $SHELL'
gnome-terminal -- bash -c 'chmod +x ./bin/admin_logs && ./bin/admin_logs; exec $SHELL'
gnome-terminal -- bash -c 'chmod +x ./bin/admin_alerts && ./bin/admin_alerts; exec $SHELL'
gnome-terminal -- bash -c 'chmod +x ./bin/nth_313_pub && ./bin/nth_313_pub; exec $SHELL'
gnome-terminal -- bash -c 'chmod +x ./bin/nth_313_sub && ./bin/nth_313_sub; exec $SHELL'