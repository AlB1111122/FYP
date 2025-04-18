# Bare metal vs Linux video processor FYP

FYP comparing the performance of the same video decoding and filtering application on bare metal and Linux on Raspberry Pi 4B [(results repo)](https://github.com/AlB1111122/FYPJupiterNotebooks). Part research, mostly bare metal development.

The filter can be changed by simply going into [kernel.cc](src/platform/baremetal/kernel.cc) for bare metal or [display_sdl.cc](src/platform/Linux/display_sdl.cc) for Linux, changing the [filter](include/common/filter.h) being used in the `updateFrame` function, and recompiling.

**This project contains a number of very large files (especially `soccerBytes.cc`, but also the `pl_mpeg` files), and may cause an IDE to crash if you have a lot of C/C++ extensions installed or have those files open when compiling. You may want to disable extensions like IntelliSense or comment out the `#include "../../../soccerBytes.h"` in [kernel.cc](src/platform/baremetal/kernel.cc) until you are compiling the project.**

The video decoder [pl_mpeg](https://github.com/phoboslab/pl_mpeg) only decodes .mpg video. To play any video with the Linux version you can convert it like this:

```sh
ffmpeg -i input.mp4 -c:v mpeg1video -q:v 0 -c:a mp2 -format mpeg output.mpg
```

To compile the bare metal version you need to turn the `.mpg` video file into a c byte array.
There is a video file and the byte array version already in this project for convenience.
_This guide assumes the user is using a Linux development machine, tested on Debian only._

# Linux (Debian for Raspberry Pi 4B) version

## Compilation requirements:

- build-essential
- cmake
- libsdl2-dev
- pkg-config
- minicom (to test UART communication)
- make

## To run:

Either use direnv to set the shell variables `direnv allow` or just set the variables in .envrc manually.

```sh
make scratch-build-l
make run
```

Performance metrics will be written to results.csv.

# Bare metal (Raspberry Pi 4B) version

## Requirements

### Software

- arm-gnu cross compiler for Raspberry Pi 4B (aarch64-none-elf) [(available here)](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
- minicom to receive UART (or a preferred serial communication program)
- make
- rpi-imager [(info)](https://www.raspberrypi.com/news/raspberry-pi-imager-imaging-utility/)

### Hardware

- Raspberry Pi 4B
- microSD card
- microSD card reader
- USB to UART adapter (for performance logs)
- micro HDMI to HDMI cable

## To run

1. Use the rpi-imager to write 64 bit Debian to the sd card.
2. Remove any kernel and .img files from the boot section of the newly written sd card.
3. Compile the bare metal version of the project; go to the root of the project and run `make`.
4. Copy [the project's new kernel image](build/bm/img/kernel8.img) to the SD cards boot directory. (this may require you to make the boot direcory bigger).
5. Overwrite the config.txt in the boot directory of the SD card with this project's [config.txt](src/platform/baremetal/config/config.txt).
6. Unmount the SD card from the dev machine and insert it into the Raspberry Pi 4B.
7. Plug a powered on monitor into the Raspberry Pi 4B via HDMI.
8. Connect the UART jumpers to the Raspberry Pi 4Bs GPIO pins and the USB to the dev machine.
9. Connect to the USB to UART adapter on the dev machine with minicom with a baudrate of 115200

```sh
minicom -b 115200 -o -D /dev/ttyUSB0
```

_may not be ttyUSB0 as it is dependent on the adapter._

To save the UART output:
Before the logs start, press Ctrl+A, then L, and enter a filename. This file will be created in your home directory.

10. Plug the power supply into the Raspberry Pi 4B and watch the video. first a colour gradient will show on screen, then it will black out, then the video will play.
11. After the video stops the performance metrics will be sent over UART.
