//main.c
//authored by Tony Brown
//
//
//tasks 1 and 2 blink the ACT LED
//main initialises the devices and IP tasks
#include <stdio.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <event_groups.h>
#include <task.h>

#include "interrupts.h"
#include "gpio.h"
#include "video.h"

#include "portmacro.h"
#include "uspi.h"
#include "uspios.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOSIPConfigDefaults.h"
#include "FreeRTOSIPConfig.h"
#include "FreeRTOS_IP_Private.h"

#include "main.h"
#include "ds18b20.h"

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS( xTimeInMs ) ( ( portTickType ) xTimeInMs * ( configTICK_RATE_HZ / ( ( portTickType ) 1000 ) ) )
#endif

#define PRINT_CTL_REG_ONLY
#define ICACHE_ENABLE 0x00001000
#define BRANCH_PREDICTION_ENABLE 0x00000800
#define DCACHE_ENABLE 0x00000004

#define TBYTES 74


void task1() {
	int i = 0;
	while(1) {
		i++;
		SetGpio(47, 1);
		vTaskDelay(200);
	}
}


void task2() {
	int i = 0;
	while(1) {
		i++;
		vTaskDelay(100);
		SetGpio(47, 0);
		vTaskDelay(100);
	}
}


void
printAddressConfiguration() {

    uint32_t ip, netmask, gwaddr, dnsserver;

    FreeRTOS_GetAddressConfiguration(&ip, &netmask, &gwaddr, &dnsserver);
    println("ADDR INFO:", WHITE_TEXT);
    printHex("IP: ", ip, WHITE_TEXT);
    printHex("Netmask: ", netmask, WHITE_TEXT);
    printHex("GWADDR: ", gwaddr, WHITE_TEXT);
    printHex("DNS: ", dnsserver, WHITE_TEXT);
}


#define NUM_TEMPERATURE_CHARS 13
struct qItem {
    char msg[NUM_TEMPERATURE_CHARS];
};

xQueueHandle tque;
const portTickType qwait4s = pdMS_TO_TICKS( 4000UL );


static void prvServerConnectionInstance( void *pvParameters );


#define tcpechoSHUTDOWN_DELAY	( pdMS_TO_TICKS( 5000 ) )

//This would be defined if more that one client connection task worked.
#undef CREATE_SOCK_TASK

#define DEBUG_TEMP_TASK 0


/**********************************
 * Temperature server task. 
 * Reads the temperature data from 
 * the DS18B20 task.
 * Sends it to connected client.
*///////////////////////////////// 
void vTemperatureServerTask(){
        int status = 0;
        static const portTickType xReceiveTimeOut = portMAX_DELAY;
        const portTickType xDelay8s = pdMS_TO_TICKS( 8000UL );
        const portTickType xDelay500ms = pdMS_TO_TICKS( 500UL );
        const BaseType_t xBacklog = 4;
        struct qItem qi;

        TickType_t xTimeOnShutdown;
        uint8_t *pucRxBuffer;
        // for debug
        FreeRTOS_Socket_t * sockt;

	//setup a socket structure
        //loaded = 2;
        //printAddressConfiguration();
        volatile int times = 10;
        println("Server task starting", BLUE_TEXT);
        if (FreeRTOS_IsNetworkUp()) {
            println("Network is UP", BLUE_TEXT);
        }
        else {
            println("Network is Down", BLUE_TEXT);
            while (!FreeRTOS_IsNetworkUp()) {
                vTaskDelay( xDelay500ms );
            }
        }
        #if DEBUG_TEMP_TASK == 1
        println("Serv tsk done wait net", BLUE_TEXT);
        #endif

        Socket_t listen_sock;
	listen_sock = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
        if (listen_sock == FREERTOS_INVALID_SOCKET) {
            println("Socket is NOT valid", GREEN_TEXT);
        }
        else {
            println("Socket is valid", BLUE_TEXT);
        }

        /* Set a time out so accept() will just wait for a connection. */
        FreeRTOS_setsockopt( listen_sock,
                         0,
                         FREERTOS_SO_RCVTIMEO,
                         &xReceiveTimeOut,
                         sizeof( xReceiveTimeOut ) );


        /**
         ** If I dont set REUSE option, accept will never return
        **/
        BaseType_t xReuseSocket = pdTRUE;
        FreeRTOS_setsockopt( listen_sock,
                     0,
                     FREERTOS_SO_REUSE_LISTEN_SOCKET,
                     (void *)&xReuseSocket,
                     sizeof( xReuseSocket ) );

        struct freertos_sockaddr server, client;
        server.sin_port = FreeRTOS_htons((uint16_t)2056);
	//server.sin_addr = FreeRTOS_inet_addr("192.168.1.9");

        socklen_t cli_size = sizeof(client);
	status = FreeRTOS_bind(listen_sock, &server, sizeof(server));
        printHex("Bind status: ", (int)status, BLUE_TEXT);
        sockt = (FreeRTOS_Socket_t*)listen_sock;
        printHex("Bind port: ", (unsigned int)sockt->usLocalPort, BLUE_TEXT);

        println("Server task about to listen", BLUE_TEXT);
	status = FreeRTOS_listen(listen_sock, xBacklog);
        printHex("listen status: ", (int)status, BLUE_TEXT);


        int clients = 0;
        int32_t lBytes, lSent, lTotalSent;


            #ifdef CREATE_SOCK_TASK
            for ( ;; ) {
                //Only seem to be able to spawn one client task that works.  Additional client tasks don't work.
                println("Server task accepting", BLUE_TEXT);
	        Socket_t connect_sock = FreeRTOS_accept(listen_sock, (struct freertos_sockaddr*)&client, &cli_size);
                println("    accepted", BLUE_TEXT);

                xTaskCreate( prvServerConnectionInstance, "WeatherServer", 4096, ( void * ) connect_sock, tskIDLE_PRIORITY, NULL );
            }
            #else

            println("Server task accepting", BLUE_TEXT);
	    Socket_t connect_sock = FreeRTOS_accept(listen_sock, (struct freertos_sockaddr*)&client, &cli_size);
            println("    accepted", BLUE_TEXT);

	    pucRxBuffer = ( uint8_t * ) pvPortMalloc( ipconfigTCP_MSS );
                for ( ;; ) {
                    memset( pucRxBuffer, 0x00, ipconfigTCP_MSS );
                    if (  (lBytes = FreeRTOS_recv(connect_sock, pucRxBuffer, ipconfigTCP_MSS, 0)) > 0) {
                        #if DEBUG_TEMP_TASK == 1
                        printHex("Chars Received: ", (unsigned int)lBytes, BLUE_TEXT);
                        #endif
                        if (pdPASS == xQueueReceive(tque, &qi, qwait4s))
                        {
                            #if DEBUG_TEMP_TASK == 1
                            println("Server: recd from q", BLUE_TEXT);
                            println(qi.msg, BLUE_TEXT);
                            #endif
                            lBytes = NUM_TEMPERATURE_CHARS;
                            lSent = 0;
                            lTotalSent = 0;
                            while ((lSent >= 0) && (lTotalSent < lBytes)) {
                                lSent = FreeRTOS_send(connect_sock, &qi.msg, lBytes-lTotalSent, 0);
                                lTotalSent += lSent;
                            }
                            if (lSent < 0)
                                break;
                        }
                        else {
                            println("Server: could not read from q", RED_TEXT);
                        }
                    }
                    else {
                        FreeRTOS_shutdown(connect_sock, FREERTOS_SHUT_RDWR);

                        /* Wait for the shutdown to take effect, indicated by FreeRTOS_recv()
                        returning an error. */
                        xTimeOnShutdown = xTaskGetTickCount();
	                do
	                {
		            if( FreeRTOS_recv( connect_sock, pucRxBuffer, ipconfigTCP_MSS, 0 ) < 0 )
		            {
			        break;
		            }
	                } while( ( xTaskGetTickCount() - xTimeOnShutdown ) < tcpechoSHUTDOWN_DELAY );

                        vPortFree( pucRxBuffer );
                        FreeRTOS_closesocket( connect_sock );

                        break;
                    }
                }


            #endif


        #ifndef CREATE_SOCK_TASK
        vTaskDelay( xDelay8s );
        for (;;) {
            println("Srv Tsk spin", BLUE_TEXT);
            vTaskDelay( xDelay8s );
        }
        #endif

}

#define tcpechoSHUTDOWN_DELAY	( pdMS_TO_TICKS( 5000 ) )


/**********************************
 * client connection handler task  
 * function. Currently not used 
 * as only 1 instance can be created.
 * 
*//////////////////////////////////
static void prvServerConnectionInstance( void *pvParameters )
{
int32_t lBytes, lSent, lTotalSent;
Socket_t xConnectedSocket;
static const TickType_t xReceiveTimeOut = pdMS_TO_TICKS( 5000 );
static const TickType_t xSendTimeOut = pdMS_TO_TICKS( 5000 );
TickType_t xTimeOnShutdown;
uint8_t *pucRxBuffer;

	xConnectedSocket = ( Socket_t ) pvParameters;

	/* Attempt to create the buffer used to receive the string to be echoed
	back.  This could be avoided using a zero copy interface that just returned
	the same buffer. */
	pucRxBuffer = ( uint8_t * ) pvPortMalloc( ipconfigTCP_MSS );

	if( pucRxBuffer != NULL )
	{
		FreeRTOS_setsockopt( xConnectedSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );
		FreeRTOS_setsockopt( xConnectedSocket, 0, FREERTOS_SO_SNDTIMEO, &xSendTimeOut, sizeof( xReceiveTimeOut ) );

		for( ;; )
		{
			/* Zero out the receive array so there is NULL at the end of the string
			when it is printed out. */
			memset( pucRxBuffer, 0x00, ipconfigTCP_MSS );

			/* Receive data on the socket. */
			lBytes = FreeRTOS_recv( xConnectedSocket, pucRxBuffer, ipconfigTCP_MSS, 0 );

			/* If data was received, echo it back. */
			if( lBytes >= 0 )
			{
				lSent = 0;
				lTotalSent = 0;

				/* Call send() until all the data has been sent. */
				while( ( lSent >= 0 ) && ( lTotalSent < lBytes ) )
				{
					lSent = FreeRTOS_send( xConnectedSocket, pucRxBuffer, lBytes - lTotalSent, 0 );
					lTotalSent += lSent;
				}

				if( lSent < 0 )
				{
					/* Socket closed? */
					break;
				}
			}
			else
			{
				/* Socket closed? */
				break;
			}
		}
	}

	/* Initiate a shutdown in case it has not already been initiated. */
	FreeRTOS_shutdown( xConnectedSocket, FREERTOS_SHUT_RDWR );

	/* Wait for the shutdown to take effect, indicated by FreeRTOS_recv()
	returning an error. */
	xTimeOnShutdown = xTaskGetTickCount();
	do
	{
		if( FreeRTOS_recv( xConnectedSocket, pucRxBuffer, ipconfigTCP_MSS, 0 ) < 0 )
		{
			break;
		}
	} while( ( xTaskGetTickCount() - xTimeOnShutdown ) < tcpechoSHUTDOWN_DELAY );

	/* Finished with the socket, buffer, the task. */
	vPortFree( pucRxBuffer );
	FreeRTOS_closesocket( xConnectedSocket );

	vTaskDelete( NULL );
}



/**********************************
 * local assert function  
 * 
 * 
*//////////////////////////////////
void
tassert_fail(const char *expression, const char *pfile, unsigned lineno)
{
    println(expression, 0xFFFF0000);
    println(pfile, 0xFFFF0000);
    printHex("Line ", lineno, 0xFFFF0000);
    while(1){;} //system failure
}



/**********************************
 * the temp data format for  
 * data converted from bits.
 * 
*//////////////////////////////////
typedef struct _temp
{
    int intpart;
    int fracpart;
} temp_t;


/**********************************
 * convert the ds18B20 bits to 
 * to a ds18B20 temperature 
 * integer and fractional parts 
*//////////////////////////////////
int
convert_temp(unsigned char lsb, unsigned char msb, temp_t * temp)
{
    int neg = 0;
    unsigned int val = (msb << 8) | lsb;
    if (msb & 0x80) {
        neg = 1;
        val = (~val+1)&0x0000FFFF;
    }

    temp->intpart = (val & 0x07F0) >> 4;
    if (neg) temp->intpart = -temp->intpart;
    temp->fracpart = 0;
    //express the fractional part as a 4 digit integer
    if ((lsb & 0x8) == 0x8)
        temp->fracpart += 5000;  // 0.5
    if ((lsb & 0x4) == 0x4)
        temp->fracpart += 2500;  // 0.25
    if ((lsb & 0x2) == 0x2)
        temp->fracpart += 1250;  // 0.1250
    if ((lsb & 0x1) == 0x1)
        temp->fracpart += 625;   // 0.0625
}


/**********************************
 * Simple modulo function
 * 
*//////////////////////////////////
int modulo(int num, int mod)
{
    while (num >= mod)
    {
       num -= mod;
    }
    return num;
}

/**********************************
 * dedicated num2string function
 * for the ds18B20 temperature format
*//////////////////////////////////
void
num2string(int isIntPart, int num, char * buf)
{ // assume buf has room for four (not intpart) or five (intpart) chars
  int index = 0;

  if (isIntPart)
  {
      if (num<0) {
          buf[index] = '-';
          num = -num;
      }
      else
          buf[index] = ' ';

      index = 1;
  }

  buf[index++] = (char)(num/1000 + '0');
  if (num >= 1000) num = modulo(num, 1000);
  buf[index++] = (char)(num/100 + '0');
  if (num >= 100) num = modulo(num, 100);
  buf[index++] = (char)(num/10 + '0');
  if (num >= 10) num = modulo(num, 10);
  buf[index++] = (char)(num + '0');
}


#define DEBUG_DS18B20 0

#define GPIO_PIN_4 4
void vDS18B20Task ( void * pvParameters)
{
    int i;
    int result = 0;
    w1_block_obj_t rom, block;
    memset(&rom, 0, sizeof(w1_block_obj_t));
    memset(&block, 0, sizeof(w1_block_obj_t));
    temp_t temp;

    struct qItem qi;


    ds18b20_readrom_single(GPIO_PIN_4, &rom);
    for (i=0; i<8; ++i)
    {
        printHex("Rom index: ", (unsigned int)i, GREEN_TEXT);
        printHex("Rom val: ", (unsigned int)rom.data[i], GREEN_TEXT);
    }

    for ( ;; )
    {
        result = ds18b20_readtemp_single(GPIO_PIN_4, &block);
        {
            #if DEBUG_DS18B20 == 1
            if (0 == result)
            {
                for (i=0; i<9; ++i)
                {
                    printHex("Block index: ", (unsigned int)i, GREEN_TEXT);
                    printHex("Block val: ", (unsigned int)block.data[i], GREEN_TEXT);
                }
            }
            #endif
            if (0 == result)
            {
                convert_temp(block.data[0], block.data[1], &temp);
                #if DEBUG_DS18B20 == 1
                printHex("Temp Int  part: ", (unsigned int)temp.intpart, GREEN_TEXT);
                printHex("Temp Frac part: ", (unsigned int)temp.fracpart, GREEN_TEXT);
                #endif
                num2string(1, temp.intpart, (char*)&qi.msg[0]);
                qi.msg[5] = '.';
                num2string(0, temp.fracpart, (char *)&qi.msg[6]);
                qi.msg[10] = '\r';
                qi.msg[11] = '\n';
                qi.msg[12] = '\0';

                if (pdPASS == xQueueSendToBack(tque, &qi, qwait4s)) {
                    #if DEBUG_DS18B20 == 1
                    println("DS18B20: sent to queue", GREEN_TEXT);
                    #endif
                }
                else {
                    #if DEBUG_DS18B20 == 1
                    println("DS18B20: failed queue", RED_TEXT);
                    #endif
                }
            }
            else {
                println("DS18B20 : failed to read temp", RED_TEXT);
            }

        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}



void
setSystemCtlReg() {
    int sysctl = cpuinfo_getSystemCtlReg();
    printHex("SCTLR: ", sysctl, 0xFFFFFFFF);
    #ifndef PRINT_CTL_REG_ONLY
    sysctl |= ICACHE_ENABLE | BRANCH_PREDICTION_ENABLE | DCACHE_ENABLE;
    cpuinfo_setSystemCtlReg(sysctl);
    sysctl = cpuinfo_getSystemCtlReg();
    printHex("SCTLR Settings: ", sysctl, 0xFFFFFFFF);
    #endif
}

void printCPUID() {
    int mainID = cpuinfo_getMainIDReg();
    printHex("Main ID: ", mainID, 0xFFFFFFFF);
}

void printMemModelFeat0() {
    int memID = cpuinfo_getIDMMFR0();
    printHex("Mem Model Feat0: ", memID, 0xFFFFFFFF);
}



int
main(void)
{
    initFB();
    DisableInterrupts();
    //cpuinfo_setSystemCtlReg();
    InitInterruptController();

    //printCPUID();

    //ensure the IP and gateway match the router settings!
    const unsigned char ucIPAddress[ 4 ] = {192, 168, 1, 9};
    const unsigned char ucNetMask[ 4 ] = {255, 255, 255, 0};
    const unsigned char ucGatewayAddress[ 4 ] = {192, 168, 1, 1};
    const unsigned char ucDNSServerAddress[ 4 ] = {192, 168, 1, 1};
    //ethernet usb
    const unsigned char ucMACAddress[ 6 ] = {0xB8, 0x27, 0xEB, 0xa5, 0x35, 0xc1};
    //wireless usb
    //const unsigned char ucMACAddress[ 6 ] = {0x74, 0xda, 0x38, 0x58, 0xcf, 0x4c};

    FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);
    printAddressConfiguration();
    MsDelay(2000);

    loaded = 1;

    tque = xQueueCreate((portBASE_TYPE)4, (portBASE_TYPE)sizeof(struct qItem));
    if (!tque) {
        println("tque not created", BLUE_TEXT);
    }
    else {
        println("tque created", BLUE_TEXT);

        //  args      the code      name     stack depth   parameters  priority   task handle
        // The LED tasks give external visual indication 
        xTaskCreate( task1, "LED task 1", 1000,       NULL,        1,       NULL);
        xTaskCreate( task2, "LED task 2", 1000,       NULL,        1,       NULL);

        // Create the DS18B20 task and it's server.
        xTaskCreate( vDS18B20Task, "DS18B20Task", 1000,       NULL,        1,       NULL);
        xTaskCreate(vTemperatureServerTask, "WeatherServer", 8192, NULL, 1, NULL);

        // Network wont come up until the scheduler starts, ethernet task and IP task must start.
        vTaskStartScheduler();
    }

    for (;;) {
        println("ERROR:MAIN", 0xFFFFFFFF);
    }


}

/*********************************
 *  HANDLERS FOR nasty events,
 *    see startup.s
 *
 *  These make identifying the
 *  failure easier
*/
void
DAbort(void) {
    println("Data Abort", RED_TEXT);
    while (1) {}
}

void
PFetch(void) {
    println("Prefetch Abort", RED_TEXT);
    while (1) {}
}

void
FIQ(void) {
    println("FIQ", RED_TEXT);
    while (1) {}
}

void
UNUSED(void) {
    println("UNUSED", RED_TEXT);
    while (1) {}
}

void
HANG(void) {
    println("HANG", RED_TEXT);
    while (1) {}
}

void
UNDEFINED_INST(void) {
    println("UNDEFINED_INST", RED_TEXT);
    while (1) {}
}



