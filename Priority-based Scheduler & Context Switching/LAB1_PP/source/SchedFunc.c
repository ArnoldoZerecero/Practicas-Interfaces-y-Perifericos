/*
 * SchedFunc.c
 *
 *  Created on: 01/10/2018
 *      Author: Edwin Francisco Zamora Oros
 */

#include <stdint.h>
#include "SchedFunc.h"
#include "TaskDriver.h"
#include "fsl_debug_console.h"

void * gpvNextTaskAddress;

void * gpvCurrentStackAddress;

void * gpvNextStackAddress;

uint8_t gbNextTask = 0;

void vfnTaskStartSched(void)
{
	if(!flgfnTaskCreate(vfnIdleTask, "IDLE_TASK", STACK_SIZE, NULL, IDLE_PRIORITY)) //Create lowest priority task
	{
#if DEBUG_MODE
		PRINTF("Task creation failed!.\r\n");
#endif
		while (1)
		{
		}
	}
	vfnPrepareNextContext();

	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; /*Se forza una interrupción*/
}

uint8_t flgfnSchedPolicy(uint8_t bNumberOfTasks)
{
	uint8_t flgPriorityOk = 1;

	while(--bNumberOfTasks)
	{
		if(saTCD[bNumberOfTasks].bTaskPriority == saTCD[bNumberOfTasks - 1].bTaskPriority)
		{
			flgPriorityOk = 0; /*Error, prioridad repetida*/
		}
	}
	return flgPriorityOk;
}

uint8_t bfnSched(void)
{
	uint8_t bCounter = 0;
	uint8_t bMaximumPriority = 255;
	uint8_t bTaskNumber = 0;

	while((gbaReadyQueue[bCounter] != MAX_TASKS))
	{
		if(saTCD[gbaReadyQueue[bCounter]].bTaskPriority < bMaximumPriority)
		{
			bMaximumPriority = saTCD[gbaReadyQueue[bCounter]].bTaskPriority;
			bTaskNumber = gbaReadyQueue[bCounter];
		}
		bCounter++;
	}
	return bTaskNumber;
}

void vfnPrepareNextContext(void)
{
	gbNextTask = bfnSched(); /*Se decide cuál task es el siguiente desde la ready queue, según su prioridad*/
	gpvNextTaskAddress = (uint32_t *)saTCD[gbNextTask].pvTaskAddress; /*Se pasan direcciones de task y task stack a apuntadores globales*/
	gpvNextStackAddress = (uint32_t *)&saTCD[gbNextTask].dwaStackFrame;
}

__attribute__ (( naked )) void PendSV_Handler(void)
{
	__asm volatile
	(
			"	.syntax unified				    \n"
			/*Se carga a r1 la dirección del stack frame a almacenar*/
			"  ldr  r1, =gpvCurrentStackAddress \n"
			/*Se coloca el apuntador encima de los registros manuales*/
			"  adds r1, #56                    \n"
			/*Guardar en el stack frame, los 8 registros manuales*/
			"   str    r4,[r1,#0]               \n"
			"   str    r5,[r1,#4]               \n"
			"   str    r6,[r1,#8]               \n"
			"   str    r7,[r1,#12]              \n"
			/*Se pasan los registros altos a los bajos para poder hacer str*/
			"	mov    r4, r8                  \n"
			"	mov    r5, r9                  \n"
			"	mov    r6, r10                 \n"
			"	mov    r7, r11                 \n"
			"   str    r4,[r1,#16]             \n"
			"   str    r5,[r1,#20]             \n"
			"   str    r6,[r1,#24]             \n"
			"   str    r7,[r1,#28]             \n"
			/*Se accede a la función en C para preparar el context a cargar*/
			/*Se le pasa al r2, la dirección del stack frame a cargar*/
			"  ldr	r2, =gpvNextStackAddress   \n"
			/*Se coloca el apuntador encima de los últimos 4 registros manuales*/
			"  adds r2,  #68                   \n"
			/*Cargar desde el stack frame, los últimos 4 registros manuales*/
			"   ldr  r4, [r2,#0]               \n"
			"   ldr  r5, [r2,#4]               \n"
			"   ldr  r6, [r2,#8]               \n"
			"   ldr  r7, [r2,#12]              \n"
			/*Se pasan los registros bajos a los altos*/
			"	mov r8, r4                     \n"
			"	mov r9, r5                     \n"
			"	mov r10, r6                    \n"
			"	mov r11, r7                    \n"
			/*Se coloca el apuntador encima de los primeros 4 registros manuales*/
			"   subs r2,#16                    \n"
			/*Cargar desde el stack frame, los primeros 4 registros manuales*/
			"   ldr  r4, [r2,#0]               \n"
			"   ldr  r5, [r2,#4]               \n"
			"   ldr  r6, [r2,#8]               \n"
			"   ldr  r7, [r2,#12]              \n"
			/*Se carga la dirección del next task*/
			"   ldr r1,=gpvNextTaskAddress     \n"
			"   ldr r2,[r1,#0]                 \n"
			"	bx 	r2					       \n"
			"	.align 4						\n"
	);
}
