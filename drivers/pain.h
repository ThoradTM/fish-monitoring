/*
 * pain.h
 *
 *  Created on: Mar 24, 2024
 *      Author: clara
 */


#ifndef PAIN_H_
#define PAIN_H_
#include <stdint.h>

#define GREEN_LED_MASK 8

#define GREEN_OB_LED     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))

void initPain();

#endif


