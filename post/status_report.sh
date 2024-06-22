#!/bin/bash
# Report status of every tool

mosquitto_sub -t "tools/dust_collection/status" -W 5 &
sleep 1
mosquitto_pub -t "tools/dust_collection/status" -m "REPORT STATUS"


