# LNbits TpoS Printer

This project features an ESP32 controlled thermal printer solution to print payment receipts as payments are made to the LNbits TPoS extension.

The project is fully inspired by the [Coinos printer](coinos/coinos-printer) project.

## Setup

Connect the printer to the ESP32 as follows.

You will probably need a seperate power supply for the printer as it needs ~ 1amp which an ESP32 struggles to provide.

1. Printer GND to ESP32 GND and power supply GND
1. Printer VCC to power supply 5v - 9v
1. Printer RX to ESP32 GPIO 21
1. Printer TX to ESP32 GPIO 2

You *may* be able to use the ESP32 5v pin to power the printer and I have had some success with this. However print quality is slightly degraded and speed is low. If you are going to do this, use a 5V power supply with at least 2 amps available. Wiring as follows:

1. Printer GND to ESP32 GND
1. Printer VCC to ESP32 5v
1. Printer RX to ESP32 GPIO 21
1. Printer TX to ESP32 GPIO 2

## Firmware

1. Copy config.h.example to config.h
1. Edit config.h to match your setup
1. Compile and upload to ESP32
1. Power on