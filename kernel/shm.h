/*
 * shm.h
 *
 *  Created on: Sep 22, 2025
 *      Author: clara
 */

#ifndef KERNEL_SHM_H_
#define KERNEL_SHM_H_


typedef struct _shm
{
    int shared;
    int temperature;
    int turbidity;
    int presses;
    int presses2;
    int feedingAmounts;
}shm;


#endif /* KERNEL_SHM_H_ */
