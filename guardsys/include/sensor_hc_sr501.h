#ifndef SENSOR_HC_SR501_H
#define SENSOR_HC_SR501_H

// HC-SR501 GPIO Interface: GPIO_A0
#define HC_SR501_GPIO_PIN "PA0"

int HC_SR501_Init(void);
int HC_SR501_ReadStatus(int *isTriggered);

#endif // SENSOR_HC_SR501_H
