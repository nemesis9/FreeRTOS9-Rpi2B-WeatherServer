//onewire.c
//authored by Tony Brown
//

#ifndef NDEBUG 
#include <video.h>
#endif

#include <gpio.h>
#include <portmacro.h>
#include "onewire.h"

//static int w1_reset(int pin);
//static void w1_write(int pin, unsigned char b, int power);
//static void w1_read_block(int pin, int bytes, w1_block_obj_t *block_obj);
//static int w1_read_bit(int pin);

/* 20 counts overhead - ~5 us */
void waitUs(int u) {
   unsigned int s = GET_TIMER_COUNT();
   do {
   } while ( (GET_TIMER_COUNT() - s) < COUNTS_PER_US*u); 
}



void
w1_data(int pin, int state)
{
    if (state)
        SetGpioDirection(pin, GPIO_IN);
    else
        SetGpio(pin, 0);
        SetGpioDirection(pin, GPIO_OUT);
}




static void
w1_delay_us(int u, int plus_io)
{
    waitUs(u);    
}


static int 
w1_getbit(int pin)
{
    return ReadGpio(pin);
}

void 
w1_setbit(int pin, int state)
{
    SetGpio(pin, state);
}


#define READ_DELAY1 5
#define READ_DELAY2 45
int
w1_read_bit(int pin)
{
    int res;

    w1_data(pin, 0);
    w1_delay_us(1, 0);
    SetGpioDirection(pin, GPIO_IN);
                                   //ORIGINAL CODE w1_delay_us(5, 1);
    //w1_delay_us(READ_DELAY1, 1);
    res = w1_getbit(pin);
                                  //ORIGINAL CODE w1_delay_us(60, 0);
    w1_delay_us(READ_DELAY2, 0);
    return res;
}


// reads a byte
static unsigned char
w1_read_byte(int pin)
{
    unsigned char res = 0;
    int mask, k;

    for (k = 0, mask=0x1; k < 8; ++k, mask <<= 1) {
        if (w1_read_bit(pin))
            res |= mask;
    }
    return res;
}

void 
w1_read_block(int pin, int bytes, w1_block_obj_t *block_obj)
{
    int i;
    for (i=0; i<bytes; ++i)
    {
        block_obj->data[i] = w1_read_byte(pin);
    }
    return;   
}   


static int low_time[2] = {55, 5};
static int high_time[2] = {5, 55};

static void
w1_write_bit(int pin, int v)
{
    v = !!v;
    w1_data(pin, 0);
    w1_delay_us(low_time[v], 1);
#if 1
    SetGpioDirection(pin, GPIO_IN);
#else
    w1_setbit(pin, 1);
#endif
    w1_delay_us(high_time[v], 1);
}


void
w1_write(int pin, unsigned char b, int power)
{
    int mask, k;

    for (k = 0, mask=0x1; k < 8; ++k, mask <<= 1)
        w1_write_bit(pin, !!(mask & b));
    if (0 == power) {
        SetGpioDirection(pin, GPIO_IN);
    }
}


// 'romp' assumed to point to an array of 8 bytes which is the
// rom_id of the device to select.
// This is not called in the code I ported from.
static void
w1_select(int pin, unsigned char * romp)
{
    int k;

    w1_write(pin, 0x55, 0);
    for (k = 0; k < 8; ++k)
        w1_write(pin, romp[k], 0);
}


static void
w1_skip(int pin)
{
    w1_write(pin, 0xcc, 0);
}

static void
w1_depower(int pin)
{
    SetGpioDirection(pin, GPIO_IN);
}


/* Should wait 250 us for bus to come high, if not return 1.
 * If so, do reset sequence and return 1 if there is a "presence"
 * pulse, else return 0. */

/* the name 'reset' could be misleading, the maxintegrated doc 
 *    refers to this as initialization, should be done as first
 *    part of every command.
 */
int
w1_reset(int pin)
{
    int res;
    int retries = 125;

    SetGpioDirection(pin, GPIO_IN);
    do {
        if (0 == retries--)
            return 0;
        w1_delay_us(2, 0);
    } while (! w1_getbit(pin));
portENTER_CRITICAL()
    w1_data(pin, 0);                // put a zero on the GPIO line
    w1_delay_us(500, 1);            //  and hold it for 500 us
    SetGpioDirection(pin, GPIO_IN); //  let line float high
    w1_delay_us(65, 1);             //  wait to read for 65 us
    res = ! w1_getbit(pin);         //  slave should drive line low, making res = 1
    w1_delay_us(490, 1);            //  wait until the end of the slot
portEXIT_CRITICAL()
    return res;
}




/*

//maximintegrated application note for search
//https://www.maximintegrated.com/en/app-notes/index.mvp/id/162 

// FIND DEVICES
void FindDevices(void)
{
    unsigned char m;
    if(!ow_reset()) //Begins when a presence is detected
    {
        if(First()) //Begins when at least one part is found
        {
            numROMs=0;
            do
            {
                numROMs++;
                for(m=0;m<8;m++)
                {
                    FoundROM[numROMs][m]=ROM[m]; //Identifies ROM
                    \\number on found device
                } 
                printf("\nROM CODE =%02X%02X%02X%02X\n",
                         FoundROM[5][7],FoundROM[5][6],FoundROM[5][5],FoundROM[5][4],
                         FoundROM[5][3],FoundROM[5][2],FoundROM[5][1],FoundROM[5][0]);
            }while (Next()&&(numROMs<10)); //Continues until no additional devices are found
        }
    }
}


// FIRST
// The First function resets the current state of a ROM search and calls
// Next to find the first device on the 1-Wire bus.
//
unsigned char First(void)
{
    lastDiscrep = 0; // reset the rom search last discrepancy global
    doneFlag = FALSE;
    return Next(); // call Next and return its return value
}



// NEXT
// The Next function searches for the next device on the 1-Wire bus. If
// there are no more devices on the 1-Wire then false is returned.
//
unsigned char Next(void)
{
    unsigned char m = 1; // ROM Bit index
    unsigned char n = 0; // ROM Byte index
    unsigned char k = 1; // bit mask
    unsigned char x = 0;
    unsigned char discrepMarker = 0; // discrepancy marker
    unsigned char g; // Output bit
    unsigned char nxt; // return value
    int flag;

    nxt = FALSE; // set the next flag to false
    dowcrc = 0; // reset the dowcrc
    flag = ow_reset(); // reset the 1-Wire
    if(flag||doneFlag) // no parts -> return false
    {
        lastDiscrep = 0; // reset the search
        return FALSE;
    }
    write_byte(0xF0); // send SearchROM command
    do
        // for all eight bytes
        {
            x = 0;
            if(read_bit()==1) x = 2;
            delay(6);
            if(read_bit()==1) x |= 1; // and its complement
            if(x ==3) // there are no devices on the 1-Wire
                break;

            else
            {
                if(x>0) // all devices coupled have 0 or 1
                    g = x>>1; // bit write value for search
                else
                {
                    // if this discrepancy is before the last
                    // discrepancy on a previous Next then pick
                    // the same as last time
                    if(m<lastDiscrep)
                        g = ((ROM[n]&k)>0);
                    else // if equal to last pick 1
                        g = (m==lastDiscrep); // if not then pick 0
                    // if 0 was picked then record
                    // position with mask k
                    if (g==0) discrepMarker = m;
                }
                if(g==1) // isolate bit in ROM[n] with mask k
                    ROM[n] |= k;
                else
                    ROM[n] &= ~k;
                write_bit(g); // ROM search write
                m++; // increment bit counter m
                k = k<<1; // and shift the bit mask k
                if(k==0) // if the mask is 0 then go to new ROM
                { // byte n and reset mask
                    ow_crc(ROM[n]); // accumulate the CRC
                    n++; k++;
                }
            }
        }while(n<8); //loop until through all ROM bytes 0-7

    if(m<65||dowcrc) // if search was unsuccessful then
        lastDiscrep=0; // reset the last discrepancy to 0
    else
    {
        // search was successful, so set lastDiscrep,
        // lastOne, nxt
        lastDiscrep = discrepMarker;
        doneFlag = (lastDiscrep==0);
        nxt = TRUE; // indicates search is not complete yet, more
        // parts remain
    }
    return nxt;
}




*/


