#/bin/sh

openocd -f interface/stlink.cfg \
          -f target/stm32f1x.cfg \
          -c "init; reset halt; flash write_image erase ./build/new_nixie.bin 0x08000000; reset halt; exit"
