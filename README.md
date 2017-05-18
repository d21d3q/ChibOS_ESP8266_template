# Template for using ESP8266 with ChibiOS

This repository is intended as a base for creating new projects.

Project is based on example from Chibios demo for STM32F4_DISCOVERY board and uses  [ESP8266_AT_Commands_parser](https://github.com/d21d3q/ESP8266_AT_Commands_parser)
library.

On default system is running in tickless mode - that is why there are yeld
macros in update thread and main thread.

Thread working areas are not optimized. 

## Usage

In order to compile it you need to specify in Makefile path to directory
containing Chibios source code.
```
CHIBIOS = /home/d21d3q/programming/ChibiOS
```

For serial communication with ESP `UART3` is being used.
You can choose whether you want to use serial driver or native UART by
(un)commenting this line in Makefile:

```
UDEFS += -DESP_LL_USE_UART
```

`UART2` is being used for debug info with default speed 34800

#### Compilation

Just run `make` and flash it. With openocd:
```
openocd -f /usr/local/share/openocd/scripts/board/stm32f4discovery.cfg -c init -c targets -c "halt" -c "flash write_image erase build/ChibiWifi.elf" -c "verify_image build/ChibiWifi.elf" -c "reset run" -c shutdown
```
or import makefile project in [System Workbench for STM32](http://www.openstm32.org/System+Workbench+for+STM32)

In order to flash and debug code from System Workbench you need to have running
gdb server:
```
openocd -f /usr/local/share/openocd/scripts/board/stm32f4discovery.cfg -c init -c targets -c "halt"
```
This will start gdb server on port 3333.
