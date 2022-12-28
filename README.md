# IN12-STM32-Nixie-Clock

STM32 Nixie clock project.
<br/>
<p align="center">
    <img width="600" src="https://github.com/Kononenko-K/IN12-STM32-Nixie-Clock/blob/main/pics/photo1.jpg">
</p>

## Overview

This is a Nixie clock project that built with four soviet IN-12 indicators. It works with 5V power supply from a Type-C connecor and has an internal high voltage generator.

## [Hardware](Hardware)

<br/>
<p align="center">
    <img width="750" src="https://github.com/Kononenko-K/IN12-STM32-Nixie-Clock/blob/main/pics/boards.jpg">
</p>

The clock consist of a several different PCB modules. There is a [display module](/Hardware/nixie%20display) ([schematic](/Hardware/nixie%20display/project.pdf)) that uses single K155ID1 (SN74141N soviet eqivalent) as a high voltage BCD-to-Decimal decoder that drives four indicators through a dynamic indication by switching Nixies' anodes with four TLP627 optocouples. A [controller module](/Hardware/mcu%20board) ([schematic](/Hardware/mcu%20board/project.pdf)) is made with STM32F103C8Tx that has an internal RTC peripheral and a battery input which keeps the RTC running when the main power supply is off. The module also contains a boost HV converter that uses a PWM signal from one of the MCU timers outputs. It has no feedback loop so it may be required to trim duty cycle in a source code depending on a type of components you're going to use. All the other PCBs are basically used as the parts of enclosure but some of them also contain a few components (e.g. buttons, RTC battery holder).

Modules connection guide:

|display module      |mcu module|
|--------------------|----------|
|A1                  |PA4       |
|A2                  |PA3       |
|A3                  |PB0       |
|A4                  |PB2       |
|--------------------|----------|                    
|D2                  |PA5       |
|D1                  |PA7       |
|D3                  |PB1       |
|D0                  |PB10      |
|--------------------|----------|                    
|NE                  |PA0       |
|--------------------|----------|                    
|+180V               |HV        |
|--------------------|----------|
|+5V                 |+5V       |

All the respective GNDs should be connected together.


## [Firmware](Firmware)

The project is made in Keil v5 with HAL so it should be easy to migrate to any other compatible STM32 lqfp-48 controller if needed. As mentioned before it may be required to adjust Boost converter duty cycle, so it can be made by editing a PWM macro in [main.c](/Firmware/Src/main.c) in range from 0 to 159. High duty cycle values may damage the device. There are also a few macros that adjust the deley between switching the indicators, so they also may need to be trimmed to make tubes switch fast enough but without phantom glowing of other tubes nearby. There's also a feature in the code that prevents cathode poisoning by quickly switching over all the tubes every 5 minutes.

