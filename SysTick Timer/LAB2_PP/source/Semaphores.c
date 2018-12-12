/*
 * Semaphores.c
 *
 *  Created on: 24/09/2018
 *      Author: Windows 8.1
 */

#include <stdint.h>
#include "Semaphores.h"
#include "TaskDriver.h"

void * pvfnSystemMalloc(uint16_t wSizeBytes)
{
	static uint32_t _waStackFrameForAllTasks[MAX_TASKS][STACK_SIZE/4]; /*Espacio de memoria total para todos los stacks posibles*/
	static uint32_t * _wpStackFrameForAllTasks = &_waStackFrameForAllTasks[0][0];
	static uint32_t _wRemainingBytes = MAX_TASKS*STACK_SIZE;

	void * pvAllocatedMemoryAddress;

	_wRemainingBytes -= wSizeBytes;
	if(!_wRemainingBytes) /*Si no hay espacio suficiente*/
	{
		_wRemainingBytes += wSizeBytes;
		return NULL;
	}

	pvAllocatedMemoryAddress = _wpStackFrameForAllTasks;

	_wpStackFrameForAllTasks += wSizeBytes/4; /*Se incrementa el apuntador para asignar los siguientes espacios disponibles en la próxima instancia*/

	return pvAllocatedMemoryAddress;
}

void vfnSemaphoreTake(uint8_t * pbSemaphore)
{
	*pbSemaphore = 0;
}

void vfnSemaphoreGive(uint8_t * pbSemaphore)
{
	*pbSemaphore = 1;
}
