#/bin/sh

openocd -f interface/stlink.cfg \
          -f target/stm32f1x.cfg \
          -c "init; reset halt; stm32f1x unlock 0; reset halt; exit"
