#ifndef _GPIO_H_
#define _GPIO_H_

/* GPIO event detect types */
enum DETECT_TYPE {
	DETECT_NONE,
	DETECT_RISING,
	DETECT_FALLING,
	DETECT_HIGH,
	DETECT_LOW,
	DETECT_RISING_ASYNC,
	DETECT_FALLING_ASYNC
};

/* GPIO pull up or down states */
/* When you set pull-up or pull-down 
 * on a pin, you write to the GPPUD
 * register bits 1:0 regardless of
 * which pin it is.  You then use the 
 * clocking register to determine the
 * specific pin you want to changed.
*/
#define PULL_DOWN_ENABLE 1
#define PULL_UP_ENABLE 2
enum PULL_STATE {
	PULL_DISABLE,
	PULL_UP,
	PULL_DOWN,
	PULL_RESERVED
};

/* Pin data direction */
enum GPIO_DIR {
	GPIO_IN,
	GPIO_OUT
};

/* GPIO pin setup */
void SetGpioFunction	(unsigned int pinNum, unsigned int funcNum);
/* A simple wrapper around SetGpioFunction */
void SetGpioDirection	(unsigned int pinNum, enum GPIO_DIR dir);

/* Set GPIO output level */
void SetGpio			(unsigned int pinNum, unsigned int pinVal);

/* Read GPIO pin level */
int ReadGpio			(unsigned int pinNum);

/* GPIO pull up/down resistor control function (NOT YET IMPLEMENTED) */
int PudGpio				(unsigned int pinNum, enum PULL_STATE state);

/* Interrupt related functions */
void EnableGpioDetect	(unsigned int pinNum, enum DETECT_TYPE type);
void DisableGpioDetect	(unsigned int pinNum, enum DETECT_TYPE type);
void ClearGpioInterrupt	(unsigned int pinNum);

#endif
