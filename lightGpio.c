#include "lightGpio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <poll.h>

#define _GPIO_FILE_NAME_LEN 50
#define _GPIO_FN_EXPORT "/sys/class/gpio/export"
#define _GPIO_FN_UNEXPORT "/sys/class/gpio/unexport"
#define _GPIO_DR_MAIN "/sys/class/gpio/gpio%i"
#define _GPIO_FN_DIRECTION "/sys/class/gpio/gpio%i/direction"
#define _GPIO_FN_VALUE "/sys/class/gpio/gpio%i/value"
#define _GPIO_FN_EDGE "/sys/class/gpio/gpio%i/edge"

#define _GPIO_WAIT_TIMEOUT 100000

int _gpio_referenc_counter[GPIO_MAX_NUMBER + 1] = {[0 ... GPIO_MAX_NUMBER] = 0};

int _gpio_write_number(int fd, int number) {
    char numstr[4];
    sprintf(numstr, "%i", number);
    int len = (number < 100) ? 2 : 3;
    int ret = write(fd, numstr, len);
    return (ret < 0) ? GPIO_ERROR_FAILED_TO_WRITE_TO_GPIO_FILE : GPIO_NO_ERROR;
}

void _gpio_timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
}

/**
 *  It seems that it takes a while for udev to set correct access rights to newly created gpioN folder.
 *  This method will wait for the folder to be accessable.
 */
int _gpio_wait_for_udev(int number) {
    char dirName[_GPIO_FILE_NAME_LEN];
    sprintf(dirName, _GPIO_FN_VALUE, number);
    struct timespec tstart={0,0}, tnow={0,0}, twait={0, 100}, tdiff;
    clock_gettime(CLOCK_MONOTONIC, &tstart);
    do {
        int res = access(dirName, W_OK);
        if (res == 0) {
            return GPIO_NO_ERROR;
        }
        nanosleep(&twait, NULL);
        clock_gettime(CLOCK_MONOTONIC, &tnow);
        _gpio_timespec_diff(&tstart, &tnow, &tdiff);
    } while ((tdiff.tv_sec > 0) || (tdiff.tv_nsec > _GPIO_WAIT_TIMEOUT));
    return GPIO_ERROR_WAITING_FOR_GPION_TIMEDOUT;
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
    return _gpio_wait_for_udev(number);
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

int _gpio_set_edge(int number, int edge) {
    char fileName[_GPIO_FILE_NAME_LEN];
    sprintf(fileName, _GPIO_FN_EDGE, number);
    int fd = open(fileName, O_WRONLY);
    if (fd < 0) {
        return GPIO_ERROR_CANNOT_OPEN_GPIO_FILE;
    }
    int err;
    switch (edge) {
        case GPIO_EDGE_RISING:
            err = write(fd, "rising", 6);
            break;
        case GPIO_EDGE_FALLING:
            err = write(fd, "falling", 7);
            break;
        case GPIO_EDGE_BOTH:
            err = write(fd, "both", 4);
            break;
        case GPIO_EDGE_NONE:
            err = write(fd, "none", 4);
            break;
        default:
            err = GPIO_ERROR_UNEXPECTED_VALUE;
    }
    close(fd);
    return err;
}

int _gpio_ensure_value_file_opened(Gpio * gpioStruct) {
    if (gpioStruct->valueFd < 0) {
        char fileName[_GPIO_FILE_NAME_LEN];
        sprintf(fileName, _GPIO_FN_VALUE, gpioStruct->number);
        int fd = open(fileName, O_RDONLY);
        if (fd < 0) {
            return GPIO_ERROR_CANNOT_OPEN_GPIO_FILE;
        }
        gpioStruct->valueFd = fd;
    }
    return GPIO_NO_ERROR;
}

int gpio_open(Gpio * gpioStruct) {
    int number = gpioStruct->number,
        direction = gpioStruct->direction,
        edge = gpioStruct->edge;
    if (number < 0 || number > GPIO_MAX_NUMBER) {
        return GPIO_ERROR_NUMBER_OUT_OF_RANGE;
    }
    if (direction != GPIO_DIRECTION_IN || direction != GPIO_DIRECTION_OUT) {
        return GPIO_ERROR_WRONG_DIRECTION;
    }
    switch (edge) {
        case GPIO_EDGE_RISING:
        case GPIO_EDGE_FALLING:
        case GPIO_EDGE_BOTH:
            if (direction == GPIO_DIRECTION_OUT) {
                return GPIO_ERROR_UNSUPPORTED_COMBINATION;
            }
        case GPIO_EDGE_NONE:
            break;
        default:
            return GPIO_ERROR_UNEXPECTED_VALUE;
    }
    gpioStruct->valueFd = -1;
    _gpio_referenc_counter[number]++;

    int hasDir = _gpio_has_dir(number);
    int err;
    if (hasDir <= 0) {
        err = _gpio_export(number);
        if (err < 0) {
            return err;
        }
    }
    err = _gpio_set_direction(number, direction);
    if (err < 0) {
        return err;
    }
    return _gpio_set_edge(number, edge);
}

int gpio_close(Gpio * gpioStruct) {
    _gpio_referenc_counter[gpioStruct->number]--;
    if (gpioStruct->valueFd >= 0) {
        close(gpioStruct->valueFd);
    }
    if (_gpio_referenc_counter[gpioStruct->number] == 0) {
        return _gpio_unexport(gpioStruct->number);
    }
    return GPIO_NO_ERROR;
}

int gpio_get_value(Gpio * gpioStruct) {
    if (gpioStruct->direction != GPIO_DIRECTION_IN) {
        return GPIO_ERROR_WRONG_DIRECTION;
    }
    int err = _gpio_ensure_value_file_opened(gpioStruct);
    if (err < 0) {
        return err;
    }
    char ch;
    err = read(gpioStruct->valueFd, &ch, 1);
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

int gpio_set_value(Gpio * gpioStruct, int value) {
    if (gpioStruct->direction != GPIO_DIRECTION_OUT) {
        return GPIO_ERROR_WRONG_DIRECTION;
    }
    int err = _gpio_ensure_value_file_opened(gpioStruct);
    if (err < 0) {
        return err;
    }
    char ch = (value == GPIO_VALUE_HIGH) ? '1' : '0';
    err = write(gpioStruct->valueFd, &ch, 1);
    return (err < 0) ? err : GPIO_NO_ERROR;
}

int gpio_wait_for_edge(Gpio * gpioStruct, int timeout) {
    int edge = gpioStruct->edge;
    if (edge != GPIO_EDGE_RISING && edge != GPIO_EDGE_FALLING && edge != GPIO_EDGE_BOTH) {
        return GPIO_ERROR_UNEXPECTED_VALUE;
    }
    int err = _gpio_ensure_value_file_opened(gpioStruct);
    if (err < 0) {
        return err;
    }
    struct pollfd pfd;
    pfd.fd = gpioStruct->valueFd;
    pfd.events = POLLPRI | POLLERR;
    
    //Make a dummy read, otherwise poll will return imidietly if the file is freshly opened.
    char ch;
    read(pfd.fd, &ch, 1);

    err = poll(&pfd, 1, timeout);
    if (err == 0) {
        return GPIO_WAIT_TIMEDOUT;
    } else if (err < 0) {
        return GPIO_WAIT_RETURNED_WITH_ERROR;
    } else if (err == 1) {
        return GPIO_WAIT_RETURNED;
    }
    return GPIO_ERROR_UNEXPECTED_VALUE;
}