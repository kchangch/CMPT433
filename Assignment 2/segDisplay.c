#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "segDisplay.h"

#define EXPORT "/sys/class/gpio/export"
#define LEFT_DIRECTION "/sys/class/gpio/gpio61/direction"
#define RIGHT_DIRECTION "/sys/class/gpio/gpio44/direction"
#define LEFT_VALUE "/sys/class/gpio/gpio61/value"
#define RIGHT_VALUE "/sys/class/gpio/gpio44/value"
#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"
#define I2C_DEVICE_ADDRESS 0x20
#define REG_DIRA 0x00
#define REG_DIRB 0x01
#define REG_OUTA 0x14
#define REG_OUTB 0x15

static char upper_bits[10][5] = {"0x86", "0x02", "0x0e", "0x0e", "0x8a", "0x8c", "0x8d", "0x14", "0x8e", "0x8e"};
static char lower_bits[10][5] = {"0xa1", "0x80", "0x31", "0xb0", "0x90", "0xb0", "0xb1", "0x04", "0xb1", "0xb0"};

static int Seg_Display_initI2cBus(char* bus, int address) {
    int i2cFileDesc = open(bus, O_RDWR);
    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    
    if (result < 0) {
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }

    return i2cFileDesc;
}

// Write to file function
void Seg_Display_write2File(const char* type, char* value ) {
    FILE* _file = fopen(type, "w");
    if (_file == NULL) {
        printf("Unable to open file (%s) for write\n", type);
    }
    int charWritten = fprintf(_file, "%s", value);
    if (charWritten <= 0) {
        printf("Error writing in file\n");
        exit(1);
    }
    fclose(_file);
}

void Seg_Display_writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value) {
    unsigned char buff[2];
    buff[0] = regAddr;
    buff[1] = value;
    int res = write(i2cFileDesc, buff, 2);
    if (res != 2) {
        perror("I2C: Unable to write i2c register.");
        exit(1);
    }
}

// turns the right digit on or off
static void Seg_Display_setDigitDisplay(int gpioNum, int state) {
    char leftDigit[48] = "/sys/class/gpio/gpio61/value";
    char rightDigit[48] = "/sys/class/gpio/gpio44/value";
    char stateStr[2];
	sprintf(stateStr, "%d", state);

    if (gpioNum == 44) {
        FILE *f = fopen(leftDigit, "w");
        if (f == NULL) {
            printf("ERROR: Unable to open %s.\n", leftDigit);
            exit(-1);
        }
        fprintf(f, "%s", stateStr);
        fclose(f);
    }

    if (gpioNum == 61) {
        FILE *f = fopen(rightDigit, "w");
        if (f == NULL) {
            printf("ERROR: Unable to open %s.\n", leftDigit);
            exit(-1);
        }
        fprintf(f, "%s", stateStr);
        fclose(f);
    }
}

// 0x14 0xa1, 0x15 0x86 = 0
// 0x14 0x80, 0x15 0x02 = 1
// 0x14 0x31, 0x15 0x0e = 2
// 0x14 0xb0, 0x15 0x0e = 3
// 0x14 0x90, 0x15 0x8a = 4
// 0x14 0xb0, 0x15 0x8c = 5
// 0x14 0xb1, 0x15 0x8d = 6
// 0x14 0x04, 0x15 0x14 = 7
// 0x14 0xb1, 0x15 0x8e = 8
// 0x14 0xb0, 0x15 0x8e = 9
static void Seg_Display_displayNumber(int i2cFileDesc, int number) {
    // compute digits
    int leftDigit = number / 10;
    int rightDigit = number % 10;
    
    // reset
    Seg_Display_setDigitDisplay(61, 0);
    Seg_Display_setDigitDisplay(44, 0);

    // write to right
    Seg_Display_writeI2cReg(i2cFileDesc, REG_OUTA, (int)strtol(lower_bits[rightDigit], NULL, 16));
    Seg_Display_writeI2cReg(i2cFileDesc, REG_OUTB, (int)strtol(upper_bits[rightDigit], NULL, 16));

    // turn on right
    Seg_Display_setDigitDisplay(61, 1);
    Seg_Display_setDigitDisplay(44, 0);
    nanosleep((const struct timespec[]){{0, 10000000L}}, NULL);

    // reset
    Seg_Display_setDigitDisplay(61, 0);
    Seg_Display_setDigitDisplay(44, 0);

    // write to left
    Seg_Display_writeI2cReg(i2cFileDesc, REG_OUTA, (int)strtol(lower_bits[leftDigit], NULL, 16));
    Seg_Display_writeI2cReg(i2cFileDesc, REG_OUTB, (int)strtol(upper_bits[leftDigit], NULL, 16));

    // turn on left
    Seg_Display_setDigitDisplay(61, 0);
    Seg_Display_setDigitDisplay(44, 1);
    nanosleep((const struct timespec[]){{0, 10000000L}}, NULL);
}

// Initialize GPIOs
int Seg_Display_configI2C() {

    int i2cFileDesc = Seg_Display_initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);

    system("config-pin P9_18 i2c");
    system("config-pin P9_17 i2c");

    Seg_Display_write2File(EXPORT, "61");

    struct timespec seconds, nanoseconds;
    memset(&seconds, 0, sizeof(struct timespec));
    memset(&nanoseconds, 0, sizeof(struct timespec));

    seconds.tv_sec = 0;
    nanoseconds.tv_nsec = 40000000;

    nanosleep(&seconds, &nanoseconds);

    Seg_Display_write2File(EXPORT, "44");
    nanosleep(&seconds, &nanoseconds);

    Seg_Display_write2File(LEFT_DIRECTION, "out");
    Seg_Display_write2File(RIGHT_DIRECTION, "out");

    Seg_Display_write2File(LEFT_VALUE, "1");
    Seg_Display_write2File(RIGHT_VALUE, "1");

    Seg_Display_writeI2cReg(i2cFileDesc, REG_DIRA, 0x00);
    Seg_Display_writeI2cReg(i2cFileDesc, REG_DIRB, 0x00);

    system("i2cset -y 1 0x20 0x00 0x00");
    system("i2cset -y 1 0x20 0x01 0x00");

    return i2cFileDesc;
}