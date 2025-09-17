/*
 * asm_functions.h
 *
 *  Created on: Nov 3, 2024
 *      Author: Daniel
 */

#ifndef DEPENDENCIES_ASM_FUNCTIONS_H_
#define DEPENDENCIES_ASM_FUNCTIONS_H_
#include <stdint.h>

extern void setPsp(void *);
extern uint8_t getSvc();
extern void * pushContext();
extern void setAsp();
extern void * popContext();
extern void * popStart();
extern uint8_t serviceNumberFetch();
extern uint32_t * getMsp();
extern uint32_t * getPsp();
extern void setUnPriv();
extern void setPriv();

#endif /* DEPENDENCIES_ASM_FUNCTIONS_H_ */
