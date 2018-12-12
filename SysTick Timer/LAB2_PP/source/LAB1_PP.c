/*
 * Copyright 2016-2018 NXP Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * @file    Scheduler.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "MKL25Z4.h"
#include "TaskDriver.h"
#include "SchedFunc.h"

#define TARGET_TIME_5MS 105000

static void _vfnIncrementTask(void *pParameters);
static void _vfnDecrementTask(void *pParameters);
static void _vfnDoubleTask(void *pParameters);
static void _vfnHalveTask(void *pParameters);

uint16_t gwVariable = 1; /*Variable global para los tasks*/

/*
 * @brief   Application entry point.
 */
int main(void)
{
	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	/* Init FSL debug console. */
	BOARD_InitDebugConsole();


	if(SysTick_Config(TARGET_TIME_5MS))
	{
#if DEBUG_MODE
		PRINTF("Invalid SystemTick reload value!.\r\n");
#endif
		while (1)
		{
		}
	}
    NVIC_SetPriority(PendSV_IRQn, 255); /*Prioridad más baja posible*/
    NVIC_SetPriority(SysTick_IRQn, 0); /*Prioridad más alta posible*/

	if (!flgfnTaskCreate(_vfnIncrementTask, "INCREMENT_TASK", STACK_SIZE, NULL, 3))
	{
#if DEBUG_MODE
		PRINTF("Task creation failed!.\r\n");
#endif
		while (1)
		{
		}
	}

	vfnTaskStartSched();

	while(1)
	{

	}
	return 0 ;
}

static void _vfnIncrementTask(void *pParameters)
{
	gwVariable++;
	if (!flgfnTaskCreate(_vfnDecrementTask, "DECREMENT_TASK", STACK_SIZE, NULL, 2))
	{
#if DEBUG_MODE
		PRINTF("Task creation failed!.\r\n");
#endif
		while (1)
		{
		}
	}

	vfnTaskYield();
}

static void _vfnDecrementTask(void *pParameters)
{
	if(gwVariable)
	{
		gwVariable--;
	}
	if (!flgfnTaskCreate(_vfnDoubleTask, "DOUBLE_TASK", STACK_SIZE, NULL, 1))
	{
#if DEBUG_MODE
		PRINTF("Task creation failed!.\r\n");
#endif
		while (1)
		{
		}
	}
	vfnTaskYield();
}

static void _vfnDoubleTask(void *pParameters)
{
	gwVariable = gwVariable*2;
	if (!flgfnTaskCreate(_vfnHalveTask, "HALVE_TASK", STACK_SIZE, NULL, 0))
	{
#if DEBUG_MODE
		PRINTF("Task creation failed!.\r\n");
#endif
	}
	vfnTaskYield();
}

static void _vfnHalveTask(void *pParameters)
{
	gwVariable = gwVariable/2;
	if(!gwVariable)
	{
		gwVariable++;
		vfnTaskDelayBlocked(4, 400); /*2 segundos bloqueado*/
	}
	vfnTaskYield();
}

void vfnIdleTask(void *pParameters)
{
	vfnTaskYield();
}

