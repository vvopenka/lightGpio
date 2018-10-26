#include "lightGpio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define _GPIO_FILE_NAME_LEN 50
#define _GPIO_FN_EXPORT "/sys/class/gpio/export"
#define _GPIO_FN_UNEXPORT "/sys/class/gpio/unexport"
#define _GPIO_DR_MAIN "/sys/class/gpio/gpio%i"
#define _GPIO_FN_DIRECTION "/sys/class/gpio/gpio%i/direction"
#define _GPIO_FN_VALUE "/sys/class/gpio/gpio%i/value"

int _gpio_referenc_counter[GPIO_MAX_NUMBER + 1] = {[0 ... GPIO_MAX_NUMBER] = 0};

int _gpio_write_number(int fd, int number) {
    char numstr[4];
    sprintf(numstr, "%i", number);
    int len = (number < 100) ? 2 : 3;
    int ret = write(fd, numstr, len);
    return (ret < 0) ? GPIO_ERROR_FAILED_TO_WRITE_TO_GPIO_FILE : GPIO_NO_ERROR;
}

int _gpio_export(int number) {
    int fd = open(_GPIO_FN_EXPORT, O_WRONLY);
    if (fd < 0) {
        return GPIO_ERROR_CANNOT_OPEN_GPIO_FILE;
    }
    if (_gpio_write_number(fd, number) < 0) {
        return GPIO_ERROR_FAILED_TO_WRITE_TO_GPIO_FILE;
    }
    close(fd);
    return GPIO_NO_ERROR;
}

int _gpio_unexport(int number) {
    int fd = open(_GPIO_FN_UNEXPORT, O_WRONLY);
    if (fd < 0) {
        return GPIO_ERROR_CANNOT_OPEN_GPIO_FILE;
    }
    if (_gpio_write_number(fd, number) < 0) {
        return GPIO_ERROR_FAILED_TO_WRITE_TO_GPIO_FILE;
    }
    close(fd);
    return GPIO_NO_ERROR;
}

int _gpio_has_dir(int number) {
    char dirName[_GPIO_FILE_NAME_LEN];
    sprintf(dirName, _GPIO_DR_MAIN, number);
    struct stat dir_stat;
    if (stat(dirName, &dir_stat) < 0) {
        return GPIO_ERROR_CANNOT_OPEN_GPIO_FILE;
    }
    return S_ISDIR(dir_stat.st_mode) ? 1 : 0;
}

int _gpio_set_direction(int number, int direction) {
    char fileName[_GPIO_FILE_NAME_LEN];
    sprintf(fileName, _GPIO_FN_DIRECTION, number);
    int fd = open(fileName, O_WRONLY);
    if (fd < 0) {
        return GPIO_ERROR_CANNOT_OPEN_GPIO_FILE;
    }
    int err;
    if (direction == GPIO_DIRECTION_IN) {
        err = write(fd, "in", 2);
    } else {
        err = write(fd, "out", 3);
    }
    close(fd);
    return (err < 0) ? GPIO_ERROR_FAILED_TO_WRITE_TO_GPIO_FILE : GPIO_NO_ERROR;
}

int Gpio_open(Gpio * gpioStruct, int number, int direction) {
    if (number < 0 || number > GPIO_MAX_NUMBER) {
        return GPIO_ERROR_NUMBER_OUT_OF_RANGE;
    }
    gpioStruct->number = number;
    gpioStruct->direction = direction;
    _gpio_referenc_counter[number]++;

    int hasDir = _gpio_has_dir(number);
    if (hasDir < 0) {
        return GPIO_ERROR_CANNOT_OPEN_GPIO_FILE;
    } else if (hasDir == 0) {
        int err = _gpio_export(number);
        if (err < 0) {
            return err;
        }
    }
    return _gpio_set_direction(number, direction);
}

int Gpio_close(Gpio * gpioStruct) {
    _gpio_referenc_counter[gpioStruct->number]--;
    if (_gpio_referenc_counter[gpioStruct->number] == 0) {
        return _gpio_unexport(gpioStruct->number);
    }
    return GPIO_NO_ERROR;
}

int Gpio_get_value(Gpio * gpioStruct) {
    if (gpioStruct->direction != GPIO_DIRECTION_IN) {
        return GPIO_ERROR_WRONG_DIRECTION;
    }
    char fileName[_GPIO_FILE_NAME_LEN];
    sprintf(fileName, _GPIO_FN_VALUE, gpioStruct->number);
    int fd = open(fileName, O_RDONLY);
    if (fd < 0) {
        return GPIO_ERROR_CANNOT_OPEN_GPIO_FILE;
    }
    char ch;
    int err = read(fd, &ch, 1);
    close(fd);
    if (err < 0) {
        return err;
    }
    switch(ch) {
        case '0':
            return GPIO_VALUE_LOW;
        case '1':
            return GPIO_VALUE_HIGH;
        default:
            return GPIO_ERROR_UNEXPECTED_VALUE;
    }
}

int Gpio_set_value(Gpio * gpioStruct, int value) {
    if (gpioStruct->direction != GPIO_DIRECTION_OUT) {
        return GPIO_ERROR_WRONG_DIRECTION;
    }
    char fileName[_GPIO_FILE_NAME_LEN];
    sprintf(fileName, _GPIO_FN_VALUE, gpioStruct->number);
    int fd = open(fileName, O_RDONLY);
    if (fd < 0) {
        return GPIO_ERROR_CANNOT_OPEN_GPIO_FILE;
    }
    char ch = (value == GPIO_VALUE_HIGH) ? '1' : '0';
    int err = write(fd, &ch, 1);
    close(fd);
    return (err < 0) ? err : GPIO_NO_ERROR;
}