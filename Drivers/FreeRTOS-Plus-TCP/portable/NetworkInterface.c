/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"

#define TONY

#ifdef TONY
#include <uspi/assert.h>
#include "video.h"

#define TBDBG( msg ) println(msg, AQUA_TEXT);
#define TBDBGERR( msg ) println(msg, RED_TEXT);
#define TBHDBG( msg, val ) printHex(msg, (unsigned int)val, AQUA_TEXT);

void
printEthBuf(unsigned char* buf, unsigned long len)
{
    if (buf == NULL) {
        println("BUF NULL", 0xFFFFFFFF);
        return;
    }
    int sz = len;
    int extra = 0;
    while (sz > 0) {
        sz -= 4;
        if (sz > 0 && sz < 4) break;
    }
    if (sz > 0) extra = 1;
    unsigned int * bufptr = (unsigned int*)buf;
    for (int i=0; i<extra + len/4; ++i) {
        printHex("", FreeRTOS_ntohl(*bufptr), 0xFFFFFFFF);
        bufptr++;
    }
}
#else
#define TBDBG( msg )
#define TBHDBG( msg, val )
#define printEthBuf( buf, len)
#endif

/* The queue used to pass events into the IP-task for processing. */
xQueueHandle xOutputQueue = NULL;

typedef struct OutputInfo_asdf{
	int pNetworkBufferDescriptor_t;
	int bReleaseAfterSend;
} OutputInfo;

void ethernetPollTask(){
	unsigned char *pucUseBuffer;
	int ulReceiveCount, ulResult;
	static NetworkBufferDescriptor_t *pxNextNetworkBufferDescriptor = NULL;
	const unsigned long xMinDescriptorsToLeave = 2UL;
	const portTickType xBlockTime = pdMS_TO_TICKS( 100UL );
	static IPStackEvent_t xRxEvent = { eNetworkRxEvent, NULL };

	//create a queue to store addresses of NetworkBufferDescriptors
	xOutputQueue = xQueueCreate( ( unsigned long ) ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS, 4 );

        #ifdef TONY
        int len;
        #endif 

	for( ;; ){
                
		//send any waiting packets first
		for(int i = 0; i < uxQueueMessagesWaiting(xOutputQueue); i++){
//TBDBG("try send")
			OutputInfo out;
			//xQueueReceive(xOutputQueue, &out, 1000);
                        //int mwaiting = (int)uxQueueMessagesWaiting(xOutputQueue);
                        //printHex("Eth Msgs: ", (unsigned int)mwaiting, BLUE_TEXT);
			xQueueReceive(xOutputQueue, &out, portMAX_DELAY);
                        //printHex("Eth: out: ", (unsigned int)out.pNetworkBufferDescriptor_t, BLUE_TEXT);
			NetworkBufferDescriptor_t * const pxDescriptor = (NetworkBufferDescriptor_t*)out.pNetworkBufferDescriptor_t; 
                        len = (int)pxDescriptor->xDataLength;
                        //printHex("Ethout pxd puc: ", (unsigned int)pxDescriptor->pucEthernetBuffer, BLUE_TEXT);
                        #ifdef TONY
                        assert(pxDescriptor->pucEthernetBuffer != NULL);
                        //printEthBuf(pxDescriptor->pucEthernetBuffer, pxDescriptor->xDataLength);
                        #endif
                        //if (pxDescriptor->pucEthernetBuffer != NULL) {
			USPiSendFrame((void *)pxDescriptor->pucEthernetBuffer, pxDescriptor->xDataLength);
                        //}
			if(out.bReleaseAfterSend) vReleaseNetworkBufferAndDescriptor( pxDescriptor );
//TBHDBG("sent len", len)
		}

		/* If pxNextNetworkBufferDescriptor was not left pointing at a valid
		descriptor then allocate one now. */
		if( ( pxNextNetworkBufferDescriptor == NULL ) && ( uxGetNumberOfFreeNetworkBuffers() > xMinDescriptorsToLeave ) )
		{
                        //TBDBG("Allocating new Desc")
			pxNextNetworkBufferDescriptor = pxGetNetworkBufferWithDescriptor( ipTOTAL_ETHERNET_FRAME_SIZE, xBlockTime );
                        //if (pxNextNetworkBufferDescriptor != NULL) {
                            //TBDBG("Allocating new Desc SUCCESS")
                        //}
		}
		if( pxNextNetworkBufferDescriptor != NULL )
		{
			/* Point pucUseBuffer to the buffer pointed to by the descriptor. */
			pucUseBuffer = ( unsigned char* ) ( pxNextNetworkBufferDescriptor->pucEthernetBuffer - ipconfigPACKET_FILLER_SIZE );
		}
		else
		{
			/* As long as pxNextNetworkBufferDescriptor is NULL, the incoming
			messages will be flushed and ignored. */
                        TBDBGERR("PUC BUF NULL!")
			pucUseBuffer = NULL;
		}

		/* Read the next packet from the hardware into pucUseBuffer. */
		ulResult = USPiReceiveFrame (pucUseBuffer, &ulReceiveCount);

		if( ( ulResult != 1 ) || ( ulReceiveCount == 0 ) )
		{
			/* No data from the hardware. */
			continue;//break;
		}
#ifdef TONY
//TBHDBG("Frame received len", ulReceiveCount)
// if length is 3c it is likely an ARP packet
//if(ulReceiveCount != 0x3c) 
//for(int i = 0; i < ulReceiveCount; i++){printHex("", ((char*)pucUseBuffer)[i], 0xFFFFFFFF);}
//if (ulReceiveCount != 0x3c) {
    //println("NEW WAY", 0xFFFFFFFF);
//printEthBuf(pucUseBuffer, ulReceiveCount);
#endif

		if( pxNextNetworkBufferDescriptor == NULL )
		{
			/* Data was read from the hardware, but no descriptor was available
			for it, so it will be dropped. */
			iptraceETHERNET_RX_EVENT_LOST();
			continue;
		}

		iptraceNETWORK_INTERFACE_RECEIVE();
		pxNextNetworkBufferDescriptor->xDataLength = ( size_t ) ulReceiveCount;
		xRxEvent.pvData = ( void * ) pxNextNetworkBufferDescriptor;

		/* Send the descriptor to the IP task for processing. */
		if( xSendEventStructToIPTask( &xRxEvent, xBlockTime ) != pdTRUE )
		{
			/* The buffer could not be sent to the stack so must be released 
			again. */
			vReleaseNetworkBufferAndDescriptor( pxNextNetworkBufferDescriptor );
			iptraceETHERNET_RX_EVENT_LOST();
			FreeRTOS_printf( ( "prvEMACRxPoll: Can not queue return packet!\n" ) );
		}
		
		/* Now the buffer has either been passed to the IP-task,
		or it has been released in the code above. */
		pxNextNetworkBufferDescriptor = NULL;
	}
}

portBASE_TYPE xNetworkInterfaceInitialise(){
	if (!USPiInitialize ()){
		println("Cannot initialize USPi", 0xFFFFFFFF);
		return pdFAIL;
	}

	if (!USPiEthernetAvailable ()){
		println("Ethernet device not found", 0xFFFFFFFF);
		return pdFAIL;
	}

	xTaskCreate(ethernetPollTask, "poll", 1024, NULL, 0, NULL);

	return pdPASS;
}

portBASE_TYPE xNetworkInterfaceOutput( NetworkBufferDescriptor_t * const pxDescriptor, portBASE_TYPE bReleaseAfterSend ){
	OutputInfo out;
        #ifdef TONY
	//portTickType xBlockTime = pdMS_TO_TICKS( 100UL );
        //println("Ethout: ", VIOLET_TEXT);
        //printHex("Ethout pxd: ", (unsigned int)pxDescriptor, AQUA_TEXT);
        //printHex("Ethout pxd puc: ", (unsigned int)pxDescriptor->pucEthernetBuffer, AQUA_TEXT);
        //printEthBuf(pxDescriptor->pucEthernetBuffer, pxDescriptor->xDataLength);
        //if (bReleaseAfterSend) {
        //     println("ETHOUT: RAS True", AQUA_TEXT);
        //}
        //else {
        //     println("ETHOUT: RAS False", AQUA_TEXT);
        //}

	//NetworkBufferDescriptor_t * pxNetworkBuffer = pxDuplicateNetworkBufferWithDescriptor( pxDescriptor, pxDescriptor->xDataLength );
        #endif
	out.pNetworkBufferDescriptor_t = (int)pxDescriptor;
	out.bReleaseAfterSend = bReleaseAfterSend;
	xQueueSendToBack(xOutputQueue, &out, 1000);
	return 1;
}
