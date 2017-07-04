# FreeRTOS Ported to Raspberry Pi 2B

This build runs a "temperature server" on the Raspberry PI 2B, as described in Notes below.

Evolution of this build:

1. fork from Jared Hull's RPi 2B FreeRTOS version 7.2 code.  https://github.com/Forty-Tw0/RaspberryPi-FreeRTOS
2. TCP/IP server made to work and then upgraded to FreeRTOS 9 (FreeRTOS and FreeRTOS+TCP).
3. "bit-banging" 1-Wire driver written using a GPIO pin as the "one" wire. (Demo/onewire.c)
4. driver layer written for the DS18B20 Onewire Temperature sensor. (Demo/ds18b20.c)

Jared Hull's code is a fork from James Walmsley's RPi 1 FreeRTOS build, modified for framebuffer support and the RPi2B.
https://github.com/jameswalmsley/RaspberryPi-FreeRTOS

The USB/Ethernet portion is a port of USPi, a LAN9514 USB driver.
https://github.com/rsta2/uspi

TCP/IP portion is the official FreeRTOS driver with modifications for compatability.
http://www.freertos.org/FreeRTOS-Labs/RTOS\_labs\_download.html


# NOTES:
    This build creates two tasks, the DS18B20 task and the TemperatureServer task.  These tasks communicate the temperature readings
    using a Queue.  Descriptions of these tasks:
  
    1. DS18B20 task.  Reads the ROM device ID. prints it and then goes into it's loop:
                      The loop reads the temperature, converts it to string and pushes it onto the queue. The queue can hold 4 readings.

    2. TemperatureServerTask:  Starts a server on a specified port and waits for a client. Once a client connects, client can send any 
                      arbitrary string and the TemperatureServer replies with the latest temperature reading.
    

    Currently, the onewire and ds18b20 code is very simple.  It has the following limitations.
    1. There is only support for a single device on the wire. This means the 1-wire search function is not implemented yet.
       Skip ROM commands are used to read the device id and to convert and read the temperature.
    2. An external pull-up is assumed, the code does not enable any internal pull-ups. Use a 4.7K resistor between Vdd and DQ.
    3. The device is assumed to be externally powered, i.e., do not connect with parasitic power. 
    4. The DS18B20 driver uses GPIO 4 by default.  This seems to be quite conventional for the temp sensors.  The code was written
       so that any GPIO could be used.

# Steps/Parts used to run:
    Parts:  Cobbler for the RPi2B, mini breadboard, 4.7K pull-up resistor, DS18B20 1-Wire temperature sensor
    1. Insert one end of the cobbler into the Pi and the other to a mini breadboard.
    2. Hook the sensor wires as follows: Vdd pin to a 3.3 V pin on the cobbler.  Hook the sensor DQ pin to a GPIO pin on the
       cobbler (4 by default). Connect the DS18B20 GND pin to a GND pin on the cobbler.  
       On the mini breadboard, connect a 4.7K-ohm resistor from DQ to Vdd.
    3. Build the code onto a micro-DS and run.
    4. Using some network client machine, connect to port 2056 and send some arbitrary string to the Pi. The Pi responds with 11 byte
       temperature reading.

       Format of temp reading is TTTT.tttt\r\n   where TTTT is the integer part of the temperature in C, tttt is the fractional part.
       Datasheet for the DS18B20  https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf

       Thanks to http://sg.danny.cz/foxg20/w1_bbtest.c for a simple example, which, in turn, is based on the Arduino OneWire Library.

# Testing:
    1. Setup a Linux laptop as a client machine. Setup the ethernet port as a static 192.168.1.1.
    2. Set the RPi2B as static 192.168.1.9
    3. Use a network client on the linux machine to send an arbitrary string to the Pi, Pi responds with temp reading as described above.
    4. This was run without failure for 24 hours.
    5. At random times the sensor was subjected to colder or hotter temperatures.  The output was observed to change accordingly.
     
# TODO:
    1. Make the 1-wire driver more fully featured, e.g. support more than one device on the wire, etc. 
    2. Support more that just the DS18B20.
    3. Make a more fully featured weather station server, e.g. add air pressure, humidity, altimeter, etc.
    4. Support 9, 10, 11 bit resolution for faster processing, currently only uses default 12-bit. 
       Resolution works as follows: there are four bits of fractional temperature, 2^-1, 2^-2, 2^-3, 2^-4
       For 9-bit resolution, the lowest three bits are left off, giving 0.5 degree C resolution, measurement time of 93.75 ms.
       For 10-bit resolution, the lowest two bits are left off, giving .25 degree C resolution, measurement time of 187.5 ms.
       For 11-bit resolution, the lowest bit is left off, giving .125 degree C resolution, measurement time of 375 ms.
       For 12-bit resolution, no bits are left off, giving .0625 degree C resolution, measurement time of 750 ms.
       
       You can see how this works, each fractional bit included doubles the processing time, but also doubles the resolution.
       For this build, a dedicated temperature server, 750 ms is acceptable for the added resolution. 
         

## Howto Build

Type make

You need to modify the arm-non-eabi- toolchain locations in the Makefile:

    kernel.elf: LDFLAGS += -L"/usr/lib/gcc/arm-none-eabi/4.9.3" -lgcc
    kernel.elf: LDFLAGS += -L"/usr/lib/arm-none-eabi/lib" -lc

On Ubuntu you can install the toolchain with: sudo apt-get install gcc-arm-none-eabi

Format your SD card as FAT32

Copy the bootcode.bin and start.elf from here https://github.com/raspberrypi/firmware/tree/master/boot

Copy the config.txt from /boot\_stuff onto the SD card to fix over/underscanning

Copy the kernel7.img generated by make

You should see the green ACT LED continuously blink as proof the task schedualer is working.

##Configuration
GCC -finstrument-functions enables tracing at the beginning and end of every function. You can add __attribute__((no_instrument_function)) to functions to disable tracing in critical sections.

The framebuffer is explicitly for debug information, this build does not take full advantage of the RPi GPU. Printing to the screen takes several milliseconds so it is adviseable to disable it. In the demo main.c, set loaded = 0.
