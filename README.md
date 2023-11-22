# LNbits TpoS Printer

This project features an ESP32 controlled thermal printer solution to print payment receipts as payments are made to the LNbits TPoS extension.

## Setup

Connect thermal printer as follows:

You will probably need a seperate power supply for the printer as it needs ~ 1amp which an ESP32 struggles to provide.

1. Printer GND to ESP32 GND and power supply GND
1. Printer VCC to power supply 5v - 9v
1. Printer RX to ESP32 GPIO 15
1. Printer TX to ESP32 GPIO 2

## Firmware

1. Copy config.h.example to config.h
1. Edit config.h to match your setup
1. Compile and upload to ESP32
1. Power on