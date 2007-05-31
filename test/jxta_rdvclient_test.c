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
 * $Id: jxta_rdvclient_test.c,v 1.18 2004/06/10 00:56:38 tra Exp $
 */


/****************************************************************
 **
 ** This test program tests the JXTA Rendezvous Service Client
 **
 ****************************************************************/

#include "jxta.h"
#include "jpr/jpr_thread.h"
#include "jxta_peergroup.h"

#include "apr_time.h"

#define WAIT_TIME (15 * 60 * 1000 * 1000) /* 15 minutes */


boolean display_peers (Jxta_rdv_service* rdv) {

  Jxta_peer* peer = NULL;
  Jxta_id* pid = NULL;
  Jxta_PA*      adv = NULL;
  JString*  string = NULL;
  Jxta_time expires = 0;
  boolean connected = FALSE;
  Jxta_status err;
  Jxta_vector* peers = NULL;
  Jxta_time currentTime;
  Jxta_endpoint_address* addr;
  int i;

  /* Get the list of peers */
  err = jxta_rdv_service_get_peers (rdv, &peers);
  if (err != JXTA_SUCCESS) {
    printf ("Failed getting the peers. reason= %d\n", (int) err);
    return FALSE;
  }
  for (i = 0; i < jxta_vector_size (peers); ++i) {

    err = jxta_vector_get_object_at (peers, (Jxta_object**) &peer, i);
    if (err != JXTA_SUCCESS) {
      printf ("Failed getting a peer. reason= %d\n", (int) err);
      return FALSE;
    }

    err = jxta_peer_get_address (peer, &addr);
    if (err != JXTA_SUCCESS) {
      printf ("Failed getting the rdv address . reason= %d\n", (int) err);
      return FALSE;
    }
 
    printf("\nRDV Addr: %s\n", jxta_endpoint_address_to_string(addr));
	   
    connected = jxta_rdv_service_peer_is_connected (rdv, peer);
    printf ("Status: %s\n", connected ? "CONNECTED" : "NOT CONNECTED");

    if (connected) {
      err = jxta_peer_get_adv (peer, &adv);
      if (err == JXTA_SUCCESS) {
	if (adv != NULL) {
	  string = jxta_PA_get_Name (adv);
	  printf ("Name: %s\n", jstring_get_string (string));
	  JXTA_OBJECT_RELEASE (string);
	  JXTA_OBJECT_RELEASE (adv);
	}
      }
      err = jxta_peer_get_peerid (peer, &pid);
      if (err == JXTA_SUCCESS) {
	if (pid != NULL) {
	  jxta_id_to_jstring ( pid, &string );
	  printf ("PeerId: %s\n", jstring_get_string (string));
	  JXTA_OBJECT_RELEASE (string);
	}
      }
      
      expires = jxta_rdv_service_peer_get_expires (rdv, peer);
      
      if (connected && (expires >= 0)) {
	Jxta_time hours = 0;
	Jxta_time minutes = 0;
	Jxta_time seconds = 0;
	
	currentTime = apr_time_now();
	if (expires < currentTime) {
	  expires = 0;
	} else {
	  expires -= currentTime;
	}
	
	expires = expires / (1000 * 1000);
	
	
      hours = (Jxta_time) (expires / (Jxta_time) (60 * 60));
      expires -= hours * 60 * 60 * 1000 * 1000;
      
      minutes = expires / 60;
      expires -= minutes * 60;
      
      seconds = expires;
      
      printf ("Lease expires in %lld hour(s) %lld minute(s) %lld second(s)\n",
	      (Jxta_time) hours,
	      (Jxta_time) minutes,
	      (Jxta_time) seconds);
      }
    }
  }
  return TRUE;
}

boolean 
jxta_rdv_service_client_test(int argc, char** argv) {

  Jxta_rdv_service* rdv;
  Jxta_status err;
  Jxta_PG* pg;
  char*    rdvAddr = NULL;

  if (argc == 2) {
    rdvAddr = argv[1];
  } else {
    rdvAddr = (char *) "127.0.0.1:9700";
  }

  err = jxta_PG_new_netpg (&pg);

  if (err != JXTA_SUCCESS) {
    printf("jxta_PG_netpg_new failed with error: %ld\n", err);
    return FALSE;
  }
  jxta_PG_get_rendezvous_service (pg, &rdv);

  {
    Jxta_peer* peer = jxta_peer_new();
    Jxta_endpoint_address* addr = jxta_endpoint_address_new2 ((char *) "http",
							      rdvAddr,
							      NULL,
							      NULL);
    jxta_peer_set_address (peer, addr);
  
    jxta_rdv_service_add_peer (rdv, peer);
    JXTA_OBJECT_RELEASE (addr);
    JXTA_OBJECT_RELEASE (peer);
  }

  /* Wait a bit */
  jpr_thread_delay ((Jpr_interval_time) 5 * 1000 * 1000);
  if (!display_peers (rdv)) {
    return FALSE;
  }
  /* Wait to let the protocols run */
  jpr_thread_delay ((Jpr_interval_time) WAIT_TIME);
  return TRUE;
}


#ifdef STANDALONE
int
main (int argc, char **argv) {

  printf("STARTING TEST\n");
  apr_initialize();
  return (int)jxta_rdv_service_client_test(argc, argv);
}
#endif






