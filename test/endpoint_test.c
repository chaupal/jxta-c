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
 * $Id: endpoint_test.c,v 1.23 2005/04/07 22:58:53 slowhog Exp $
 */

#include <stdio.h>
#include <signal.h>
#include "jxta.h"
#include "jxta_peergroup.h"

#include "../src/jxta_private.h" 

#include "jpr/jpr_thread.h"

/****************************************************************
 **
 ** This test program tests the JXTA message code. It
 ** reads a pre-cooked message (testmsg in the test directory)
 ** which is in the wire format, and builds a message.
 **
 ****************************************************************/


#define ENDPOINT_TEST_SERVICE_NAME "jxta:EndpointTest"
#define ENDPOINT_TEST_SERVICE_PARAMS "EndpointTestParams"

Jxta_PG* pg = NULL;
apr_pool_t* pool;
Jxta_endpoint_service* endpoint = NULL;
Jxta_endpoint_address* hostAddr;

Jxta_transport* http_transport;

void
listener (Jxta_object* msg, void* cookie) {

  printf("Received a message\n");
}

void
init(void) {
  Jxta_status res;
  Jxta_listener* endpoint_listener = NULL;

  apr_pool_create (&pool, NULL);

  /**
   ** Get the endpoint srvce from the net pg.
   **/
  
  res = jxta_PG_new_netpg(&pg);

  if (res != JXTA_SUCCESS) {
      printf("jxta_PG_netpg_new failed with error: %ld\n", res);
      exit(res);
  }

  jxta_PG_get_endpoint_service(pg, &endpoint);

  /* sets and endpoint listener */
  endpoint_listener = jxta_listener_new (listener, NULL, 1, 0);
  
  jxta_endpoint_service_add_listener (endpoint,
				      (char*) ENDPOINT_TEST_SERVICE_NAME,
				      (char*) ENDPOINT_TEST_SERVICE_PARAMS,
				      endpoint_listener);

  jxta_listener_start(endpoint_listener);
}



/**
 ** This functions is used by the message implementation in order
 ** to transfer bytes between the file containing the message and the
 ** message object (this is a callback).
 **/


apr_status_t
readMessageBytes (void* stream, char* buf, apr_size_t len) {

  fread ((void*) buf,(size_t) len, 1, (FILE*) stream);
  return APR_SUCCESS;
}

apr_status_t
writeMessageBytes (void* stream, char* buf, apr_size_t len) {

  fwrite ((void *) buf, (size_t) len, 1, (FILE*) stream);
  return APR_SUCCESS; 
}

int
main (int argc, char **argv)
{
   Jxta_message* msg;
   char* host;

#ifndef WIN32
   struct sigaction sa;

   sa.sa_flags = 0;
   sa.sa_handler = SIG_IGN;
   sigaction(SIGPIPE, &sa, NULL);
#endif

   if(argc > 2) {
       printf("usage: endpoint_test ip:port>\n");
       return -1;
   }

   if (argc == 1) {
     host = "127.0.0.1:9700";
   } else {
     host = argv[1];
   }

   jxta_initialize();

   /* Initialise the endpoint and http service, set the endpoint listener */
   init();

   /* Builds the destination address */
    hostAddr =  jxta_endpoint_address_new2 ("http",
					    host,
					    ENDPOINT_TEST_SERVICE_NAME,
					    ENDPOINT_TEST_SERVICE_PARAMS);

    /* Builds a message (empty) and sends it */
    msg = jxta_message_new();

    jxta_endpoint_service_send (pg,
				endpoint,
				msg,
				hostAddr);
   

    JXTA_OBJECT_RELEASE (msg);

    /* Sleep a bit to let the endpoint works */
    jpr_thread_delay (1 * 60 * 1000 * 1000);

   apr_pool_destroy(pool);
   jxta_terminate();

   return 0;
   
}
