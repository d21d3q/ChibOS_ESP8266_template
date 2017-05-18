#include "ch.h"
#include "hal.h"
#include "esp8266.h"
#include "chprintf.h"

#define WIFINAME "kontiki"
#define WIFIPASS "rikitiki"

/* ESP working structure and result enumeration */
evol ESP_t ESP;
ESP_Result_t espRes;
uint8_t myAPIP[4] = {192, 168, 2, 1};
uint8_t mySTAIP[4] = {192, 168, 1, 22};
uint8_t hisSTAIP[4];
uint8_t mySTAGWMSK[] = {192, 168, 1, 1, 255, 255, 255, 0};

ESP_APConfig_t myApConf = { "ChibiWifi", "supersecret", ESP_Ecn_WPA2_PSK, 3, 1, 0};

static THD_WORKING_AREA(waEspUpdate, 512);
static THD_WORKING_AREA(waEspMain, 512);

static virtual_timer_t EspTimeUpdateTimer;

/* Client connection pointer */
ESP_CONN_t* conn;

/* Connection manupulation */
uint32_t bw;
const uint8_t responseTemplate[] = ""
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n"
"\r\n"
"<html>\n"
"   <head>\n"
"       <meta http-equiv=\"Refresh\" content=\"1\" />\n"
"   </head>\n"
"   <body>\n"
"       <h1>Welcome to web server produced by ESP8266 Wi-Fi module!</h1>\n"
"       This website will constantly update itself every 1 second! <br>\n"
"       Current system time is %d\n"
"   </body>\n"
"</html>\n";

#define RESPT_BUF_SIZE	512
uint8_t respBuf[RESPT_BUF_SIZE];
uint32_t respSize;
/* ESP callback declaration */
int ESP_Callback(ESP_Event_t evt, ESP_EventParams_t* params);


static THD_FUNCTION(EspUpdateThread, arg) {
	(void)arg;
	chRegSetThreadName("esp update thread");

	while(1) {
        /* Process ESP update */
        ESP_Update(&ESP);
        chThdYield();
    }
}

static THD_FUNCTION(EspMainThread, arg) {
	(void)arg;
	chRegSetThreadName("esp main thread");

    /* Init ESP library with 115200 bauds */
    if ((espRes = ESP_Init(&ESP, 115200, ESP_Callback)) == espOK) {
        chprintf((BaseSequentialStream *)&SD2, "ESP module initialized successfully!\r\n");
    } else {
        chprintf((BaseSequentialStream*)&SD2, "ESP Init error. Status: %d\r\n", espRes);
    }


    if ((espRes = ESP_AP_SetIP(&ESP, myAPIP, 0, 1)) == espOK) {
        chprintf((BaseSequentialStream*)&SD2, "Succesfully set ip for AP\r\n");
    } else {
        chprintf((BaseSequentialStream*)&SD2, "Problems with setting ip for AP. Status: %d\r\n", espRes);
    }

    /* Try to connect to wifi network in blocking mode */
    if ((espRes = ESP_AP_SetConfig(&ESP, &myApConf, 0, 1)) == espOK) {
        chprintf((BaseSequentialStream*)&SD2, "Succesfully set config for AP\r\n");
    } else {
        chprintf((BaseSequentialStream*)&SD2, "Problems with setting config for AP. Status: %d\r\n", espRes);
    }

    /* Try to connect to wifi network in blocking mode */
    if ((espRes = ESP_STA_SetIP(&ESP, mySTAIP, mySTAGWMSK, 0, 1)) == espOK) {
        chprintf((BaseSequentialStream*)&SD2, "Succesfully set ip for STA\r\n");
    } else {
        chprintf((BaseSequentialStream*)&SD2, "Problems with setting ip for STA. Status: %d\r\n", espRes);
    }

    /* Try to connect to wifi network in blocking mode */
    if ((espRes = ESP_STA_Connect(&ESP, WIFINAME, WIFIPASS, NULL, 0, 1)) == espOK) {
        chprintf((BaseSequentialStream*)&SD2, "Connected to network\r\n");
    } else {
        chprintf((BaseSequentialStream*)&SD2, "Problems trying to connect to network: %d\r\n", espRes);
    }

    /* Try to connect to wifi network in blocking mode */
    if ((espRes = ESP_STA_GetIP(&ESP, hisSTAIP, 1)) == espOK) {
        chprintf((BaseSequentialStream*)&SD2, "Succesfully obtained STA ip: %d.%d.%d.%d\r\n", (uint32_t)hisSTAIP[0],
																							  (uint32_t)hisSTAIP[1],
																							  (uint32_t)hisSTAIP[2],
																							  (uint32_t)hisSTAIP[3]);
    } else {
        chprintf((BaseSequentialStream*)&SD2, "Problems with setting ip. Status: %d\r\n", espRes);
    }

    /* Enable server mode on port 80 (HTTP) */
    if ((espRes = ESP_SERVER_Enable(&ESP, 80, 1)) == espOK) {
        chprintf((BaseSequentialStream*)&SD2, "Server mode is enabled. Try to connect to %d.%d.%d.%d to see the magic\r\n", ESP.STAIP[0], ESP.STAIP[1], ESP.STAIP[2], ESP.STAIP[3]);
    } else {
        chprintf((BaseSequentialStream*)&SD2, "Problems trying to enable server mode: %d\r\n", espRes);
    }
    while (1) {
        ESP_ProcessCallbacks(&ESP);         /* Process all callbacks */
        chThdYield();
    }
}

/***********************************************/
/**               Library callback            **/
/***********************************************/
int ESP_Callback(ESP_Event_t evt, ESP_EventParams_t* params) {
    ESP_CONN_t* conn;
    uint8_t* data;

    switch (evt) {                              /* Check events */
        case espEventIdle:
            chprintf((BaseSequentialStream*)&SD2, "Stack is IDLE!\r\n");
            break;
        case espEventConnActive: {
            conn = (ESP_CONN_t *)params->CP1;   /* Get connection for event */
            chprintf((BaseSequentialStream*)&SD2, "Connection %d from %d.%d.%d.%d just became active!\r\n", conn->Number,
																										    (uint32_t)conn->RemoteIP[0],
																										    (uint32_t)conn->RemoteIP[1],
																										    (uint32_t)conn->RemoteIP[2],
																										    (uint32_t)conn->RemoteIP[3]);
            break;
        }
        case espEventConnClosed: {
            conn = (ESP_CONN_t *)params->CP1;   /* Get connection for event */
            chprintf((BaseSequentialStream*)&SD2, "Connection %d was just closed!\r\n", conn->Number);
            break;
        }
        case espEventDataReceived: {
            conn = (ESP_CONN_t *)params->CP1;   /* Get connection for event */
            data = (uint8_t *)params->CP2;      /* Get data */

            /* Notify user about informations */
            chprintf((BaseSequentialStream*)&SD2, "Data received: %d bytes\r\n", params->UI);

            if (ESP_IsReady(&ESP) == espOK) {   /* Send data back when we have received all the data from device */
                if (strstr((char *)data, "/favicon")) { /* When browser requests favicon image, ignore it! */
                    ESP_CONN_Close(&ESP, conn, 0);      /* Close connection directly on favicon request */
                } else {
                	respSize = chsnprintf((char*)respBuf, RESPT_BUF_SIZE, (char*)responseTemplate, (uint32_t)chVTGetSystemTime());
                    espRes = ESP_CONN_Send(&ESP, conn, respBuf, respSize, &bw, 0); /* Send data on other requests */
                }
            }
            break;
        }
        case espEventDataSent:
            conn = (ESP_CONN_t *)params->CP1;   /* Get connection for event */
            chprintf((BaseSequentialStream*)&SD2, "Data sent conn: %d\r\n", conn->Number);
            chprintf((BaseSequentialStream*)&SD2, "Close conn resp: %d\r\n", ESP_CONN_Close(&ESP, conn, 0));
            break;
        case espEventDataSentError:
            conn = (ESP_CONN_t *)params->CP1;   /* Get connection for event */
            ESP_CONN_Close(&ESP, conn, 0);
            break;
        default:
            break;
    }

    return 0;
}

static void EspUpdateTime_cb(void *arg) {
  (void)arg;
//  LED_toggle();
	ESP_UpdateTime(&ESP, 1);
  chSysLockFromISR();
  chVTSetI(&EspTimeUpdateTimer, MS2ST(1), EspUpdateTime_cb, NULL);
  chSysUnlockFromISR();
}

void StartEspThreads(void){
	chThdCreateStatic(waEspUpdate, sizeof(waEspUpdate), NORMALPRIO, EspUpdateThread, NULL);
	chThdCreateStatic(waEspMain, sizeof(waEspMain), NORMALPRIO, EspMainThread, NULL);
//	chThdCreateStatic(waEspTimeUpdate, sizeof(waEspTimeUpdate), HIGHPRIO, EspTimeUpdateThread, NULL);

	chVTObjectInit(&EspTimeUpdateTimer);
	chVTSet(&EspTimeUpdateTimer, MS2ST(1), EspUpdateTime_cb, NULL);
}
