#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "potManager.h"

// Function to get voltage reading from BBG
int Pot_getVoltage0Reading() {
    FILE *f = fopen(A2D_FILE_VOLTAGE0, "r");
    if (!f) {
        printf("ERROR: Unable to open voltage input file.  Cape loaded?\n");
        printf("\tCheck /boot/uEnv.txt for correct options.\n");
        exit(-1);
    }

    int a2dReading = 0;
    int itemsRead = fscanf(f, "%d", &a2dReading);
    if (itemsRead <= 0) {
        printf("ERROR: Unable to read values from voltage input file.\n");
        exit(-1);
    }
    
    fclose(f);

    return a2dReading;
}

// Function to approximate reading from BBG
int Pot_getApproxReading() {
    int voltageToArraySizeMapping[10][2] = {
        {0, 1},
        {500, 20},
        {1000, 60},
        {1500, 120},
        {2000, 250},
        {2500, 300},
        {3000, 500},
        {3500, 800},
        {4000, 1200},
        {4100, 2100}
    };

    // query the knob for the current analog voltage
    int reading = Pot_getVoltage0Reading();

    int currDiff = 0;
    double approxReading = 0;

    // find where the reading fits in on the x axis
    for (int i = 1; i < 10; i++) {
        currDiff = reading - voltageToArraySizeMapping[i][0];
        // when the difference is negative, reading fits between i-1 and i
        if (currDiff < 0) {
            int y_1 = voltageToArraySizeMapping[i-1][1];
            int y_2 = voltageToArraySizeMapping[i][1];
            int dy = y_2 - y_1;
            int x_1 = voltageToArraySizeMapping[i-1][0];
            int x_2 = voltageToArraySizeMapping[i][0];
            int dx = x_2 - x_1;
            
            double m = (double)dy / (double)(dx);
            double b = y_2 - (m * x_2);

            approxReading = (m * reading) + b;
            // printf("approximated: %3f\n", approxReading);
            break;
        }
    }
    return approxReading;
}