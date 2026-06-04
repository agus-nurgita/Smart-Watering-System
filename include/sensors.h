#ifndef SENSORS_H
#define SENSORS_H

void readSensor();
void rainStabilityFilter();

int getMedian(int *arr, int size);

#endif