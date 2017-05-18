# Template for using ESP8266 with ChibiOS

This repository is intended as a base for creating new projects.

Project is based on example from Chibios demo for STM32F4_DISCOVERY board and uses  [ESP8266_AT_Commands_parser](https://github.com/d21d3q/ESP8266_AT_Commands_parser)
library.

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
