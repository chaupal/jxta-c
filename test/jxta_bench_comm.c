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
 * $Id: jxta_bench_comm.c,v 1.8 2005/09/07 19:10:18 slowhog Exp $
 */

/**
 * JXTA-C bidirectionnal bandwidth benchmark (endpoint & pipe)
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
#include "jxta_listener.h"
#include "jxta_endpoint_service.h"
#include "jxta_rdv_service.h"
#include "jxta_pipe_service.h"
#include "jxta_pipe_adv.h"
#include "jxta_dr.h"

#define OPTIMIZED 1
#define GENERATE_PIPE_ADV 0

/**
 * Peer group used
 */
static Jxta_PG *pg = NULL;

/**
 * Services used
 */
static Jxta_rdv_service *rdv_service = NULL;
static Jxta_pipe_service *pipe_service = NULL;
static Jxta_endpoint_service *endpoint_service = NULL;

/**
 * To know in which case we are
 */
static Jxta_boolean jxta_pipe = FALSE;
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

/**
 * Log if required
 */
#if ! OPTIMIZED
static Jxta_log_file *log_file;
static Jxta_log_selector *log_selector;
#endif

/**
 * For the pipe service
 */
/** Producer pipe related stuff */
#if GENERATE_PIPE_ADV
static Jxta_id *producer_pipe_id = NULL;
#endif
static Jxta_pipe_adv *producer_pipe_adv = NULL;
static Jxta_pipe *producer_pipe = NULL;

/** Reflector pipe related stuff */
#if GENERATE_PIPE_ADV
static Jxta_id *reflector_pipe_id = NULL;
#endif
static Jxta_pipe_adv *reflector_pipe_adv = NULL;
static Jxta_pipe *reflector_pipe = NULL;

static Jxta_inputpipe *inputpipe = NULL;
static Jxta_outputpipe *outputpipe = NULL;
static Jxta_listener *incoming_listener = NULL;
static Jxta_listener *outgoing_listener = NULL;

/** 
 * For the endpoint service
 */
static char *endpoint_dest_address = NULL;
static Jxta_endpoint_address *dest_address = NULL;
static const char *SERVICE_NAME = "s";

/**
 * To store time
 */
apr_time_t t_begin;
apr_time_t t_end;

static FILE *gnuplot_file = NULL;
static const char *RESULT_GNUPLOT_FILENAME = "results.dat";

static const char *PRODUCER_PIPE_ADV_FILENAME = "producer_pipe.adv";
static const char *REFLECTOR_PIPE_ADV_FILENAME = "reflector_pipe.adv";

/**
 * Forward definition to make the compilor happy
 **/
static void message_listener(Jxta_object * obj, void *arg);
static Jxta_message *create_payload(void);
static Jxta_status send_message(Jxta_message * message);
static void start_pipe_work(void);
static void start_endpoint_work(void);
static void compute_performance(apr_time_t t_begin, apr_time_t t_end);
static void end_test(void);

static void edge_rdv_listener(Jxta_object * obj, void *arg)
{
    JString *string = NULL;
    Jxta_rdv_event *rdv_event = (Jxta_rdv_event *) obj;

  /** Get info from the event */
    int type = rdv_event->event;
    assert(jxta_id_to_jstring(rdv_event->pid, &string) == JXTA_SUCCESS);

    switch (type) {
    case JXTA_RDV_CONNECTED:
        printf("Connected to a rdv peer=%s\n", jstring_get_string(string));

        if (jxta_pipe) {
            start_pipe_work();
        } else {
            start_endpoint_work();
        }

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

static void start_pipe_work(void)
{
    if (master) {
    /** Getting the inputpipe of the producer **/
        assert(jxta_pipe_service_timed_accept(pipe_service, producer_pipe_adv, 1000, &producer_pipe) == JXTA_SUCCESS);
        assert(jxta_pipe_get_inputpipe(producer_pipe, &inputpipe) == JXTA_SUCCESS);

    /** Adding the listener for incoming messages **/
        assert(jxta_inputpipe_add_listener(inputpipe, incoming_listener) == JXTA_SUCCESS);

    /** Trying to connect to the reflector pipe **/
        assert(jxta_pipe_service_connect(pipe_service, reflector_pipe_adv, 10000, NULL, outgoing_listener) == JXTA_SUCCESS);
    } else {
    /** Getting the inputpipe of the reflector **/
        assert(jxta_pipe_service_timed_accept(pipe_service, reflector_pipe_adv, 1000, &reflector_pipe) == JXTA_SUCCESS);
        assert(jxta_pipe_get_inputpipe(reflector_pipe, &inputpipe) == JXTA_SUCCESS);

    /** Adding the listener for incoming messages **/
        assert(jxta_inputpipe_add_listener(inputpipe, incoming_listener) == JXTA_SUCCESS);
    }
}

static void start_endpoint_work(void)
{
    Jxta_message *message = NULL;

    if (master) {
        printf("Producer send first endpoint message (warmup)\n");

        current_msg_size = 1;
    /** Launch the test by creating a message **/
        message = create_payload();
        assert(send_message(message) == JXTA_SUCCESS);
        remaining_msg_iter--;

        JXTA_OBJECT_RELEASE(message);
    }
}

/** Used to get the outputpipe **/
static void connect_listener(Jxta_object * obj, void *arg)
{
    Jxta_pipe_connect_event *connect_event = (Jxta_pipe_connect_event *) obj;
    Jxta_message *message = NULL;

    if (master) {
        jxta_pipe_connect_event_get_pipe(connect_event, &reflector_pipe);
        jxta_pipe_get_outputpipe(reflector_pipe, &outputpipe);
        assert(outputpipe != NULL);

        printf("Connected to the reflector\n");
    } else {
        jxta_pipe_connect_event_get_pipe(connect_event, &producer_pipe);
        jxta_pipe_get_outputpipe(producer_pipe, &outputpipe);
        assert(outputpipe != NULL);

        printf("Connected to the producer\n");
    }

  /** Launch the test by creating a message **/
  /** Even the reflector creates a message to avoid a global variable */
    message = create_payload();
    send_message(message);
    JXTA_OBJECT_RELEASE(message);

    if (!master) {
        remaining_msg_iter = specified_msg_count;
        current_msg_size = 1;
        printf("Warmup finished!\n");
    }
}

static void message_listener(Jxta_object * obj, void *arg)
{
    Jxta_message *message = (Jxta_message *) obj;

    if (warmup) {
        remaining_msg_iter = specified_msg_count;
        current_msg_size = specified_msg_size;
        warmup = FALSE;

        if (master) {
            Jxta_message *new_message = NULL;

            printf("Warmup finished! Wait a bit ... (10 seconds)\n");
            apr_sleep(10 * 1000 * 1000);

      /** Launch the test by creating a message **/
            new_message = create_payload();

      /** Send the message */
            t_begin = apr_time_now();
            send_message(new_message);
            JXTA_OBJECT_RELEASE(new_message);
        } else {

            printf("Wait a bit before sending the warmup response... (10 seconds)\n");
            apr_sleep(10 * 1000 * 1000);

            if (jxta_pipe) {
    /** Trying to connect to the producer pipe **/
                printf("Connecting to the producer\n");
                assert(jxta_pipe_service_connect(pipe_service, producer_pipe_adv, 10000, NULL, outgoing_listener) ==
                       JXTA_SUCCESS);
                return;
            }

            send_message(message);

            remaining_msg_iter = specified_msg_count;
            current_msg_size = specified_msg_size;
            printf("Warump finished!\n");
        }
        return;
    }

    if (remaining_msg_iter <= 0) {
                                 /** and we are the producer!!! */
        t_end = apr_time_now();
        compute_performance(t_begin, t_end);

        remaining_msg_iter = specified_msg_count;

    /** Go to the next message size **/
        if (specified_ntime_of_increase <= 0) {
            JXTA_OBJECT_RELEASE(message);
            end_test();
        } else {
            Jxta_message *new_message = NULL;

            remaining_msg_iter = specified_msg_count;
            specified_ntime_of_increase--;
            current_msg_size = current_msg_size * 2;

            new_message = create_payload();
      /** Soon we are going to send the message **/
            t_begin = apr_time_now();
            send_message(new_message);
            JXTA_OBJECT_RELEASE(new_message);
            return;
        }
    /** Soon we are going to send the message **/
        t_begin = apr_time_now();
    }

  /** Send the message */
    send_message(message);

    if (!master && remaining_msg_iter <= 0) {
        printf("Finished running test for message size=%d\n", current_msg_size);
        remaining_msg_iter = specified_msg_count;
      /** Go to the next message size **/
        if (specified_ntime_of_increase <= 0) {
            JXTA_OBJECT_RELEASE(message);
            end_test();
        } else {
            remaining_msg_iter = specified_msg_count;
            specified_ntime_of_increase--;
            current_msg_size = current_msg_size * 2;
        }
    }
}

static Jxta_status send_message(Jxta_message * message)
{
    Jxta_status status = JXTA_SUCCESS;
    if (jxta_pipe) {
#if ! OPTIMIZED
        printf("Sending a pipe message of size %i, of iter %i, remaining increase %i\n", current_msg_size, remaining_msg_iter,
               specified_ntime_of_increase);
#endif
        status = jxta_outputpipe_send(outputpipe, message);
    } else {
#if ! OPTIMIZED
        printf("Sending an endpoint message of size %i, of iter %i, remaining increase %i\n", current_msg_size,
               remaining_msg_iter, specified_ntime_of_increase);
#endif
        status = jxta_endpoint_service_send(pg, endpoint_service, message, dest_address);
    }

    remaining_msg_iter--;
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

/** End the test **/
static void end_test(void)
{
    if (master)
        fclose(gnuplot_file);

#if ! OPTIMIZED
    jxta_log_file_attach_selector(log_file, NULL, &log_selector);
    jxta_log_selector_delete(log_selector);
    jxta_log_file_close(log_file);
#endif

    printf("Wait 10 seconds so that JXTA become idle\n");
    apr_sleep(10 * 1000 * 1000);

    jxta_module_stop((Jxta_module *) pg);
    JXTA_OBJECT_RELEASE(pg);

    jxta_terminate();
    exit(0);
}

static void compute_performance(apr_time_t t_begin, apr_time_t t_end)
{
    apr_interval_time_t t_result;
    float message_size_k, latency, throughput = 0;

    t_result = t_end - t_begin;

    message_size_k = (current_msg_size) / 1024.;
    latency = ((float) t_result / (2 * specified_msg_count));
                                                            /** in usec **/
    printf("Message size = %i, latency in usec=%f\t", current_msg_size, latency);
    throughput = (message_size_k / (latency / (1000. * 1000.))) / 1024.;
                                                                       /** in Mo/s **/
    printf("thoughput in Mo/s=%f\n", throughput);
    fprintf(gnuplot_file, "%i\t%f\t%f\n", current_msg_size, latency, throughput);
}

static void init_pipe_work(void)
{
    FILE *file;
#if GENERATE_PIPE_ADV
    JString *tmp = NULL;
    Jxta_PGID *pg_id = NULL;

    jxta_PG_get_GID(pg, &pg_id);
#endif

  /** Add a listener to get output pipe **/
    outgoing_listener = jxta_listener_new((Jxta_listener_func) connect_listener, NULL, 1, 1);
    if (outgoing_listener != NULL) {
        jxta_listener_start(outgoing_listener);
    }

  /** Building the producer pipe adv **/
    producer_pipe_adv = jxta_pipe_adv_new();
#if ! GENERATE_PIPE_ADV
    file = fopen(PRODUCER_PIPE_ADV_FILENAME, "r");
    jxta_pipe_adv_parse_file(producer_pipe_adv, file);
#else
    file = fopen(PRODUCER_PIPE_ADV_FILENAME, "w");
    jxta_id_pipeid_new_1(&producer_pipe_id, (Jxta_id *) pg_id);
    jxta_id_to_jstring(producer_pipe_id, &tmp);
    jxta_pipe_adv_set_Id(producer_pipe_adv, jstring_get_string(tmp));
    jxta_pipe_adv_set_Type(producer_pipe_adv, JXTA_UNICAST_PIPE);
    jxta_pipe_adv_set_Name(producer_pipe_adv, "Reflector");
    JXTA_OBJECT_RELEASE(tmp);
    JXTA_OBJECT_RELEASE(producer_pipe_id);
    jxta_pipe_adv_get_xml(producer_pipe_adv, &tmp);
    fprintf(file, "%s\n", jstring_get_string(tmp));
#endif
    fclose(file);

  /** Building the reflector pipe adv **/
    reflector_pipe_adv = jxta_pipe_adv_new();
#if ! GENERATE_PIPE_ADV
    file = fopen(REFLECTOR_PIPE_ADV_FILENAME, "r");
    jxta_pipe_adv_parse_file(reflector_pipe_adv, file);
#else
    file = fopen(REFLECTOR_PIPE_ADV_FILENAME, "w");
    jxta_id_pipeid_new_1(&reflector_pipe_id, (Jxta_id *) pg_id);
    jxta_id_to_jstring(reflector_pipe_id, &tmp);
    jxta_pipe_adv_set_Id(reflector_pipe_adv, jstring_get_string(tmp));
    jxta_pipe_adv_set_Type(reflector_pipe_adv, JXTA_UNICAST_PIPE);
    jxta_pipe_adv_set_Name(reflector_pipe_adv, "Reflector");
    JXTA_OBJECT_RELEASE(tmp);
    JXTA_OBJECT_RELEASE(reflector_pipe_id);
    jxta_pipe_adv_get_xml(reflector_pipe_adv, &tmp);
    fprintf(file, "%s\n", jstring_get_string(tmp));
#endif

    fclose(file);

#if GENERATE_PIPE_ADV
    JXTA_OBJECT_RELEASE(pg_id);
    printf
        ("2 .adv have been generated, please put them in the 2 directories used by the producer and the reflector. And set GENERATE_PIPE_ADV to 0\n");
    exit(0);
#endif
}

static void init_endpoint_work(void)
{
    dest_address = jxta_endpoint_address_new2((char *) "tcp", endpoint_dest_address, (char *) SERVICE_NAME, (char *) "p");

    assert(jxta_endpoint_service_add_listener(endpoint_service, SERVICE_NAME, (char *) "p", incoming_listener) == JXTA_SUCCESS);
}

static Jxta_status init_jxta(void)
{
#if ! OPTIMIZED
    Jxta_status status = JXTA_SUCCESS;
#endif

    jxta_initialize();

  /** JXTA log **/
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

    assert(jxta_PG_new_netpg(&pg) == JXTA_SUCCESS);

    return JXTA_SUCCESS;
}

static void print_usage(int argc, char **argv)
{
    printf("Syntax: %s msg_size msg_count ntime_of_increase [-pipe|-endpoint] [-m|-s <server name>]\n", argv[0]);
    printf("-m\t For producer peer\n");
    printf("-s\t For reflector peer\n");
    exit(-1);
}

static Jxta_boolean process_args(int argc, char **argv)
{
    if (argc < 6 || argc > 7) {
        printf("Missing arguments\n");
        print_usage(argc, argv);
        return JXTA_FAILED;
    }

    specified_msg_size = atoi(argv[1]);
    specified_msg_count = atoi(argv[2]);
    remaining_msg_iter = specified_msg_count;
    specified_ntime_of_increase = atoi(argv[3]);

    if (strcmp(argv[4], "-pipe") == 0) {
        jxta_pipe = TRUE;
    } else {
        jxta_pipe = FALSE;
    }

    if (strcmp(argv[5], "-m") == 0) {
        master = TRUE;
    } else {
        master = FALSE;
    }

    if (!jxta_pipe) {
        endpoint_dest_address = argv[6];
    }

    return JXTA_SUCCESS;
}

int main(int argc, char **argv)
{
    Jxta_listener *rdv_listener = NULL;

    assert(process_args(argc, argv) == JXTA_SUCCESS);
    assert(init_jxta() == JXTA_SUCCESS);

  /** Opening result file **/
    if (master)
        gnuplot_file = fopen(RESULT_GNUPLOT_FILENAME, "w");

  /** Get useful services */
    jxta_PG_get_rendezvous_service(pg, &rdv_service);
    if (jxta_pipe) {
        jxta_PG_get_pipe_service(pg, &pipe_service);
    } else {
        jxta_PG_get_endpoint_service(pg, &endpoint_service);
    }

  /** Add a listener to rdv events */
    rdv_listener = jxta_listener_new((Jxta_listener_func) edge_rdv_listener, NULL, 1, 1);
    if (rdv_listener != NULL) {
        jxta_listener_start(rdv_listener);
        assert(jxta_rdv_service_add_event_listener(rdv_service, (char *) "test", (char *) "bench", rdv_listener) == JXTA_SUCCESS);
    }

  /** Creating listener for incoming messages */
    incoming_listener = jxta_listener_new((Jxta_listener_func) message_listener, NULL, 1, 200);
    if (incoming_listener != NULL) {
        jxta_listener_start(incoming_listener);
    }

    if (jxta_pipe) {
        init_pipe_work();
    } else {
        init_endpoint_work();
    }

  /** Wait enough time so that the test has enough time to finish **/
    apr_sleep(300 * 1000 * 1000);

    return (int) TRUE;
}

/* vim: set ts=4 sw=4 et tw=130 cin: */
