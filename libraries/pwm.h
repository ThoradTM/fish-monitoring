/*
 * pwm.h
 *
 *  Created on: Oct 7, 2025
 *      Author: clara
 */

#ifndef LIBRARIES_PWM_H_
#define LIBRARIES_PWM_H_

#include <stdint.h>

#define sf 6
#define MAX_MOTOR_SPEED (1023 - sf)

extern int32_t desired_pwm;

void initPwm();

void servoSlow();


void servoStop();


#endif /* LIBRARIES_PWM_H_ */
