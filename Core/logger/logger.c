/*
 * logger.c
 *
 *  Created on: Mar 10, 2020
 *      Author: Igor
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "logger.h"
#include "logger_config.h"

#if (LOGGER_ENABLE)

#if (LOGGER_USE_RTOS)
# include "FreeRTOS.h"
# include "semphr.h"
# include "task.h"
  static SemaphoreHandle_t mutexLogger = NULL;
  #define LOGGER_META_LEN (32 + configMAX_TASK_NAME_LEN)
#else
  volatile static uint8_t _loggerLock = 0;
  #define LOGGER_META_LEN (32)
#endif


#define LOGGER_BUFFER_SIZE (LOGGER_META_LEN + LOGGER_MSG_LEN + sizeof(LOGGER_LINE_TERMINATION))

#if (LOGGER_USE_DOUBLE_BUFFERING != 0)
  #define LOGGER_BUFFER_COUNT (2)
#else
  #define LOGGER_BUFFER_COUNT (1)
#endif

static char loggerBuffer[LOGGER_BUFFER_COUNT][LOGGER_BUFFER_SIZE];
static uint8_t debugLevel = LOG_NONE;
static uint8_t buffIndex = 0;

static void __logCreateLock(void);
static uint8_t __logLock(void);
static void __logUnlock(void);


// Function that's sending string out
// Provide hardware platform specific definition in application code
__weak void _logTransmitString(char * ptr, uint32_t len )
{
  UNUSED(ptr);
  UNUSED(len);
  return;
}



// Utility function to convert log level to it's string representation.
static const char*
getLevelStr (int32_t level)
{
  const char * str;

  switch (level) {
    case LOG_DEBUG:    str = "D"; break;
    case LOG_INFO:     str = "I"; break;
    case LOG_ERROR:    str = "E"; break;
    case LOG_CRITICAL: str = "C"; break;
    default:           str = ""; break;
  }
  return str;
}

void
_logSetLevel (const uint8_t level)
{
  debugLevel = level;
}

void
_logInit (const uint8_t level)
{
  __logCreateLock();
  _logSetLevel (level);
}

void
_log (const uint8_t level, const char *format, ...)
{

  if ((level < debugLevel) || (level > _LOG_LAST_LVL)) return;  // filter levels

  if (__logLock()) return; // failed to lock

  int32_t len_meta, len_msg, len_total;
  va_list args;
  va_start(args, format);

  char * currBuff = &loggerBuffer[buffIndex][0];

  // Print timestamp and debug level
#if (LOGGER_USE_RTOS)
  char *threadName = "[n/a]";
  if ( xPortIsInsideInterrupt() == pdTRUE)
    {
      // Printing from ISR must be avoided at all cost, but if this happens,
      // give a clear indication of fault
      threadName = "!!! ISR !!!";
    }
  else if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
      threadName = pcTaskGetName(NULL);
    }


  len_meta = snprintf (currBuff,
                  LOGGER_META_LEN,
                  "%010ld %s %s: ",
                  xTaskGetTickCount(),
                  threadName,
                  getLevelStr (level) );
#else
  len_meta = snprintf (currBuff,
                  LOGGER_META_LEN,
                  "%010ld %s: ",
                  HAL_GetTick(),
                    getLevelStr (level)
                  );
#endif
  if (len_meta >= LOGGER_META_LEN) len_meta = LOGGER_META_LEN -1;

  // Print actual message
  len_msg = vsnprintf (&currBuff[len_meta],
                   LOGGER_MSG_LEN,  // -1 for line feed at the end
                   format,
                   args);
  if (len_msg >= LOGGER_MSG_LEN) len_msg = LOGGER_MSG_LEN -1;

  len_total = len_meta+len_msg;

  /* remove line termination if it's in message itself (e.g. from external lib) */
  if (currBuff[len_total-1] == '\r' || currBuff[len_total-1] == '\n' ) len_total--;
  if (currBuff[len_total-1] == '\r' || currBuff[len_total-1] == '\n' ) len_total--;

  /* add line termination specified in the config */
  strcpy(&currBuff[len_total], LOGGER_LINE_TERMINATION);
  _logTransmitString(currBuff, len_total + sizeof(LOGGER_LINE_TERMINATION)-1);

  /* buffer switch */
  buffIndex++;
  if (buffIndex >= LOGGER_BUFFER_COUNT) buffIndex = 0;

  va_end(args);
  __logUnlock();
}

static void __logCreateLock(void)
{
#if (LOGGER_USE_RTOS)
  if (mutexLogger == NULL)
    {
      mutexLogger = xSemaphoreCreateMutex();
    }
#else
  _loggerLock = 0;
#endif
}

static uint8_t __logLock(void)
{
#if (LOGGER_USE_RTOS)
  // do not take mutex if scheduler is not running or logger executed from interrupt
  if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING ||
      xPortIsInsideInterrupt() == pdTRUE) return 0;
  BaseType_t status = xSemaphoreTake(mutexLogger, pdMS_TO_TICKS(10) );
  if (status != pdPASS) return 1;
#else
  uint8_t timeout = 10;
  do {
      if (_loggerLock == 0) break;
      HAL_Delay(1);
  } while (--timeout);
  if (timeout == 0) return 1;

  __disable_irq();
  _loggerLock = 1;
  __enable_irq();
#endif
  return 0;
}

static void __logUnlock(void)
{
#if (LOGGER_USE_RTOS)
  // do not give mutex if scheduler is not running or logger executed from interrupt
  if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING ||
      xPortIsInsideInterrupt() == pdTRUE) return;
  (void)xSemaphoreGive(mutexLogger);
#else
  __disable_irq();
  _loggerLock = 0;
  __enable_irq();
#endif
}



#endif
