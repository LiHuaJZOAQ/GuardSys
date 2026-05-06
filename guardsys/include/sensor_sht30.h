#ifndef SENSOR_SHT30_H
#define SENSOR_SHT30_H

// SHT30 I2C Interface: I2C4 (SDA, SCL)
#define SHT30_I2C_BUS 4
#define SHT30_ADDR 0x44

int SHT30_Init(void);
int SHT30_ReadData(float *temp, float *hum);

#endif // SENSOR_SHT30_H
