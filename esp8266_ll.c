/**	
   ----------------------------------------------------------------------
    Copyright (c) 2016 Tilen Majerle

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software, 
    and to permit persons to whom the Software is furnished to do so, 
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
    AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------
 */
#include <esp8266_ll.h>
#include "ch.h"
#include "hal.h"

BSEMAPHORE_DECL(EspSem, 0);

#ifndef ESP_LL_USE_UART
static THD_WORKING_AREA(waEspReceiveData, 512);
static THD_FUNCTION(EspReceiveDataThread, arg) {
	(void)arg;
	chRegSetThreadName("esp receive data thread");
	uint8_t ch;
	while(1){
		ch = (uint8_t)sdGet(&SD3);
		ESP_DataReceived(&ch, 1);
	}
}
#else
#define BUFF_SIZE	64
uint8_t buff[BUFF_SIZE];
/*
 * This callback is invoked when a character is received but the application
 * was not ready to receive it, the character is passed as parameter.
 */
static void rxchar(UARTDriver *uartp, uint16_t c) {
  (void)uartp;
  uint8_t ch = (uint8_t)c;
  ESP_DataReceived(&ch, 1);
}

static UARTConfig EspUartConf= {
  NULL,
  NULL,
  NULL,
  rxchar,
  NULL,
  115200,
  0,
  0,
  0
};
#endif


uint8_t ESP_LL_Callback(ESP_LL_Control_t ctrl, void* param, void* result) {
    switch (ctrl) {
        case ESP_LL_Control_Init: {                 /* Initialize low-level part of communication */
            ESP_LL_t* LL = (ESP_LL_t *)param;       /* Get low-level value from callback */
            (void)LL;
            /************************************/
            /*  Device specific initialization  */
            /************************************/
#ifndef ESP_LL_USE_UART
        	SerialConfig sConf;
        	sConf.speed = 115200;
		    palSetPadMode(GPIOB, 11, PAL_MODE_ALTERNATE(7));
		    palSetPadMode(GPIOD, 8, PAL_MODE_ALTERNATE(7));
        	sdStart(&SD3, &sConf);
        	chThdCreateStatic(waEspReceiveData, sizeof(waEspReceiveData), HIGHPRIO, EspReceiveDataThread, NULL);
#else
			uartStart(&UARTD3, &EspUartConf);
		    palSetPadMode(GPIOB, 11, PAL_MODE_ALTERNATE(7));
		    palSetPadMode(GPIOD, 8, PAL_MODE_ALTERNATE(7));
#endif
            if (result) {
                *(uint8_t *)result = 0;             /* Successfully initialized */
            }
            return 1;                               /* Return 1 = command was processed */
        }
        case ESP_LL_Control_Send: {
            ESP_LL_Send_t* send = (ESP_LL_Send_t *)param;   /* Get send parameters */

            /* Send actual data to UART, implement function to send data */
#ifndef ESP_LL_USE_UART
        	sdWrite(&SD3, (uint8_t *)send->Data, send->Count);
            if (result) {
                *(uint8_t *)result = 0;             /* Successfully send */
            }
#else
			msg_t msg;
			size_t size = (size_t)send->Count;
			msg = uartSendTimeout(&UARTD3, &size, (uint8_t *)send->Data, 0xffffffff);
            if (result) {
                *(uint8_t *)result = (msg == MSG_OK)? 0 : 1;             /* Successfully send */
            }
#endif
            return 1;                               /* Command processed */
        }
        case ESP_LL_Control_SetReset: {             /* Set reset value */
            uint8_t state = *(uint8_t *)param;      /* Get state packed in uint8_t variable */
            (void)state;
            /* not used */
            return 1;                               /* Command has been processed */
        }
        case ESP_LL_Control_SetRTS: {               /* Set RTS value */
            uint8_t state = *(uint8_t *)param;      /* Get state packed in uint8_t variable */
            (void)state;                            /* Prevent compiler warnings */
            return 1;                               /* Command has been processed */
        }
#if ESP_RTOS
        case ESP_LL_Control_SYS_Create: {           /* Create system synchronization object */
            ESP_RTOS_SYNC_t* Sync = (ESP_RTOS_SYNC_t *)param;   /* Get pointer to sync object */
            (void)Sync;
            /* nothing here since semaphore is declared in global scope */

            if (result) {
                *(uint8_t *)result = 0;    /*!< Set result value */
            }
            return 1;                               /* Command processed */
        }
        case ESP_LL_Control_SYS_Delete: {           /* Delete system synchronization object */
            ESP_RTOS_SYNC_t* Sync = (ESP_RTOS_SYNC_t *)param;   /* Get pointer to sync object */
            (void)Sync;
            /* nothing here since semaphore is declared in global scope */

            if (result) {
                *(uint8_t *)result = 1;    /*!< Set result value */
            }
            return 1;                               /* Command processed */
        }
        case ESP_LL_Control_SYS_Request: {          /* Request system synchronization object */
            ESP_RTOS_SYNC_t* Sync = (ESP_RTOS_SYNC_t *)param;   /* Get pointer to sync object */
            (void)Sync;                             /* Prevent compiler warnings */
            msg_t ret;
            ret = chBSemWaitTimeout(&EspSem, 1000);

            *(uint8_t *)result = (ret == MSG_OK) ? 0 : 1; /* Set result according to response */
            return 1;                               /* Command processed */
        }
        case ESP_LL_Control_SYS_Release: {          /* Release system synchronization object */
            ESP_RTOS_SYNC_t* Sync = (ESP_RTOS_SYNC_t *)param;   /* Get pointer to sync object */
            (void)Sync;                             /* Prevent compiler warnings */
            chBSemSignal(&EspSem);

            *(uint8_t *)result = 0;
            return 1;                               /* Command processed */
        }
#endif /* ESP_RTOS */
        default:
            return 0;
    }
}

