#ifndef SENSOR_MQ2_H
#define SENSOR_MQ2_H

// MQ-2 ADC Interface: ADC0
#define MQ2_ADC_CHANNEL 0

int MQ2_Init(void);
int MQ2_ReadSmoke(float *concentration);

#endif // SENSOR_MQ2_H
