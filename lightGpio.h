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

#define GPIO_MAX_NUMBER 128

typedef struct Gpio {
    int number;
    int direction;
} Gpio;

int Gpio_open(Gpio * gpioStruct, int number, int direction);
int Gpio_close(Gpio * gpioStruct);
int Gpio_get_value(Gpio * gpioStruct);
int Gpio_set_value(Gpio * gpioStruct, int value);

#endif