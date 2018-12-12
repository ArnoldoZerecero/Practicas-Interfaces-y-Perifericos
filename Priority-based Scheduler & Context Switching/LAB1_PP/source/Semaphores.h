/*
 * Semaphores.h
 *
 *  Created on: 24/09/2018
 *      Author: Windows 8.1
 */

#ifndef SEMAPHORES_H_
#define SEMAPHORES_H_

#include <stdint.h>

void * pvfnSystemMalloc(uint16_t wSizeBytes);

void vfnSemaphoreTake(uint8_t * pbSemaphore);

void vfnSemaphoreGive(uint8_t * pbSemaphore);

#endif /* SEMAPHORES_H_ */
