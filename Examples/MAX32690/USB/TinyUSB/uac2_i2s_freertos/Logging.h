/******************************************************************************
 *
 * Copyright (C) 2025 Analog Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#ifndef EXAMPLES_MAX32690_USB_TINYUSB_UAC2_I2S_FREERTOS_LOGGING_H_
#define EXAMPLES_MAX32690_USB_TINYUSB_UAC2_I2S_FREERTOS_LOGGING_H_

#include <stdarg.h>
#include <stdio.h>

/**
 * Defines what the source of the logged information is from
 */
typedef enum {
    BKGND = 0, /**< Background Task    */
    USBD,
    CODEC,
    I2S,

    //Always last for a count. Don't use
    LOG_SOURCE_COUNT
} log_source_t;

/**
 * Defines the level to log information at
 */
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
} log_level_t;

/* The global log level is the highest level allowed at compile time. If not
 * set as a command line parameter, this is the default
 */
#ifndef GLOBAL_LOG_LEVEL
#define GLOBAL_LOG_LEVEL LOG_LEVEL_WARN
#endif

/**
 * Initializes the LoggingSystem.  All sources by default get GLOBAL_LOG_LEVEL
 */
int LoggingInit(void);

/**
 * Gets the current logging level for the provided source
 * @param src - Source to inspect
 * @returns Current log level
 */
log_level_t LoggingGetSourceLevel(log_source_t src);

/**
 * Sets the logging level for the given source. Will be clamped to
 * GLOBAL_LOG_LEVEL
 * @param src - Source to set
 * @param level - Level to set
 */
void LoggingSetSourceLevel(log_source_t src, log_level_t level);

/**
 * Performs a print to the logging console based on format and args.
 * @param fmt - Format string
 * @param ... - Variable arguments
 */
int LoggingPrint(const char *fmt, ...);

/**
 * Performs a print to the logging console using va_list arguments
 * @param fmt - Format string
 * @param args - Argument list
 */
void LoggingvPrint(const char *fmt, va_list args);

/** Were the logged output should go.  This should have the same argument
 *  setup as printf  (fmt, ...)
 */
#define LOG_MSG_OUTPUT LoggingPrint

/*****
 * The following Macros should be used for logging.  This allows compile time
 * optimization of eliminating Log statements if the GLOBAL_LOG_LEVEL is not
 * high enough for them.
 */
#if GLOBAL_LOG_LEVEL >= LOG_LEVEL_ERR
#define LOG_MSG_ERR(src, msg, ...)                   \
    if (LoggingGetSourceLevel(src) >= LOG_LEVEL_ERR) \
    LOG_MSG_OUTPUT("ERR:[" #src "]:" msg "\n", __VA_ARGS__)
#else
#define LOG_MSG_ERR(src, msg, ...)
#endif

#if GLOBAL_LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_MSG_WARN(src, msg, ...)                   \
    if (LoggingGetSourceLevel(src) >= LOG_LEVEL_WARN) \
    LOG_MSG_OUTPUT("WARN:[" #src "]:" msg "\n", __VA_ARGS__)
#else
#define LOG_MSG_WARN(src, msg, ...)
#endif

#if GLOBAL_LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_MSG_INFO(src, msg, ...)                   \
    if (LoggingGetSourceLevel(src) >= LOG_LEVEL_INFO) \
    LOG_MSG_OUTPUT("INFO:[" #src "]:" msg "\n", __VA_ARGS__)
#else
#define LOG_MSG_INFO(src, msg, ...)
#endif

#if GLOBAL_LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_MSG_DBG(src, msg, ...)                     \
    if (LoggingGetSourceLevel(src) >= LOG_LEVEL_DEBUG) \
    LOG_MSG_OUTPUT("DBG:[" #src "]:" msg "\n", __VA_ARGS__)
#else
#define LOG_MSG_DBG(src, msg, ...)
#endif

/* Same MACROs as above but '0' versions which have no values passed to the
   printf call.  C99 (and others) wont allow __VA_ARGS__ to be empty, this
   is a solution to that
 */
#if GLOBAL_LOG_LEVEL >= LOG_LEVEL_ERR
#define LOG_MSG_ERR0(src, msg)                       \
    if (LoggingGetSourceLevel(src) >= LOG_LEVEL_ERR) \
    LOG_MSG_OUTPUT("ERR:[" #src "]:" msg "\n")
#else
#define LOG_MSG_ERR0(src, msg)
#endif

#if GLOBAL_LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_MSG_WARN0(src, msg)                       \
    if (LoggingGetSourceLevel(src) >= LOG_LEVEL_WARN) \
    LOG_MSG_OUTPUT("WARN:[" #src "]:" msg "\n")
#else
#define LOG_MSG_WARN0(src, msg)
#endif

#if GLOBAL_LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_MSG_INFO0(src, msg)                       \
    if (LoggingGetSourceLevel(src) >= LOG_LEVEL_INFO) \
    LOG_MSG_OUTPUT("INFO:[" #src "]:" msg "\n")
#else
#define LOG_MSG_INFO0(src, msg)
#endif

#if GLOBAL_LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_MSG_DBG0(src, msg)                         \
    if (LoggingGetSourceLevel(src) >= LOG_LEVEL_DEBUG) \
    LOG_MSG_OUTPUT("DBG:[" #src "]:" msg "\n")
#else
#define LOG_MSG_DBG0(src, msg)
#endif
#endif // EXAMPLES_MAX32690_USB_TINYUSB_UAC2_I2S_FREERTOS_LOGGING_H_
