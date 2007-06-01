/* 
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *       Sun Microsystems, Inc. for Project JXTA."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact Project JXTA at http://www.jxta.org.
 *
 * 5. Products derived from this software may not be called "JXTA",
 *    nor may "JXTA" appear in their name, without prior written
 *    permission of Sun.
 *
 * THIS SOFTWARE IS PROVIDED AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL SUN MICROSYSTEMS OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of Project JXTA.  For more
 * information on Project JXTA, please see
 * <http://www.jxta.org/>.
 *
 * This license is based on the BSD license adopted by the Apache Foundation.
 *
 * $Id: unittest_jxta_func.c,v 1.12 2006/09/29 06:52:06 slowhog Exp $
 */

#include <stdio.h>

/** FIXME: apr_general.h need to be included somewhere in jpr.
 *  None of the code will compile or run in win32 without it.
 */
#include <apr_general.h>

#include <jxta.h>

#include "unittest_jxta_func.h"

Jxta_boolean run_testfunctions(const struct _funcs testfunc[], int *tests_run, int *tests_passed, int *tests_failed)
{
    Jxta_boolean passed = TRUE;
    int i = 0;
    
    while (testfunc[i].test_func != NULL) {
        const char * result = testfunc[i].test_func();

        *tests_run += 1;
        
        if (NULL == result) {
            fprintf(stderr, "#%d : PASSED : %s \n", (i + 1), testfunc[i].test_desc);
            *tests_passed += 1;
        } else {
            fprintf(stderr, "#%d : FAILED : %s reason : %s\n", (i + 1), testfunc[i].test_desc, result);
            passed = FALSE;
            *tests_failed += 1;
        }
        
        i++;
    }

    return passed;
}

static const char *verbositys[] = { "warning", "info", "debug", "trace", "paranoid" };

/** 
 * A helper function that can be called from the main function if the test programs 
 * are run as standalone application.
 */
int main_test_suites(const struct _suite suite[], int argc, char **argv) {
    int verbosity = 0;
    Jxta_log_file *log_f = NULL;
    Jxta_log_selector *log_s = NULL;
    char logselector[256];
    char *lfname = NULL;
    char *fname = NULL;
    int run, failed, passed;
    Jxta_boolean result;
    Jxta_status status;
    int optc;
    char **optv = argv;
    int each_suite = 0;
    
    jxta_initialize();

    for( optc = 1; optc < argc; optc++ ) {
        char *opt = argv[optc];
        
        if( *opt != '-' ) {
            fprintf(stderr, "# %s - Invalid option.\n", argv[0]);
            status = JXTA_INVALID_ARGUMENT;
            goto FINAL_EXIT;
        }
        
        opt++;
        
        switch( *opt ) {
            case 'v' :
                verbosity++;
                break;
                
            case 'l' :
                lfname = strdup(argv[++optc]);
                break;

            case 'f' : 
                fname = strdup(argv[++optc]);
                break;
            
            default :
                fprintf(stderr, "# %s - Invalid option : %c.\n", argv[0], *opt );
        status = JXTA_INVALID_ARGUMENT;
        goto FINAL_EXIT;
        }
    }

    run = failed = passed = 0;

    if (verbosity > (sizeof(verbositys) / sizeof(const char *)) - 1) {
        verbosity = (sizeof(verbositys) / sizeof(const char *)) - 1;
    }

    strcpy(logselector, "*.");
    strcat(logselector, verbositys[verbosity]);
    strcat(logselector, "-fatal");

    fprintf(stderr, "Starting logger with selector : %s\n", logselector);
    log_s = jxta_log_selector_new_and_set(logselector, &status);
    if (NULL == log_s) {
        fprintf(stderr, "# %s - Failed to init default log selector.\n", argv[0]);
        status = JXTA_INVALID_ARGUMENT;
        goto FINAL_EXIT;
    }

    if (fname) {
        jxta_log_file_open(&log_f, fname);
        free(fname);
    } else {
        jxta_log_file_open(&log_f, "-");
    }

    jxta_log_using(jxta_log_file_append, log_f);
    jxta_log_file_attach_selector(log_f, log_s, NULL);

    if (NULL != lfname) {
        FILE *loggers = fopen(lfname, "r");

        if (NULL != loggers) {
            do {
                if (NULL == fgets(logselector, sizeof(logselector), loggers)) {
                    break;
                }

                fprintf(stderr, "Starting logger with selector : %s\n", logselector);

                log_s = jxta_log_selector_new_and_set(logselector, &status);
                if (NULL == log_s) {
                    fprintf(stderr, "# %s - Failed to init default log selector.\n", argv[0]);
                    status = JXTA_INVALID_ARGUMENT;
                    goto FINAL_EXIT;
                }

                jxta_log_file_attach_selector(log_f, log_s, NULL);
            } while (TRUE);

            fclose(loggers);
        }
    }

    jxta_log_file_attach_selector(log_f, log_s, NULL);

    if (lfname) {
        free(lfname);
    }

    while( NULL != suite[each_suite].suite_func ) {
        Jxta_boolean success;
        int suite_run = 0;
        int suite_passed = 0;
        int suite_failed = 0;
        
        fprintf(stderr, "=========== %s ============\n", suite[each_suite].suite_desc);
        
        success = suite[each_suite].suite_func(&suite_run, &suite_passed, &suite_failed);
        
        if( !success ) {
            fprintf(stderr, "Suite FAILED : %s\n", suite[each_suite].suite_desc );
            fprintf(stderr, "Tests run:    %d\n", suite_run);
            fprintf(stderr, "Tests passed: %d\n", suite_passed);
            fprintf(stderr, "Tests failed: %d\n", suite_failed);
        }
        
        run += suite_run;
        passed += suite_passed;
        failed += suite_failed;
        
        each_suite++;
    }
    
    fprintf(stderr, "=========== UNIT TEST RESULTS ============\n" );
    
    fprintf(stderr, "Tests run:    %d\n", run);
    fprintf(stderr, "Tests passed: %d\n", passed);
    fprintf(stderr, "Tests failed: %d\n", failed);
    if ( 0 == failed ) {
        status = JXTA_SUCCESS;
    } else {
        fprintf(stderr, "Some tests failed\n");
        status = JXTA_FAILED;
    }

FINAL_EXIT:
    jxta_log_using(NULL, NULL);
    jxta_log_file_close(log_f);
    jxta_log_selector_delete(log_s);

    jxta_terminate();

    return (JXTA_SUCCESS == status) ? 0 : 1;
}

/** 
 * A helper function that can be called from the main function if the test programs 
 * are run as standalone application.
 */
int main_test_function(const struct _funcs testfunc[], int argc, char **argv)
{
    int verbosity = 0;
    Jxta_log_file *log_f = NULL;
    Jxta_log_selector *log_s = NULL;
    char logselector[256];
    char *lfname = NULL;
    char *fname = NULL;
    int run, failed, passed;
    Jxta_boolean result;
    Jxta_status status;
    int optc;
    char **optv = argv;
    
    jxta_initialize();

    for( optc = 1; optc < argc; optc++ ) {
        char *opt = argv[optc];
        
        if( *opt != '-' ) {
            fprintf(stderr, "# %s - Invalid option.\n", argv[0]);
            status = JXTA_INVALID_ARGUMENT;
            goto FINAL_EXIT;
        }
        
        opt++;
        
        switch( *opt ) {
            case 'v' :
                verbosity++;
                break;
                
            case 'l' :
                lfname = strdup(argv[++optc]);
                break;

            case 'f' : 
                fname = strdup(argv[++optc]);
                break;
            
            default :
                fprintf(stderr, "# %s - Invalid option : %c.\n", argv[0], *opt );
        status = JXTA_INVALID_ARGUMENT;
        goto FINAL_EXIT;
        }
    }

    run = failed = passed = 0;

    if (verbosity > (sizeof(verbositys) / sizeof(const char *)) - 1) {
        verbosity = (sizeof(verbositys) / sizeof(const char *)) - 1;
    }

    strcpy(logselector, "*.");
    strcat(logselector, verbositys[verbosity]);
    strcat(logselector, "-fatal");

    fprintf(stderr, "Starting logger with selector : %s\n", logselector);
    log_s = jxta_log_selector_new_and_set(logselector, &status);
    if (NULL == log_s) {
        fprintf(stderr, "# %s - Failed to init default log selector.\n", argv[0]);
        status = JXTA_INVALID_ARGUMENT;
        goto FINAL_EXIT;
    }

    if (fname) {
        jxta_log_file_open(&log_f, fname);
        free(fname);
    } else {
        jxta_log_file_open(&log_f, "-");
    }

    jxta_log_using(jxta_log_file_append, log_f);
    jxta_log_file_attach_selector(log_f, log_s, NULL);

    if (NULL != lfname) {
        FILE *loggers = fopen(lfname, "r");

        if (NULL != loggers) {
            do {
                if (NULL == fgets(logselector, sizeof(logselector), loggers)) {
                    break;
                }

                fprintf(stderr, "Starting logger with selector : %s\n", logselector);

                log_s = jxta_log_selector_new_and_set(logselector, &status);
                if (NULL == log_s) {
                    fprintf(stderr, "# %s - Failed to init default log selector.\n", argv[0]);
                    status = JXTA_INVALID_ARGUMENT;
                    goto FINAL_EXIT;
                }

                jxta_log_file_attach_selector(log_f, log_s, NULL);
            } while (TRUE);

            fclose(loggers);
        }
    }

    jxta_log_file_attach_selector(log_f, log_s, NULL);

    if (lfname) {
        free(lfname);
    }

    result = run_testfunctions(testfunc, &run, &passed, &failed);
    fprintf(stderr, "Tests run:    %d\n", run);
    fprintf(stderr, "Tests passed: %d\n", passed);
    fprintf(stderr, "Tests failed: %d\n", failed);
    if (result) {
        status = JXTA_SUCCESS;
    } else {
        fprintf(stderr, "Some tests failed\n");
        status = JXTA_FAILED;
    }

FINAL_EXIT:
    jxta_log_using(NULL, NULL);
    jxta_log_file_close(log_f);
    jxta_log_selector_delete(log_s);

    jxta_terminate();

    return (JXTA_SUCCESS == status) ? 0 : 1;
}


/**
 * Implemenation of SendFunc
 */
Jxta_status JXTA_STDCALL sendFunction(void *cookie, const char *buf, size_t len, unsigned int flag)
{
    read_write_test_buffer *stream = (read_write_test_buffer *) cookie;

    memmove( &stream->buffer[stream->position], buf, len );

    stream->position += len;
    
    stream->flags = flag;

    return JXTA_SUCCESS;
}


/**
 * Implementation of WriteFunc
 */
Jxta_status JXTA_STDCALL writeFunction(void *cookie, const char *buf, size_t len)
{
    read_write_test_buffer *stream = (read_write_test_buffer *) cookie;
    
    memmove( &stream->buffer[stream->position], buf, len );

    stream->position += len;
    return JXTA_SUCCESS;
}


/**
 * Implemenation of ReadFunc
 */
Jxta_status JXTA_STDCALL readFromStreamFunction(void *cookie, char *buf, size_t len)
{
    read_write_test_buffer *stream = (read_write_test_buffer *) cookie;

    memmove( buf, &stream->buffer[stream->position], len );

    stream->position += len;
    return JXTA_SUCCESS;
}
