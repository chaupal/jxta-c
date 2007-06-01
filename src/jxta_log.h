/*
 * Copyright (c) 2004 Henry Jen. All rights reserved.
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
 */

#ifndef __JXTA_LOG_H__
#define __JXTA_LOG_H__

#include <stdarg.h>

#include "jxta_types.h"
#include "jxta_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

    enum Jxta_log_levels {
        JXTA_LOG_LEVEL_INVALID = -1,
        JXTA_LOG_LEVEL_MIN = 0,
        JXTA_LOG_LEVEL_FATAL = 0,
        JXTA_LOG_LEVEL_ERROR,
        JXTA_LOG_LEVEL_WARNING,
        JXTA_LOG_LEVEL_INFO,
        JXTA_LOG_LEVEL_DEBUG,
        JXTA_LOG_LEVEL_TRACE,
        JXTA_LOG_LEVEL_PARANOID,
        JXTA_LOG_LEVEL_MAX
    };

    typedef enum Jxta_log_levels Jxta_log_level;

#define JXTA_LOG_LEVEL_FLAG_FATAL 1
#define JXTA_LOG_LEVEL_FLAG_ERROR 2
#define JXTA_LOG_LEVEL_FLAG_WARNING 4
#define JXTA_LOG_LEVEL_FLAG_INFO 8
#define JXTA_LOG_LEVEL_FLAG_DEBUG 16
#define JXTA_LOG_LEVEL_FLAG_TRACE 32
#define JXTA_LOG_LEVEL_FLAG_PARANOID 64

/* Mask for all level less then specified */
#define JXTA_LOG_LEVEL_MASK_NONE 0
#define JXTA_LOG_LEVEL_MASK_FATAL 1
#define JXTA_LOG_LEVEL_MASK_ERROR 3
#define JXTA_LOG_LEVEL_MASK_WARNING 7
#define JXTA_LOG_LEVEL_MASK_INFO 15
#define JXTA_LOG_LEVEL_MASK_DEBUG 31
#define JXTA_LOG_LEVEL_MASK_TRACE 63
#define JXTA_LOG_LEVEL_MASK_PARANOID 127
#define JXTA_LOG_LEVEL_MASK_ALL 127

/**
 * Prototype of the logger function
 *
 * @param void * user data, typically would be a struct holding the
 *        data needed by the logger function
 * @param const char * cat, a character string can be used to
 *        categorize the log entriedi, also can be used by a selector to
 *        decide whether the message should be logged.
 * @param int level, the level of the log message, can be used by a
 *        selector to decide whether the message should be logged.
 * @param const char * fmt, the format string
 * @param va_list ap, the arguments for the format string
 *
 * @return Status code
 */
    typedef Jxta_status(JXTA_STDCALL * Jxta_log_callback) (void *user_data, const char *cat,
                                                           int level, const char *fmt, va_list ap);

     JXTA_DECLARE(Jxta_status) jxta_log_initialize(void);
     JXTA_DECLARE(void) jxta_log_terminate(void);

/**
 * Register the logger function
 *
 * @param Jxta_log_callback, the logger function
 * @param void * user data, typically would be a struct holding the
 *        data needed by the logger function
 */
     JXTA_DECLARE(void) jxta_log_using(Jxta_log_callback log_cb, void *user_data);

/**
 * Call this function to append log with the global registered callback
 * logger function
 */
     JXTA_DECLARE(Jxta_status) jxta_log_append(const char *cat, int level, const char *fmt, ...);

/**
 * Call this function to append log with the global registered callback
 * logger function
 */
     JXTA_DECLARE(Jxta_status) jxta_log_appendv(const char *cat, int level, const char *fmt, va_list ap);

/**
 * Log selector - helper structure to determine whether a log should be
 * recorded by the logger
 */
    typedef struct Jxta_log_selector Jxta_log_selector;

     JXTA_DECLARE(Jxta_log_selector *) jxta_log_selector_new(void);

     JXTA_DECLARE(void) jxta_log_selector_delete(Jxta_log_selector * self);

/**
 * Create a log selector
 *
 * @return Jxta_log_selector* - the pointer to the Jxta_log_selector
 *         structure constructed. NULL if the seletor is invalid.
 */
     JXTA_DECLARE(Jxta_log_selector *) jxta_log_selector_new_and_set(const char *selector, Jxta_status * rv);

     JXTA_DECLARE(Jxta_status) jxta_log_selector_add_category(Jxta_log_selector * self, const char *cat);

     JXTA_DECLARE(Jxta_status) jxta_log_selector_remove_category(Jxta_log_selector * self, const char *cat);

     JXTA_DECLARE(Jxta_status) jxta_log_selector_list_positive(Jxta_log_selector * self, Jxta_boolean positive);

     JXTA_DECLARE(Jxta_status) jxta_log_selector_set_level_mask(Jxta_log_selector * self, unsigned int mask);

     JXTA_DECLARE(Jxta_status) jxta_log_selector_clear_level_mask(Jxta_log_selector * self, unsigned int mask);

/**
 * Set the selector by config string
 *
 * @param const char * selector - the string used to initialize
 *        selector, the format is basically like selector used by syslog.conf
 *
 */
     JXTA_DECLARE(Jxta_status) jxta_log_selector_set(Jxta_log_selector * self, const char *selector);

/**
 * Determine whether the category and level is selected by the selector
 *
 * @param Jxta_log_selector * self - the pointer to the Jxta_log_selector
 *        structure used to make the decision
 * @param const char * cat - the log category
 * @param int level - the log level
 *
 * @return 0 if not selected, 1 otherwise.
 */
     JXTA_DECLARE(Jxta_boolean) jxta_log_selector_is_selected(Jxta_log_selector * self, const char *cat, Jxta_log_level level);

/**
 * Log into a user file
 */
    typedef struct Jxta_log_file Jxta_log_file;

/**
 * Open a log file
 * @param Jxta_log_file ** newf, pointer to receive the Jxta_log_file 
 *        structure
 * @param const char *fname, the file name to be opened. Should be a
 *        absolute path to ensure the file location; Otherwise, the file
 *        will be created as a relative path to current folder
 */
     JXTA_DECLARE(Jxta_status) jxta_log_file_open(Jxta_log_file ** newf, const char *fname);

/**
 * Close the log file
 */
     JXTA_DECLARE(Jxta_status) jxta_log_file_close(Jxta_log_file * self);

/**
 * The logger callback function
 *
 * @param void *userdata, the pointer to the Jxta_log_file structure
 */
     JXTA_DECLARE(Jxta_status)
     jxta_log_file_append(void *user_data, const char *cat, int level, const char *fmt, va_list ap);

/**
 * Set the selector for the log file
 */
     JXTA_DECLARE(Jxta_status) jxta_log_file_attach_selector(Jxta_log_file * self, Jxta_log_selector * selector,
                                                             Jxta_log_selector ** orig_sel);

#define _STR(x) _VAL(x)
#define _VAL(x) #x

/**
* A Convenience macro for use in jxta_log_append(). Produces a string 
*       [file:line]
* of the current source file and line. Useful for some log messages.
**/
#define FILEANDLINE "[" __FILE__ ":" _STR(__LINE__) "] "

#ifdef __cplusplus
}                               /* extern "C" */
#endif
#endif                          /* __JXTA_LOG_H__ */
/* vim: set sw=4 ts=4 et: */
