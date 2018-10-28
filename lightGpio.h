#ifndef __lightGpio_h
#define __lightGpio_h

#define GPIO_DIRECTION_IN 0
#define GPIO_DIRECTION_OUT 1

#define GPIO_VALUE_LOW 0
#define GPIO_VALUE_HIGH 1

#define GPIO_EDGE_NONE 0
#define GPIO_EDGE_RISING 1
#define GPIO_EDGE_FALLING 2
#define GPIO_EDGE_BOTH 3

#define GPIO_NO_ERROR 0
#define GPIO_ERROR_NUMBER_OUT_OF_RANGE -1
#define GPIO_ERROR_CANNOT_OPEN_GPIO_FILE -2
#define GPIO_ERROR_FAILED_TO_WRITE_TO_GPIO_FILE -3
#define GPIO_ERROR_WRONG_DIRECTION -4
#define GPIO_ERROR_UNEXPECTED_VALUE -5
#define GPIO_ERROR_WAITING_FOR_GPION_TIMEDOUT -6
#define GPIO_ERROR_UNSUPPORTED_COMBINATION -7

#define GPIO_MAX_NUMBER 128
#define GPIO_NO_TIMEOUT -1
#define GPIO_WAIT_TIMEDOUT 0
#define GPIO_WAIT_RETURNED 1
#define GPIO_WAIT_RETURNED_WITH_ERROR -1

typedef struct Gpio {
    int number;
    int direction;
    int edge;
    int valueFd;
} Gpio;

int gpio_open(Gpio * gpioStruct);
int gpio_close(Gpio * gpioStruct);
int gpio_get_value(Gpio * gpioStruct);
int gpio_set_value(Gpio * gpioStruct, int value);
int gpio_wait_for_edge(Gpio * gpioStruct, int timeout);

#endif