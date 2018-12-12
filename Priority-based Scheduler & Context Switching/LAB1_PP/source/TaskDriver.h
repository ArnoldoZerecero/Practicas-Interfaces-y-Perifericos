/*
 * TaskDriver.h
 *
 *  Created on: 24/09/2018
 *      Author: Jesús Arnoldo Zerecero Núñez
 */

#ifndef TASKDRIVER_H_
#define TASKDRIVER_H_

#include <stdint.h>

#define MAX_TASKS 32
#define STACK_SIZE 256
#define NULL ((void *)0)

typedef struct
{
	int8_t * bTaskID;
	uint8_t bTaskNumber;
	uint8_t bTaskPriority;
	void * pvParameters;
	uint32_t dwaStackFrame[STACK_SIZE/4];
	void *pvTaskAddress;
}Task_Control_Descriptor_t;

extern Task_Control_Descriptor_t saTCD[MAX_TASKS];

extern uint8_t gbaReadyQueue[MAX_TASKS]; /*Un espacio con MAX_TASKS representa un espacio vacío*/

uint8_t flgfnTaskCreate(void * const kpvTaskFunction, const char * const kbTaskID, const uint16_t kwStackSizeBytes, void * const kpvParameters, const uint8_t kbTaskPriority);

uint8_t flgfnTaskCreateBlocked(void * const kpvTaskFunction, const char * const kbTaskID, const uint16_t kwStackSizeBytes, void * const kpvParameters, const uint8_t kbTaskPriority);

void vfnTaskReady(const uint8_t kbTaskNumber);

void vfnTaskYield(void);

void vfnTaskRegistersInitialization(uint32_t dwaStackFrame[], void * const kpvTaskFunction);

void vfnReadyQueue(const uint8_t kbTaskNumber, const uint8_t kfInOrOut);

#endif /* TASKDRIVER_H_ */
