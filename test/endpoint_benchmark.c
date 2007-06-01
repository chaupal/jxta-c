/*
 * Copyright (c) 2004 Sun Microsystems, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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
 * $Id: endpoint_benchmark.c,v 1.1 2006/10/20 16:49:07 slowhog Exp $
 */

/**
 * JXTA-C bidirectionnal bandwidth benchmark (endpoint)
 *
 * Created by Mathieu Jan (INRIA), <Mathieu.Jan@irisa.fr>
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <apr_general.h>
#include <apr_time.h>

#include "jxta.h"
#include "jxta_debug.h"
#include "jxta_peer.h"
#include "jxta_peergroup.h"
#include "jxta_endpoint_service.h"
#include "jxta_rdv_service.h"
#include "jxta_dr.h"

#define OPTIMIZED 1

typedef Jxta_status(*test_init_t) (void);
/**
 * Peer group used
 */
static Jxta_PG *pg = NULL;

/**
 * Services used
 */
static Jxta_rdv_service *rdv_service = NULL;
static Jxta_endpoint_service *endpoint_service = NULL;

/**
 * To know in which case we are
 */
static Jxta_boolean sync_mode = FALSE;
static Jxta_boolean master = FALSE;

/**
 * Test param
 */
static Jxta_boolean warmup = TRUE;
static int specified_msg_count = 0;
static int specified_ntime_of_increase = 0;
static int specified_msg_size = 0;
static int remaining_msg_iter = 0;
static int current_msg_size = 0;
static int remaining_inc_iter = 0;

/**
 * Log if required
 */
#if ! OPTIMIZED
static Jxta_log_file *log_file;
static Jxta_log_selector *log_selector;
#endif

/** 
 * For the endpoint service
 */
static char *endpoint_dest_address = NULL;
static Jxta_endpoint_address *dest_address = NULL;
static const char *SERVICE_NAME = "s";

static void *ep_cookie;
static Jxta_callback_func func;
static apr_thread_mutex_t *cond_lock;
static apr_thread_cond_t *cond_var;

/**
 * To store time
 */
apr_time_t t_begin;
apr_time_t t_end;

static FILE *gnuplot_file = NULL;
static const char *RESULT_GNUPLOT_FILENAME = "results.dat";

/**
 * Forward definition to make the compilor happy
 **/
static Jxta_status JXTA_STDCALL message_listener(Jxta_object * obj, void *arg);
static Jxta_message *create_payload(void);
static Jxta_status send_message(Jxta_message * message, Jxta_boolean sync);
static void do_endpoint_work(test_init_t init_func, Jxta_callback_func cb);
static void compute_performance(apr_time_t t_begin, apr_time_t t_end, int msg_cnt);
static apr_status_t timedwait(apr_interval_time_t timeout);
static void move_along(void);

static void JXTA_STDCALL edge_rdv_listener(Jxta_object * obj, void *arg)
{
    Jxta_status rv;
    JString *string = NULL;
    Jxta_rdv_event *rdv_event = (Jxta_rdv_event *) obj;

  /** Get info from the event */
    int type = rdv_event->event;
    rv = jxta_id_to_jstring(rdv_event->pid, &string);
    assert(rv == JXTA_SUCCESS);

    switch (type) {
    case JXTA_RDV_CONNECTED:
        printf("Connected to a rdv peer=%s\n", jstring_get_string(string));
        move_along();
        break;
    case JXTA_RDV_FAILED:
        printf("Failed connection to a rdv peer=%s\n", jstring_get_string(string));
        break;
    case JXTA_RDV_DISCONNECTED:
        printf("Disconnect from the rdv peer=%s\n", jstring_get_string(string));
        break;
    default:
        printf("Event not catched = %d\n", type);
    }

    JXTA_OBJECT_RELEASE(string);
}

static Jxta_status JXTA_STDCALL test_proxy(Jxta_object * obj, void *arg)
{
    Jxta_message *message = (Jxta_message *) obj;

    if (warmup) {
        if (!master) {
            printf("Sending the warmup response...\n");
            send_message(message, TRUE);
            printf("Warmup finished!\n");
        }

        current_msg_size = specified_msg_size;
        warmup = FALSE;

        if (master) {
            move_along();
        }
        return JXTA_SUCCESS;
    } else {
        assert(func);
        return func(obj, arg);
    }
}

static void do_endpoint_work(test_init_t init_func, Jxta_callback_func cb)
{
    Jxta_status rv;
    Jxta_message *message = NULL;

    warmup = TRUE;
    func = cb;
    current_msg_size = 1;
    remaining_msg_iter = specified_msg_count;
    remaining_inc_iter = specified_ntime_of_increase;

    if (master) {
        printf("Producer send first endpoint message (warmup)\n");

        /* Launch the test by creating a message */
        message = create_payload();
        do {
            rv = send_message(message, TRUE);
            assert(rv == JXTA_SUCCESS);
            rv = timedwait(1000L * 1000);
        } while (APR_TIMEUP == rv);
        JXTA_OBJECT_RELEASE(message);
        warmup = FALSE;
        printf("Warmup finished! Wait a bit ... (5 seconds)\n");
        apr_sleep(5L * 1000 * 1000);

        init_func();
    }

    /* wait the test finished */
    timedwait(0);
    if (master) {
        printf("Wait for 5 seconds to calm the test\n");
        apr_sleep(5L * 1000 * 1000);
    }
}

/* Speed Test */
static Jxta_status speed_init()
{
    Jxta_message *new_message = NULL;

    while (remaining_inc_iter--) {
        new_message = create_payload();
        t_begin = apr_time_now();
        while (remaining_msg_iter--) {
            send_message(new_message, sync_mode);
        }
        t_end = apr_time_now();
        compute_performance(t_begin, t_end, specified_msg_count);
        remaining_msg_iter = specified_msg_count;
        current_msg_size = current_msg_size << 1;
    }
    JXTA_OBJECT_RELEASE(new_message);
    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL speed_test(Jxta_object * obj, void *arg)
{
    static int started = 0;

    if (master) {
        move_along();
    } else {
        if (!started) {
            started = 1;
            printf("Start running test for message size=%d\n", current_msg_size);
            t_begin = apr_time_now();
        }
        remaining_msg_iter--;
        if (remaining_msg_iter <= 0) {
            printf("Finished running test for message size=%d\n", current_msg_size);
            t_end = apr_time_now();
            compute_performance(t_begin, t_end, specified_msg_count);
            started = 0;
            remaining_inc_iter--;
            /* Go to the next message size */
            if (remaining_inc_iter <= 0) {
                apr_sleep(2000000L);    /* wait a bit for master to be ready for sync mode */
                send_message((Jxta_message *) obj, TRUE);
                move_along();
            } else {
                remaining_msg_iter = specified_msg_count;
                current_msg_size = current_msg_size << 1;
            }
        }
    }
    return JXTA_SUCCESS;
}

/* Ping Pong Test */
static void ignite_traffic(int msg_size)
{
    Jxta_message *new_message = NULL;

    current_msg_size = (0 >= msg_size) ? current_msg_size << 1 : msg_size;

    new_message = create_payload();
    t_begin = apr_time_now();
    send_message(new_message, sync_mode);
    remaining_msg_iter--;
    JXTA_OBJECT_RELEASE(new_message);
}

static Jxta_status ping_pong_init()
{
    ignite_traffic(specified_msg_size);
    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL ping_pong(Jxta_object * obj, void *arg)
{
    Jxta_message *message = (Jxta_message *) obj;

    if (remaining_msg_iter <= 0) {
        assert(0 == remaining_msg_iter);
        assert(master);
        /* only master will be here */
        t_end = apr_time_now();
        compute_performance(t_begin, t_end, 2 * specified_msg_count);
        remaining_inc_iter--;
        if (remaining_inc_iter <= 0) {
            JXTA_OBJECT_RELEASE(message);
            move_along();
            return JXTA_SUCCESS;
        } else {
            remaining_msg_iter = specified_msg_count;
            ignite_traffic(0);
            return JXTA_SUCCESS;
        }
    }

    assert(remaining_msg_iter > 0);
    send_message(message, sync_mode);
    remaining_msg_iter--;

    if (!master && remaining_msg_iter <= 0) {
        printf("Finished running test for message size=%d\n", current_msg_size);
        remaining_inc_iter--;
        /* Go to the next message size */
        if (remaining_inc_iter <= 0) {
            move_along();
            return JXTA_SUCCESS;
        } else {
            remaining_msg_iter = specified_msg_count;
            current_msg_size = current_msg_size << 1;
        }
    }
    return JXTA_SUCCESS;
}

static Jxta_status send_message(Jxta_message * message, Jxta_boolean sync)
{
    Jxta_status status = JXTA_SUCCESS;
#if ! OPTIMIZED
    printf("Sending an endpoint message of size %i, of iter %i, remaining increase %i\n", current_msg_size,
           remaining_msg_iter, remaining_inc_iter);
#endif
    if (sync) {
        status = jxta_endpoint_service_send_sync(pg, endpoint_service, message, dest_address);
    } else {
        status = jxta_endpoint_service_send_async(pg, endpoint_service, message, dest_address);
    }

    return status;
}

static Jxta_message *create_payload(void)
{
    Jxta_message *msg = jxta_message_new();
    Jxta_message_element *el = NULL;
    char *payload = NULL;

    payload = malloc(current_msg_size * sizeof(char));
    payload = (char *) memset((void *) payload, 0x61, current_msg_size);

    el = jxta_message_element_new_1((char *) "P", NULL, payload, current_msg_size, NULL);
    jxta_message_add_element(msg, el);

    JXTA_OBJECT_RELEASE(el);
    free(payload);

    return msg;
}

static void move_along(void)
{
    apr_thread_mutex_lock(cond_lock);
    apr_thread_cond_signal(cond_var);
    apr_thread_mutex_unlock(cond_lock);
}

static apr_status_t timedwait(apr_interval_time_t timeout)
{
    apr_status_t rv = APR_SUCCESS;

    apr_thread_mutex_lock(cond_lock);
    if (timeout > 0) {
        rv = apr_thread_cond_timedwait(cond_var, cond_lock, timeout);
    } else {
        apr_thread_cond_wait(cond_var, cond_lock);
    }
    apr_thread_mutex_unlock(cond_lock);
    return rv;
}

static void compute_performance(apr_time_t t_begin, apr_time_t t_end, int msg_cnt)
{
    apr_interval_time_t t_result;
    float message_size_k, latency, throughput = 0;

    t_result = t_end - t_begin;

    message_size_k = (current_msg_size) / 1024.;
    latency = ((float) t_result / msg_cnt);
                                                            /** in usec **/
    printf("Message size = %i, latency in usec=%f\t", current_msg_size, latency);
    throughput = (message_size_k / (latency / (1000. * 1000.))) / 1024.;
                                                                       /** in Mo/s **/
    printf("thoughput in Mo/s=%f\n", throughput);
    fprintf(gnuplot_file, "%i\t%f\t%f\n", current_msg_size, latency, throughput);
}

static Jxta_status init_jxta(void)
{
    Jxta_status rv;
    apr_pool_t *pool;

#if ! OPTIMIZED
    Jxta_status status = JXTA_SUCCESS;
#endif

    jxta_initialize();

    /* JXTA log */
#if ! OPTIMIZED
    log_selector = jxta_log_selector_new_and_set("*.*", &status);
    if (NULL == log_selector || status != JXTA_SUCCESS) {
        fprintf(stderr, "# Failed to init JXTA log selector.\n");
        return JXTA_FAILED;
    }

    jxta_log_file_open(&log_file, "jxta.log");
    jxta_log_using(jxta_log_file_append, log_file);
    jxta_log_file_attach_selector(log_file, log_selector, NULL);
#endif
    rv = apr_pool_create(&pool, NULL);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_thread_mutex_create(&cond_lock, APR_THREAD_MUTEX_DEFAULT, pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_thread_cond_create(&cond_var, pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = jxta_PG_new_netpg(&pg);
    return rv;
}

static void print_usage(int argc, char **argv)
{
    printf("Syntax: %s msg_size msg_count ntime_of_increase [-sync|-async] [-m|-s <server name>]\n", argv[0]);
    printf("-m\t For producer peer\n");
    printf("-s\t For reflector peer\n");
    exit(-1);
}

static Jxta_boolean process_args(int argc, char **argv)
{
    if (7 != argc) {
        printf("Missing arguments\n");
        print_usage(argc, argv);
        return JXTA_FAILED;
    }

    specified_msg_size = atoi(argv[1]);
    if (specified_msg_size <= 0) {
        specified_msg_size = 16;
    }

    specified_msg_count = atoi(argv[2]);
    if (specified_msg_count <= 0) {
        specified_msg_count = 1;
    }

    specified_ntime_of_increase = atoi(argv[3]);
    if (specified_ntime_of_increase < 0) {
        specified_ntime_of_increase = 0;
    }

    sync_mode = (strcmp(argv[4], "-sync") == 0);

    master = (strcmp(argv[5], "-m") == 0);

    endpoint_dest_address = argv[6];

    return JXTA_SUCCESS;
}

int main(int argc, char **argv)
{
    Jxta_status rv;
    Jxta_listener *rdv_listener = NULL;

    assert(process_args(argc, argv) == JXTA_SUCCESS);
    assert(init_jxta() == JXTA_SUCCESS);

    /* Opening result file */
    gnuplot_file = fopen(RESULT_GNUPLOT_FILENAME, "w");

    /* Get useful services */
    jxta_PG_get_rendezvous_service(pg, &rdv_service);
    jxta_PG_get_endpoint_service(pg, &endpoint_service);

    /* Add a listener to rdv events */
    rdv_listener = jxta_listener_new((Jxta_listener_func) edge_rdv_listener, NULL, 1, 1);
    if (rdv_listener != NULL) {
        jxta_listener_start(rdv_listener);
        rv = jxta_rdv_service_add_event_listener(rdv_service, (char *) "test", (char *) "bench", rdv_listener);
        assert(rv == JXTA_SUCCESS);
    }

    if (APR_TIMEUP == timedwait(60L * 1000 * 1000)) {
        printf("Failed to get RDV lease after a minute.\n");
        return -1;
    }

    jxta_PG_get_recipient_addr(pg, "tcp", endpoint_dest_address, SERVICE_NAME, "p", &dest_address);
    rv = jxta_PG_add_recipient(pg, &ep_cookie, SERVICE_NAME, "p", test_proxy, NULL);

    printf("** Conducting speed test ...\n");
    do_endpoint_work(speed_init, speed_test);

    printf("** Conducting ping pong test ...\n");
    do_endpoint_work(ping_pong_init, ping_pong);

    /* give it a little time to finish sending last msg */
    apr_sleep(5L * 1000 * 1000);
    jxta_PG_remove_recipient(pg, ep_cookie);
    fclose(gnuplot_file);

#if ! OPTIMIZED
    jxta_log_file_attach_selector(log_file, NULL, &log_selector);
    jxta_log_selector_delete(log_selector);
    jxta_log_file_close(log_file);
#endif

    jxta_module_stop((Jxta_module *) pg);
    JXTA_OBJECT_RELEASE(pg);

    jxta_terminate();
    return (int) TRUE;
}

/* vim: set ts=4 sw=4 et tw=130 cin: */
