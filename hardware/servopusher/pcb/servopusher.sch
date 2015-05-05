EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "servopusher"
Date "Di 05 Mai 2015"
Rev "r0"
Comp "Patrick Kanzler, FAU FabLab"
Comment1 ""
Comment2 "Triggered by pushing a button."
Comment3 "position and back into to idle position."
Comment4 "Circuit that drives a servo motor into a preprogrammed"
$EndDescr
Text Notes 850  800  0    60   ~ 0
power supply
$Comp
L CONN_01X02 P?
U 1 1 554873DE
P 950 1300
F 0 "P?" H 950 1450 50  0000 C CNN
F 1 "CONN_01X02" V 1050 1300 50  0000 C CNN
F 2 "" H 950 1300 60  0000 C CNN
F 3 "" H 950 1300 60  0000 C CNN
	1    950  1300
	-1   0    0    -1  
$EndComp
$EndSCHEMATC
