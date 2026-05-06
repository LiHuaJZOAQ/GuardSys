#ifndef ALARM_CONTROL_H
#define ALARM_CONTROL_H

// KY-012 Buzzer GPIO Interface: GPIO_A1
#define BUZZER_GPIO_PIN "PA1"

// KY-016 RGB LED GPIO/PWM Interface: GPIO_A2, GPIO_A3, GPIO_A4
#define LED_R_PIN "PA2"
#define LED_G_PIN "PA3"
#define LED_B_PIN "PA4"

int Alarm_Init(void);
int Alarm_SetStatus(int status); // 0: Normal(Green), 1: Warning(Yellow), 2: Alarm(Red+Buzzer)

#endif // ALARM_CONTROL_H
