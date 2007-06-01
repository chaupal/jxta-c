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
 * $Id: jxta_rdvclient_listener_test.c,v 1.5 2005/07/24 11:33:00 mathieu Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <apr_general.h>
#include "apr_time.h"

#include "jxta.h"
#include "jxta_debug.h"
#include "jxta_peer.h"
#include "jxta_peergroup.h"
#include "jxta_listener.h"
#include "jxta_endpoint_service.h"
#include "jxta_rdv_service.h"

#define DEBUG 1

#if DEBUG
static Jxta_log_file *log_file;
static Jxta_log_selector *log_selector;
#endif


static void rdvclient_rdv_listener (Jxta_object *obj, void *arg) {
  Jxta_status res = 0;
  JString* string = NULL;
  Jxta_rdv_event* rdv_event = (Jxta_rdv_event*) obj;

  /** Get info from the event */
  int type = rdv_event->event;
  res = jxta_id_to_jstring(rdv_event->pid,&string);

  switch (type) {
  case JXTA_RDV_CONNECTED:
#if DEBUG
    printf ("Connected to rdv peer=%s\n",jstring_get_string(string));
#endif
    break;
  case JXTA_RDV_FAILED:
#if DEBUG
    printf ("Failed connection to rdv peer=%s\n",jstring_get_string(string));
#endif
    break;
  case JXTA_RDV_DISCONNECTED:
#if DEBUG
    printf ("Disconnect from rdv peer=%s\n",jstring_get_string(string));
#endif
    break;
  case JXTA_RDV_RECONNECTED:
#if DEBUG
    printf ("Reconnect from rdv peer=%s\n",jstring_get_string(string));
#endif
    break;
  default: 
#if DEBUG
    printf("Event not catched = %d\n", type);
#endif
  }
}

int jxta_rdvclient_listener_run (int argc, char **argv) {
  Jxta_PG     * pg = NULL;
  Jxta_rdv_service  * rdv = NULL;
  Jxta_listener* listener = NULL;
  Jxta_status res = JXTA_SUCCESS;

  if (argc != 1) {
    printf ("Syntax: %s\n", argv[0]);
    exit (0);
  }

  /** Start log */
#if DEBUG
  log_selector = jxta_log_selector_new_and_set("*.*", &res);
  if (NULL == log_selector || res != JXTA_SUCCESS) {
    fprintf(stderr, "# Failed to init JXTA log selector.\n");
    return JXTA_FAILED;
  }
  jxta_log_file_open(&log_file, "jxta.log");
  jxta_log_using(jxta_log_file_append, log_file);
  jxta_log_file_attach_selector(log_file, log_selector, NULL);
#endif

  /** Start JXTA */
  assert(jxta_PG_new_netpg (&pg) == JXTA_SUCCESS);

  /** Get useful services */
  jxta_PG_get_rendezvous_service (pg, &rdv);

  /** Add a listener to rdv events */
  listener = jxta_listener_new ((Jxta_listener_func) rdvclient_rdv_listener,
				NULL,
				1,
				1);
  if (listener != NULL) {
    jxta_listener_start(listener);
    res = jxta_rdv_service_add_event_listener (rdv, (char *) "test", (char *) "edge", listener);
    if (res != JXTA_SUCCESS) {
      printf("Failed to add an event listener to rdv events");
    }
  }
  printf("wait for 20 sec for RDV event connection\n");
  apr_sleep (20 * 1000 * 1000);

#if DEBUG
  jxta_log_file_attach_selector(log_file, NULL, &log_selector);
  jxta_log_selector_delete(log_selector);
  jxta_log_file_close(log_file);
#endif

  jxta_module_stop((Jxta_module*) pg);
  JXTA_OBJECT_RELEASE(pg);

  return 0;
}


#ifdef STANDALONE
int main (int argc, char **argv) {
    int rv;

  jxta_initialize();
  rv = jxta_rdvclient_listener_run(argc, argv);
  jxta_terminate();
  return rv;
}
#endif
