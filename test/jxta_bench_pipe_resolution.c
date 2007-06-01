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
 * $Id: jxta_bench_pipe_resolution.c,v 1.1 2005/10/14 06:12:11 mathieu Exp $
 */

/**
 * JXTA-C pipe resolution benchmark
 *
 * Created by Mathieu Jan (INRIA), <Mathieu.Jan@irisa.fr>
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

#include <apr_general.h>
#include <apr_time.h>

#include "jxta.h"
#include "jxta_debug.h"
#include "jxta_peer.h"
#include "jxta_peergroup.h"
#include "jxta_listener.h"
#include "jxta_rdv_service.h"
#include "jxta_pipe_service.h"
#include "jxta_pipe_adv.h"
#include "jxta_dr.h"

#define OPTIMIZED 1
#define GENERATE_PIPE_ADV 0

/**
 * Peer group used
 */
static Jxta_PG     * pg = NULL;

/**
 * Services used
 */
static Jxta_rdv_service  * rdv_service = NULL;
static Jxta_pipe_service * pipe_service = NULL;

/**
 * Test param
 */
static Jxta_boolean warmup = TRUE;
static Jxta_boolean master = FALSE;
static int current_resolution_count = 0;
static int specified_resolution_count = 0;

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
#if GENERATE_PIPE_ADV
static Jxta_id           * looked_pipe_id = NULL;
#endif
static Jxta_pipe_adv     * looked_pipe_adv = NULL;
static Jxta_pipe         * looked_pipe = NULL;

static Jxta_outputpipe *outputpipe = NULL;
static Jxta_listener *outgoing_listener = NULL;

static Jxta_inputpipe *inputpipe = NULL;
static Jxta_listener *incoming_listener = NULL;

/**
 * To store time
 */
apr_time_t t_begin;
apr_time_t t_end;

static const char * LOOKED_PIPE_ADV_FILENAME = "looked_pipe.adv";

/**
 * Forward definition to make the compilor happy
 **/
static void start_pipe_resolution_work (void);
static void compute_performance(apr_time_t t_begin, apr_time_t t_end);
static void end_test(void);

static void edge_rdv_listener (Jxta_object *obj, void *arg) {
  JString* string = NULL;
  Jxta_rdv_event* rdv_event = (Jxta_rdv_event*) obj;
 
  /** Get info from the event */
  int type = rdv_event->event;
  assert(jxta_id_to_jstring(rdv_event->pid,&string) == JXTA_SUCCESS);

  switch (type) {
  case JXTA_RDV_CONNECTED:
    printf ("Connected to a rdv peer=%s\n",jstring_get_string(string));

    if (master)
      start_pipe_resolution_work();

    break;
  case JXTA_RDV_FAILED:
    printf ("Failed connection to a rdv peer=%s\n",jstring_get_string(string));
    break;
  case JXTA_RDV_DISCONNECTED:
    printf ("Disconnect from the rdv peer=%s\n",jstring_get_string(string));
    break;
  default: 
    printf("Event not catched = %d\n", type);
  }

  JXTA_OBJECT_RELEASE(string);
}

static void start_pipe_resolution_work (void) {
  Jxta_status res = JXTA_SUCCESS;

  /** Trying to connect to the looked pipe **/
  t_begin = apr_time_now();
  res = jxta_pipe_service_connect (pipe_service, looked_pipe_adv, 1000000, NULL, outgoing_listener);
  if (res != JXTA_SUCCESS) {
    printf("Failed to connect to the looked pipe in start_resolution_work\n");
    end_test();
  }
}

/** Used to get the outputpipe **/
static void connect_listener (Jxta_object *obj, void *arg) {
  Jxta_pipe_connect_event* connect_event = (Jxta_pipe_connect_event*) obj;

  /** Yes we want to measure that too ! */
  jxta_pipe_connect_event_get_pipe(connect_event, &looked_pipe);
  jxta_pipe_get_outputpipe (looked_pipe, &outputpipe); 
#if DEBUG
  assert(outputpipe != NULL);
  printf("Connected to the looked pipe the %i time.\n", current_resolution_time);
#endif
    
  if (warmup) {
    current_resolution_count = specified_resolution_count;
    warmup = FALSE;

    printf("Warmup finished! Wait a bit ... (10 seconds)\n");
    apr_sleep(10 * 1000 * 1000);

    t_begin = apr_time_now();
  }

  if (current_resolution_count <= 0) {
    t_end = apr_time_now();
    compute_performance(t_begin, t_end);
    end_test();
  } else {
    Jxta_status res = JXTA_SUCCESS;

#if ! OPTIMIZED
    printf("Resolving pipe, remaining iter %i\n", current_resolution_count);
#endif
    res = jxta_pipe_service_connect (pipe_service, looked_pipe_adv, 1000000, NULL, outgoing_listener);
    if (res != JXTA_SUCCESS) {
      printf("Failed to connect to the looked pipe in start_resolution_work\n");
      end_test();
    }
    current_resolution_count--;
  }
}

/** End the test **/
static void end_test (void) {

#if ! OPTIMIZED
  jxta_log_file_attach_selector(log_file, NULL, &log_selector);
  jxta_log_selector_delete(log_selector);
  jxta_log_file_close(log_file);
#endif
  
  printf("Wait 10 seconds so that JXTA become idle\n");
  apr_sleep(10 * 1000 * 1000);

  jxta_module_stop((Jxta_module*) pg);
  JXTA_OBJECT_RELEASE(pg);
  
  jxta_terminate();
  exit(0);
}

static void compute_performance(apr_time_t t_begin, apr_time_t t_end)
{
  apr_interval_time_t t_result;
  float time = 0.;
  
  t_result = t_end - t_begin;
  time = ((float) t_result / specified_resolution_count);

  printf("Total time = %f. Pipe resolution time in usec = %f\n", (float) t_result, time);
}

static void init_pipe_resolution_work (void) 
{
  FILE *file;
#if GENERATE_PIPE_ADV
  JString *tmp = NULL;
  Jxta_PGID *pg_id = NULL;
  
  jxta_PG_get_GID(pg, &pg_id);
#endif

  /** Building the looked pipe adv **/
  looked_pipe_adv = jxta_pipe_adv_new();
#if ! GENERATE_PIPE_ADV
  file = fopen (LOOKED_PIPE_ADV_FILENAME, "r");
  jxta_pipe_adv_parse_file (looked_pipe_adv, file);
#else
  file = fopen (LOOKED_PIPE_ADV_FILENAME, "w");
  jxta_id_pipeid_new_1(&looked_pipe_id, (Jxta_id*) pg_id);
  jxta_id_to_jstring(looked_pipe_id, &tmp);
  jxta_pipe_adv_set_Id(looked_pipe_adv, jstring_get_string(tmp));
  jxta_pipe_adv_set_Type(looked_pipe_adv, JXTA_UNICAST_PIPE);
  jxta_pipe_adv_set_Name(looked_pipe_adv, "Looked");
  JXTA_OBJECT_RELEASE(tmp);
  JXTA_OBJECT_RELEASE(looked_pipe_id);
  jxta_pipe_adv_get_xml(looked_pipe_adv, &tmp);
  fprintf(file, "%s\n", jstring_get_string(tmp));
#endif

  fclose(file);

#if GENERATE_PIPE_ADV
  JXTA_OBJECT_RELEASE(pg_id);
  printf("The looked pipe adv has been generated, please put it in the directory of both master and slave. And set GENERATE_PIPE_ADV to 0\n");
  exit(0);
#endif

  if (master) {
    /** Add a listener to get output pipe **/
    outgoing_listener = jxta_listener_new ((Jxta_listener_func) connect_listener,
					   NULL,
					   1,
					   1);
    if (outgoing_listener != NULL) {
      jxta_listener_start(outgoing_listener);
    }
  } else {
    assert(jxta_pipe_service_timed_accept(pipe_service, looked_pipe_adv, 1000, &looked_pipe) == JXTA_SUCCESS);
    assert(jxta_pipe_get_inputpipe(looked_pipe, &inputpipe) == JXTA_SUCCESS);
    
    /** Adding the listener for incoming messages **/
    assert(jxta_inputpipe_add_listener(inputpipe, incoming_listener) == JXTA_SUCCESS);
  }
   
}

static void message_listener(Jxta_object * obj, void *arg)
{
  /** We only benchmark pipe resolution */
}

static Jxta_status init_jxta(void) {
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

  assert(jxta_PG_new_netpg (&pg) == JXTA_SUCCESS);

  return JXTA_SUCCESS;
}

static void print_usage(int argc, char **argv) {
  printf ("Syntax: %s resolution_count [-m|-s]\n", argv[0]);
  printf ("-m\t For master peer\n");
  printf ("-s\t For slave peer\n");
  exit(-1);
}

static Jxta_boolean process_args(int argc, char **argv) {
  if (argc != 3) {
    printf ("Missing arguments\n");
    print_usage(argc, argv);
    return JXTA_FAILED;
  }

  specified_resolution_count = atoi(argv[1]);
  current_resolution_count = specified_resolution_count;

  if (strcmp(argv[2], "-m") == 0) {
    master = TRUE;
  } else {
    master = FALSE;
  }

  return JXTA_SUCCESS;
}

int main (int argc, char **argv) {
  Jxta_listener* rdv_listener = NULL;

  assert(process_args(argc,argv) == JXTA_SUCCESS);
  assert(init_jxta() == JXTA_SUCCESS);

  /** Get useful services */
  jxta_PG_get_rendezvous_service (pg, &rdv_service);
  jxta_PG_get_pipe_service (pg, &pipe_service);

  /** Add a listener to rdv events */
  rdv_listener = jxta_listener_new ((Jxta_listener_func) edge_rdv_listener,
				    NULL,
				    1,
				    1);
  if (rdv_listener != NULL) {
    jxta_listener_start(rdv_listener);
    assert(jxta_rdv_service_add_event_listener (rdv_service, (char *) "test", (char *) "bench", rdv_listener) == JXTA_SUCCESS);
  }

  /** Creating listener for incoming messages (fake, as it will not be used) */
  incoming_listener = jxta_listener_new((Jxta_listener_func) message_listener, NULL, 1, 200);
  if (incoming_listener != NULL) {
    jxta_listener_start(incoming_listener);
  }
  
  init_pipe_resolution_work();

  /** Wait enough time so that the test has enough time to finish **/
  apr_sleep(300 * 1000 * 1000);
  
  return (int) TRUE;
}
