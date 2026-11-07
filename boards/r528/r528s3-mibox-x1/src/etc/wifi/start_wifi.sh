#!/bin/sh
set +e

echo "[wifi] start_wifi"

wapi disconnect wlan0
ifup wlan0
wapi mode wlan0 2
sleep 12

wapi reconnect wlan0
sleep 10

renew wlan0
sleep 2
renew wlan0
sleep 2
renew wlan0

wapi show wlan0
echo "[wifi] wifi startup done"
