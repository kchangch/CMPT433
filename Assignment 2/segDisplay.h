// Module to control the 14-seg display on the BBG
#ifndef _SEG_DISPLAY_H
#define _SEG_DISPLAY_H

static int Seg_Display_initI2cBus(char* bus, int address);
static void Seg_Display_writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value);
static void Seg_Display_setDigitDisplay(int gpioNum, int state);
void Seg_Display_write2File(const char*type, char* value);

#endif
