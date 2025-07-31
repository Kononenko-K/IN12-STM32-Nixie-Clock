# IN12-STM32-Nixie-Clock

STM32 Nixie clock project with four Soviet IN-12 indicators.

<br/>
<p align="center">
    <img width="600" src="https://github.com/Kononenko-K/IN12-STM32-Nixie-Clock/blob/main/pics/header.jpg">
</p>

## Overview

<br/>
<p align="center">
    <img src="https://github.com/Kononenko-K/IN12-STM32-Nixie-Clock/blob/main/pics/mainboard.jpg">
</p>

This project provides a complete solution for building an STM32-powered 
Nixie clock with Soviet IN-12 tubes. It operates on a 5V power supply 
via a Type-C connector and features an internal high voltage generator.

## Hardware

<br/>
<p align="center">
    <img src="https://github.com/Kononenko-K/IN12-STM32-Nixie-Clock/blob/main/pics/panoramic.jpg">
</p>

### [**Display Module**](/Hardware/nixie%20display/project.pdf)
- Utilizes the K155ID1 (equivalent to SN74141N) as a high-voltage 
BCD-to-decimal decoder.
- Drives four Nixie tubes using dynamic scanning with TLP627 optocouplers.

### [**Controller Module**](/Hardware/mcu%20board/project.pdf)
- Based on the STM32F103C8Tx microcontroller, featuring:
  - Internal RTC peripheral for timekeeping;
  - Battery input for maintaining time during power off;
  - Boost converter for high-voltage generation using a PWM signal from 
one of the MCU's timers;
  - Four WS2812B addressable RGB LEDs for tube illumination.

### **Connection Guide**

| **Display Module** | **Controller Module** |
|--------------------|-----------------------|
| A1                 | PA4                   |
| A2                 | PA6                   |
| A3                 | PB0                   |
| A4                 | PB2                   |
|--------------------|-----------------------|
| D0                 | PA5                   |
| D1                 | PB1                   |
| D2                 | PB10                  |
| D3                 | PA7                   |
|--------------------|-----------------------|
| NE                 | PA10                  |
|--------------------|-----------------------|
| +180V              | HV                    |
|--------------------|-----------------------|
| +5V                | +5V                   |

Connect all respective GNDs together.

## Firmware

The firmware is developed as a Makefile project, generated with 
STM32CubeMX. It can be built using:

```sh
make clean && make
```

To upload the firmware, use ST-LINK/V2 and OpenOCD with the following 
command:

```sh
openocd -f interface/stlink.cfg \
          -f target/stm32f1x.cfg \
          -c "init; reset halt; flash write_image erase ./build/new_nixie.bin 0x08000000; reset halt; exit"
```

The code implements firmware protection and it can be removed with:
```sh
openocd -f interface/stlink.cfg \
          -f target/stm32f1x.cfg \
          -c "init; reset halt; stm32f1x unlock 0; reset halt; exit"
```

For convenience, you can use the provided scripts:
- [flash.sh](/Firmware/flash.sh)
- [remove_protection.sh](/Firmware/remove_protection.sh)

The boost converter's duty cycle may require adjustment based on component 
type. Additionally, delay settings can be fine-tuned for optimal tube 
switching. The firmware includes a feature to prevent cathode poisoning by 
cycling through all tubes every 5 minutes. Also, there are 15 different modes 
of illumination using addressable LEDs.

## License

- The **Firmware** in this project is licensed under the 
[MIT License](/Firmware/LICENSE), with the exception of portions of the 
firmware code that are copyrighted by STMicroelectronics; such portions are 
subject to the original licensing terms provided by STMicroelectronics.
- The **Hardware** in this project is licensed under the 
[CERN Open Hardware Licence Permissive (CERN OHL-P)](/Hardware/LICENSE).




