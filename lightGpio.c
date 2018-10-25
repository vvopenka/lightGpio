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
        return _gpio_export(number);
    }
    return GPIO_NO_ERROR;
}

int Gpio_close(Gpio * gpioStruct) {
    _gpio_referenc_counter[gpioStruct->number]--;
    if (_gpio_referenc_counter[gpioStruct->number] == 0) {
        return _gpio_unexport(gpioStruct->number);
    }
    return GPIO_NO_ERROR;
}