/*
 * logger.h
 *
 * Logger library for cortex M3+ devices.
 * Uses ITM (Instrumentation Trace Macrocell) for writing the messages.
 *
 *  Created on: Mar 10, 2020
 *      Author: Igor
 */

#ifndef INC_LOGGER_H_
#define INC_LOGGER_H_


#ifndef LOGGER_ENABLE
  #define LOGGER_ENABLE (0)
#endif

#if (LOGGER_ENABLE)

#define LOG_NONE      (0xFF)
#define LOG_CRITICAL  (0x03)
#define LOG_ERROR     (0x02)
#define LOG_INFO      (0x01)
#define LOG_DEBUG     (0x00)
#define _LOG_LAST_LVL (LOG_CRITICAL)


/**
* @brief  Log a message.
* @param  level - one of the message levels, e.g., LOG_ERROR.
* @param  format - string that will be logged, may optionally contain format
*           specifiers (see printf).
* @param  ... - additional arguments to replace format specifiers in format
*         string.
* @retval None
*/
void
_log (const uint8_t level, const char *format, ...);
#define log(level,...) do { _log((level),__VA_ARGS__); } while(0)

/**
* @brief  Log a CRITICAL message.
* @param  format - string that will be logged, may optionally contain format
*           specifiers (see printf).
* @param  ... - additional arguments to replace format specifiers in format
*         string.
* @retval None
*/
#define logCrit(...)   do { _log(LOG_CRITICAL, __VA_ARGS__); } while(0)

/**
* @brief  Log a ERROR message.
* @param  format - string that will be logged, may optionally contain format
*           specifiers (see printf).
* @param  ... - additional arguments to replace format specifiers in format
*         string.
* @retval None
*/
#define logError(...)  do { _log(LOG_ERROR, __VA_ARGS__); } while(0)

/**
* @brief  Log a INFO message.
* @param  format - string that will be logged, may optionally contain format
*           specifiers (see printf).
* @param  ... - additional arguments to replace format specifiers in format
*         string.
* @retval None
*/
#define logInfo(...)   do { _log(LOG_INFO, __VA_ARGS__); } while(0)

/**
* @brief  Log a DEBUG message.
* @param  format - string that will be logged, may optionally contain format
*           specifiers (see printf).
* @param  ... - additional arguments to replace format specifiers in format
*         string.
* @retval None
*/
#define logDebug(...)  do { _log(LOG_DEBUG, __VA_ARGS__); } while(0)

/**
* @brief  Set the log level specifying which messages will be logged.
* @param  level - value of log level, e.g. LOG_ERROR.
* @retval None
*/
void
_logSetLevel (const uint8_t level);
#define logSetLevel(level)  do { _logSetLevel((level)); } while(0)

/**
* @brief  Initialise logging library. Must be called before logging any message.
* @param  level - value of log level, e.g. LOG_ERROR.
* @retval None
*/
void
_logInit (const uint8_t level);
#define logInit(level)  do { _logInit((level)); } while(0)

#else

#define logInit(...)      do {} while (0)
#define log(...)          do {} while (0)
#define logSetLevel(...)  do {} while (0)
#define logCrit(...)      do {} while (0)
#define logError(...)     do {} while (0)
#define logInfo(...)      do {} while (0)
#define logDebug(...)     do {} while (0)

#endif /* LOGGER_ENABLE */

#endif /* INC_LOGGER_H_ */
