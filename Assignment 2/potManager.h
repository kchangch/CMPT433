// Module to control input from the potentiometer
// Approximates the reading using a PWL function
// to smooth out the noise
#ifndef _POT_MANAGER_H
#define _POT_MANAGER_H

#define A2D_FILE_VOLTAGE0   "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define A2D_VOLTAGE_REF_V   1.8
#define A2D_MAX_READING     4095

int Pot_getVoltage0Reading();
int Pot_getApproxReading();

#endif