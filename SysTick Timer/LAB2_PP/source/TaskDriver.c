/*
 * TaskDriver.c
 *
 *  Created on: 24/09/2018
 *      Author: Jesús Arnoldo Zerecero Núñez
 */

#include <stdint.h>
#include "TaskDriver.h"
#include "Semaphores.h"
#include "fsl_debug_console.h"
#include "SchedFunc.h"

Task_Control_Descriptor_t saTCD[MAX_TASKS];

uint8_t gbaReadyQueue[MAX_TASKS] = {MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,
		MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,
		MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS,MAX_TASKS}; /*Un espacio con MAX_TASKS representa un espacio vacío*/

uint8_t gbRemainingTasks = MAX_TASKS;

volatile uint32_t gdwaTaskBlockedSysTick[MAX_TASKS] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

uint8_t flgfnTaskCreate(void * const kpvTaskFunction, const char * const kbTaskID, const uint16_t kwStackSizeBytes, void * const kpvParameters, const uint8_t kbTaskPriority)
{
	uint8_t flgCreateStatus = 1;

	if(!(gbRemainingTasks - 1)) /*-1 debido a que son 32 tasks, de 0 a 31*/
	{
#if DEBUG_MODE
		PRINTF("No space for a new task!.\r\n");
#endif
		flgCreateStatus = 0; /*Error, no hay espacio para un nuevo task*/
	}

	if(kpvTaskFunction == NULL)
	{
#if DEBUG_MODE
		PRINTF("Task not found!.\r\n");
#endif
		flgCreateStatus = 0; /*No se encuentra la función para ese task*/
	}

	gbRemainingTasks--;
	if(!flgfnSchedPolicy(MAX_TASKS - gbRemainingTasks))
	{
#if DEBUG_MODE
		PRINTF("A task can't have the same priority than another task!.\r\n");
#endif
		gbRemainingTasks++;
		flgCreateStatus = 0; /*Error, el task tiene prioridad repetida*/
	}

	if(flgCreateStatus) /*Si la función fue creada con éxito*/
	{
		/*Se llena TCD*/
		saTCD[MAX_TASKS - (gbRemainingTasks + 1)].bTaskNumber = MAX_TASKS - (gbRemainingTasks + 1);
		saTCD[MAX_TASKS - (gbRemainingTasks + 1)].pvTaskAddress = kpvTaskFunction;
		saTCD[MAX_TASKS - (gbRemainingTasks + 1)].pvParameters = kpvParameters;
		saTCD[MAX_TASKS - (gbRemainingTasks + 1)].bTaskID =  (int8_t *) kbTaskID;
		saTCD[MAX_TASKS - (gbRemainingTasks + 1)].bTaskPriority =  kbTaskPriority;

		vfnReadyQueue(MAX_TASKS - (gbRemainingTasks + 1), 1); /*Se asigna como ready*/

		vfnTaskRegistersInitialization(&saTCD[MAX_TASKS - (gbRemainingTasks + 1)].dwaStackFrame[0], kpvTaskFunction);
	}

	return flgCreateStatus;
}

uint8_t flgfnTaskCreateBlocked(void * const kpvTaskFunction, const char * const kbTaskID, const uint16_t kwStackSizeBytes, void * const kpvParameters, const uint8_t kbTaskPriority)
{
	uint8_t flgCreateStatus = 1;

	if(!(gbRemainingTasks - 1)) /*-1 debido a que son 32 tasks, de 0 a 31*/
	{
#if DEBUG_MODE
		PRINTF("No space for a new task!.\r\n");
#endif
		flgCreateStatus = 0; /*Error, no hay espacio para un nuevo task*/
	}

	if(kpvTaskFunction == NULL)
	{
#if DEBUG_MODE
		PRINTF("Task not found!.\r\n");
#endif
		flgCreateStatus = 0; /*No se encuentra la función para ese task*/
	}

	gbRemainingTasks--;
	if(!flgfnSchedPolicy(MAX_TASKS - gbRemainingTasks))
	{
#if DEBUG_MODE
		PRINTF("A task can't have the same priority than another task!.\r\n");
#endif
		gbRemainingTasks++;
		flgCreateStatus = 0; /*Error, el task tiene prioridad repetida*/
	}

	if(flgCreateStatus) /*Si la función fue creada con éxito*/
	{
		/*Se llena TCD*/
		saTCD[MAX_TASKS - (gbRemainingTasks + 1)].bTaskNumber = MAX_TASKS - (gbRemainingTasks + 1);
		saTCD[MAX_TASKS - (gbRemainingTasks + 1)].pvTaskAddress = kpvTaskFunction;
		saTCD[MAX_TASKS - (gbRemainingTasks + 1)].pvParameters = kpvParameters;
		saTCD[MAX_TASKS - (gbRemainingTasks + 1)].bTaskID =  (int8_t *) kbTaskID;
		saTCD[MAX_TASKS - (gbRemainingTasks + 1)].bTaskPriority =  kbTaskPriority;

		vfnTaskRegistersInitialization(&saTCD[MAX_TASKS - (gbRemainingTasks + 1)].dwaStackFrame[0], kpvTaskFunction);
	}

	return flgCreateStatus;
}

void vfnTaskReady(const uint8_t kbTaskNumber)
{
	if(kbTaskNumber >= MAX_TASKS)
	{
#if DEBUG_MODE
		PRINTF("Invalid task number, can't set to ready!.\r\n");
#endif
	}
	else
	{
		vfnReadyQueue(kbTaskNumber, 1);
	}
}

void vfnTaskDelayBlocked(const uint8_t kbTaskNumber, const uint32_t kdwTicks)
{
	if(kbTaskNumber >= MAX_TASKS)
	{
#if DEBUG_MODE
		PRINTF("Invalid task number, can't set to blocked!.\r\n");
#endif
	}
	else
	{
		vfnReadyQueue(kbTaskNumber, 0);
		gdwaTaskBlockedSysTick[kbTaskNumber] = kdwTicks;
		vfnTaskYield();
	}
}
void vfnTaskYield(void)
{
	gpvCurrentStackAddress = (uint32_t *) &saTCD[gbNextTask].dwaStackFrame; /*Se guarda la dirección del stack en uso para guardar contexto*/
	vfnPrepareNextContext();
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; /*Se forza una interrupción*/
}

void vfnTaskRegistersInitialization(uint32_t dwaStackFrame[], void * const kpvTaskFunction)
{
	dwaStackFrame[0] = 0x01000000; /*xPSR*/
	dwaStackFrame[1] = (uint32_t) kpvTaskFunction; /*PC*/
	dwaStackFrame[2] = (uint32_t) kpvTaskFunction; /*LR*/
	dwaStackFrame[3] = 0xBABA; /*R0*/
	dwaStackFrame[4] = 0xBABA; /*R1*/
	dwaStackFrame[5] = 0xBABA; /*R2*/
	dwaStackFrame[6] = 0xBABA; /*R3*/
	dwaStackFrame[7] = 0xBABA; /*R12*/
	dwaStackFrame[8] = 0xBABA; /*R4*/
	dwaStackFrame[9] = 0xBABA; /*R5*/
	dwaStackFrame[10] = 0xBABA; /*R6*/
	dwaStackFrame[11] = 0xBABA; /*R7*/
	dwaStackFrame[12] = 0xBABA; /*R8*/
	dwaStackFrame[13] = 0xBABA; /*R9*/
	dwaStackFrame[14] = 0xBABA; /*R10*/
	dwaStackFrame[15] = 0xBABA; /*R11*/
}

void vfnReadyQueue(const uint8_t kbTaskNumber, const uint8_t kfInOrOut)
{
	static uint8_t _bNumberOfReadyTasks = 0;
	uint8_t bCounter = 0;
	uint8_t bIndex = 0;

	if(kfInOrOut) /*Si se va a agregar un task a la queue*/
	{
		gbaReadyQueue[_bNumberOfReadyTasks++] = kbTaskNumber; /*El argumento de entrada se acomoda en la siguiente posición disponible del buffer*/
	}
	else /*Si se va a quitar un task a la queue*/
	{
		while(saTCD[gbaReadyQueue[bIndex]].bTaskNumber != kbTaskNumber)
		{
			bIndex++;
			bCounter++;
			/*Se barre el arreglo hasta encontrar el Task a quitar*/
		}
		bCounter++;
		bCounter -= _bNumberOfReadyTasks;

		while(bCounter--)
		{
			gbaReadyQueue[bIndex] = gbaReadyQueue[bIndex + 1]; /*Se recorren las demás posiciones para llenar el espacio*/
			bIndex++;
		}
		gbaReadyQueue[bIndex] = MAX_TASKS; /*Se llena el último espacio con MAX_TASKS para saber que se encuentra vacío*/
		_bNumberOfReadyTasks--;
	}
}

void SysTick_Handler(void)
{
	uint8_t bCounter = MAX_TASKS;

	while(bCounter--) /*Se barre el arreglo*/
	{
		if(gdwaTaskBlockedSysTick[bCounter]) /*Si alguna posición tiene ticks restantes*/
		{
			if(!--(gdwaTaskBlockedSysTick[bCounter])) /*Se le resta un tick y se revisa si era el último*/
			{
				vfnReadyQueue(bCounter, 1);
			}
		}
	}
}


