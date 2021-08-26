# WHEM - Whole Home Energy Meter (Retired for ESP32 Version)
Whole Home Energy Meter using Arduino Mega paired with Raspberry Pi. Mega essentially acts as an ADC reading 2 pairs of current trasnducers and AC mains Voltage, one on each leg for US power grid. These values are stacked and calculated to come up with Current and Voltage levels that are passed via Serial to Raspberry PI Zero W.

Raspberry PI end runs Python script to capture serial data and pass to database. Webserver on Raspberry PI hosts basic template for displaying data on charts updating in real time.

Project has been retired for new build using more powerful ESP32 in place of Mega with cloud database and display page hosting.
