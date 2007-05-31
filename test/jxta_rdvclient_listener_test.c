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
 * $Id: jxta_rdvclient_listener_test.c,v 1.2 2004/10/07 05:54:58 tra Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include <apr_general.h>
#include "apr_time.h"

#include "jpr/jpr_thread.h"

#include "jxta.h"
#include "jxta_debug.h"
#include "jxta_peer.h"
#include "jxta_peergroup.h"
#include "jxta_listener.h"
#include "jxta_endpoint_service.h"
#include "jxta_rdv_service.h"


Jxta_pipe_service * pipe_service = NULL;
Jxta_rdv_service  * rdv = NULL;

static void rdvclient_rdv_listener (Jxta_object *obj, void *arg) {
  Jxta_status res = 0;
  JString* string = NULL;
  Jxta_rdv_event* rdv_event = (Jxta_rdv_event*) obj;

  /** Get info from the event */
  int type = rdv_event->event;
  res = jxta_id_to_jstring(rdv_event->pid,&string);

  switch (type) {
  case JXTA_RDV_CONNECTED:
    printf ("Connected to rdv peer=%s\n",jstring_get_string(string));
    break;
  case JXTA_RDV_FAILED:
    printf ("Failed connection to rdv peer=%s\n",jstring_get_string(string));
    break;
  case JXTA_RDV_DISCONNECTED:
    printf ("Disconnect from rdv peer=%s\n",jstring_get_string(string));
    break;
  case JXTA_RDV_RECONNECTED:
    printf ("Reconnect from rdv peer%s\n",jstring_get_string(string));
    break;
  default: 
    printf("Event not catched = %d\n", type);
  }
}

int jxta_rdvclient_listener_run (int argc, char **argv) {
  Jxta_PG     * pg = NULL;
  Jxta_listener* listener = NULL;
  Jxta_status res = 0;

  if (argc != 1) {
    printf ("Syntax: jxta_rdvclient_listener\n");
    exit (0);
  }

  /** Start JXTA */
  res = jxta_PG_new_netpg (&pg);
  if (res != JXTA_SUCCESS) {
    printf("jxta_PG_netpg_new failed with error: %ld\n", res);
  }

  /** Get useful services */
  jxta_PG_get_rendezvous_service (pg, &rdv);

  /** Add a listener to rdv events */
  listener = jxta_listener_new ((Jxta_listener_func) rdvclient_rdv_listener,
				NULL,
				1,
				1);
  if (listener != NULL) {
    jxta_listener_start(listener);
    res = jxta_rdv_service_add_event_listener (rdv, (char *) "test", NULL, listener);
    if (res != JXTA_SUCCESS) {
      printf("Failed to add an event listener to rdv events");
    }
  }
  printf("wait for 10 sec for RDV event connection\n");
  jpr_thread_delay ((Jpr_interval_time) 10 * 1000 * 1000);

  return 0;
}


#ifdef STANDALONE
int main (int argc, char **argv) {

  apr_initialize();
  return jxta_rdvclient_listener_run(argc, argv);
}
#endif
