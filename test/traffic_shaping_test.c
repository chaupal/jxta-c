/* 
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id$
 */

#include <stdio.h>

#include "jxta.h"

#include "unittest_jxta_func.h"
#include "jxta_traffic_shaping_priv.h"
#include "trailing_average.h"

typedef struct msg_t {
Jxta_time start;
Jxta_time stop;
long interval;
long size;
} Msg_t;

typedef struct msg_size_t {
long time;
long size;
} Msg_size_t;


static void send_msg(Jxta_traffic_shaping *t, long size);

static Jxta_time test_started;

static Msg_size_t msg_sizes_times[] = {
                { 0, 100000},
                { 8000, 100},
                { 5000, 100},
                { 5000, 100},
                { 500, 100000},
                { 3000, 300},
                { 2000, 200 },
                { 6000, 14},
                { 11000, 100},
                { 16000, 300},
                { 5000, 200 },
                { 5000, 1400},
                { 2000, 100000},
                { 6000, 300},
                { 1000, 200 },
                { 5000, 14},
                { 2000, 100},
                { 6000, 300},
                { 1000, 200 },
                { 7000, 1400},
/*                { 5000, 100000},
                { 9000, 300},
                { 9000, 200 },
                { 6000, 14},
                { 11000, 100},
                { 16000, 300},
                { 5000, 200 },
                { 5000, 1400},
                { 2000, 100000},
                { 6000, 300},
                { 1000, 200 },
                { 5000, 14},
                { 2000, 100},
                { 6000, 300},
                { 1000, 200 },
*/                { 0, 0 }
};


#define PENDING 3
#define TIME_RANGE PENDING * PENDING
#define P_DEBUG 1

typedef struct times {
    int index;
    Jxta_time start;
    Jxta_time expires;
} dyn_times_t;

static dyn_times_t *dyn_times[TIME_RANGE];

TrailingAverage *trailing_average;

const char *test_traffic_shaping_construction(void) {

    int i=0;
    Jxta_traffic_shaping *t;
    Jxta_log_selector *log_s;
    Jxta_status rv;
    Jxta_log_file *log_f;
    char logselector[256];
    char *lfname = NULL;
    char *fname = NULL;

    trailing_average = trailing_average_new(60);

    jxta_initialize();

    rv = jxta_log_initialize();
    if (JXTA_SUCCESS != rv)
        return NULL;

    log_s = jxta_log_selector_new_and_set("*.info", &rv);
    if (JXTA_SUCCESS != rv) {
        printf("Failed at line %u\n", __LINE__);
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

                log_s = jxta_log_selector_new_and_set(logselector, &rv);
                if (NULL == log_s) {
                    fprintf(stderr, "# - Failed to init default log selector.\n");
                    exit(-1);
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

    dyn_times[TIME_RANGE] = NULL;

    t = traffic_shaping_new();

    traffic_shaping_set_time(t, 2000);

    traffic_shaping_set_size(t, 300000);

    traffic_shaping_set_interval(t, 2);

    traffic_shaping_set_frame(t, 10);

    traffic_shaping_set_look_ahead(t, 30);

    traffic_shaping_set_reserve(t, 15);

    traffic_shaping_init(t);

    /* create enough entries for time ranges */

    for (i=0; i< TIME_RANGE; i++) {
        dyn_times_t *dyn_times_ptr=NULL;

        dyn_times_ptr = (dyn_times_t *) calloc(1, sizeof (dyn_times_t));
        dyn_times[i] = dyn_times_ptr ;
        dyn_times_ptr->index = i;
    }

    test_started = jpr_time_now();
    for (i=0; msg_sizes_times[i].size != 0; i++) {

        printf ("********************************* \n");
        printf ("wait for %ld seconds\n", msg_sizes_times[i].time/1000);
        printf ("********************************* \n\n");

        apr_sleep(msg_sizes_times[i].time * 1000);

        send_msg(t, msg_sizes_times[i].size);
        trailing_average_inc(trailing_average);
        printf("trailing average: %d\n", trailing_average_get(trailing_average));
    }
    for (i=0; i< TIME_RANGE; i++) {
        free(dyn_times[i]);
    }

    printf("Done\n");

    return NULL;
}

static void send_msg(Jxta_traffic_shaping *t, long size)
{
    int m_interval;
    Jxta_time now;
    Jxta_boolean look_ahead_update = TRUE;


    now = jpr_time_now();
    m_interval = ((now - test_started)/1000);

    traffic_shaping_lock(t);

    if (traffic_shaping_check_size(t, size, size, &look_ahead_update), TRUE) {
        printf ("--------------------------------------- \n");
        printf("Have enough to send the message\n");
        printf ("--------------------------------------- \n");
    } else {
        printf ("--------------------------------------- \n");
        printf("Don't have enough to send the message\n");
        printf ("--------------------------------------- \n");
    }
    traffic_shaping_unlock(t);
}


/* vi: set ts=4 sw=4 tw=130 et: */

static struct _funcs traffic_shaping_test_funcs[] = {

    {*test_traffic_shaping_construction, "Construction of traffic control objects"},

    {NULL, "null"}
};

/**
* Run the unit tests for the jxta_pa test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_traffic_shaping_tests(int *tests_run, int *tests_passed, int *tests_failed)
{
    return run_testfunctions(traffic_shaping_test_funcs, tests_run, tests_passed, tests_failed);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{

    return main_test_function(traffic_shaping_test_funcs, argc, argv);
}
#endif
