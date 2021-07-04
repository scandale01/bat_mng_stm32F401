/*
 * logger_config.h
 *
 *  Created on: Mar 11, 2020
 *      Author: Igor Petrov
 */

#ifndef INC_LOGGER_CONFIG_H_
#define INC_LOGGER_CONFIG_H_

#include <stm32f4xx.h>

#define LOGGER_USE_RTOS    (0)    // 1- use RTOS mutex, 0 - use basic spin lock
#define LOGGER_MSG_LEN (128)  // Statically allocated buffer size

/* Allows to prepare a new data buffer in parallel with the transfer. *
 * Double buffering is should be used together with DMA/IT transfer and should
 * not be used if data transfer is blocking. *
 */

#define LOGGER_USE_DOUBLE_BUFFERING (0)

#define LOGGER_LINE_TERMINATION ("\n")

#endif /* INC_LOGGER_CONFIG_H_ */
