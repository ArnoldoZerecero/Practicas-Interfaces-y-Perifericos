/*
 * SchedFunc.h
 *
 *  Created on: 01/10/2018
 *      Author: Edwin Francisco Zamora Oros
 */

#ifndef SCHEDFUNC_H_
#define SCHEDFUNC_H_

#include <stdint.h>

#define IDLE_PRIORITY 255

extern void * gpvNextTaskAddress;
extern void * gpvCurrentStackAddress;
extern void * gpvNextStackAddress;
extern uint8_t gbNextTask;

void vfnTaskStartSched(void);

uint8_t flgfnSchedPolicy(uint8_t bNumberOfTasks);

uint8_t bfnSched(void);

void vfnPrepareNextContext(void);

void vfnIdleTask(void *pParameters);

#endif /* SCHEDFUNC_H_ */
