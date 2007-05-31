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
 * $Id: chat.c,v 1.28 2005/04/07 22:58:52 slowhog Exp $
 */


/****************************************************************
 **
 ** Prints all propagates pipe messages
 **
 ****************************************************************/


#include <stdio.h>
#include <stdlib.h>

#include "jxta.h"

#include <apr_general.h>

#include "jxta_debug.h"
#include "jpr/jpr_thread.h"
#include "jxta_peergroup.h"
#include "jxta_listener.h"
#include "jxta_endpoint_service.h"
#include "jxta_vector.h"
#include "jxta_pipe_service.h"


#include "apr_time.h"


Jxta_listener* listener = NULL;

static const char * DEFAULT_ADV_FILENAME= "netchat.adv";
static const char * SENDERNAME          = "JxtaTalkSenderName";
static const char * SENDERGROUPNAME     = "GrpName";
static const char * SENDERMESSAGE       = "JxtaTalkSenderMessage";
static const char * MYGROUPNAME         = "NetPeerGroup";

static void
send_message (Jxta_outputpipe* op, char* userName, char* userMessage) {

  Jxta_message* msg = jxta_message_new();
  Jxta_message_element* el = NULL;
  char* pt = NULL;
  Jxta_status res;

  /*
  JXTA_LOG ("Send message from [%s] msg=[%s]\n", userName, userMessage);
  */  
  printf ("Send message from [%s] msg=[%s]\n", userName, userMessage);

  JXTA_OBJECT_CHECK_VALID (op);

  pt = malloc (strlen (userName) + 1);
  strcpy (pt, userName);

  el =  jxta_message_element_new_1 ((char*) SENDERNAME,
				  (char*) "text/plain",
				  pt,
				  strlen (pt),
				  NULL );

  jxta_message_add_element       (msg, el);

  JXTA_OBJECT_RELEASE (el);

free(pt);

  pt = malloc (strlen (MYGROUPNAME) + 1);
  strcpy (pt,MYGROUPNAME);
  
  el =  jxta_message_element_new_1 ((char*) SENDERGROUPNAME,
				  (char*) "text/plain",
				  pt,
				  strlen (pt),
				  NULL );
  
  jxta_message_add_element       (msg, el);

  JXTA_OBJECT_RELEASE (el);

free(pt);

  el =  jxta_message_element_new_1 ((char*) SENDERMESSAGE,
				  (char*) "text/plain",
				  userMessage,
				  strlen (userMessage),
				  NULL );

  jxta_message_add_element       (msg, el);

  JXTA_OBJECT_RELEASE (el);

  /*
  JXTA_LOG ("Sending message\n");
   */  
  printf ("Sending message\n");

  res = jxta_outputpipe_send (op, msg);
  if (res != JXTA_SUCCESS) {
    printf ("sending failed: %d\n", (int) res);
  }
  JXTA_OBJECT_CHECK_VALID (msg);
  JXTA_OBJECT_RELEASE (msg); msg = NULL;
}


static void
processIncomingMessage (Jxta_message* msg) {

  JString* groupname = NULL;
  JString* senderName = NULL;
  JString* message = NULL;
  Jxta_message_element* el = NULL;

  jxta_message_get_element_1 (msg, SENDERGROUPNAME, &el );
    
  if (el) {
    Jxta_bytevector* val = jxta_message_element_get_value(el);
    groupname = jstring_new_3( val );
    JXTA_OBJECT_RELEASE(val); val = NULL;
    
    JXTA_OBJECT_RELEASE (el);
  }

  el = NULL;
  jxta_message_get_element_1 (msg, SENDERMESSAGE, &el);
    
  if (el) {
    Jxta_bytevector* val = jxta_message_element_get_value(el);
    message = jstring_new_3( val );
    JXTA_OBJECT_RELEASE(val); val = NULL;
    
    JXTA_OBJECT_RELEASE (el);
  }

  el = NULL;
  jxta_message_get_element_1 (msg, SENDERNAME, &el);
    
  if (el) {
    Jxta_bytevector* val = jxta_message_element_get_value(el);
    senderName = jstring_new_3( val );
    JXTA_OBJECT_RELEASE(val); val = NULL;
    
    JXTA_OBJECT_RELEASE (el);
  }

  if (message) {
    fprintf (stdout,"\n##############################################################\n");
    fprintf (stdout,"CHAT MESSAGE (group= %s) from %s :\n",
	    jstring_get_string(groupname), senderName == NULL ? "Unknown" : jstring_get_string(senderName));
    fprintf (stdout,"%s\n", jstring_get_string(message) );
    fprintf (stdout,"##############################################################\n\n");
    fflush(stdout);
  }
}

static void
message_listener (Jxta_object* obj, void* arg) {
  Jxta_message* msg = (Jxta_message*) obj;

  processIncomingMessage (msg);

  JXTA_OBJECT_RELEASE (msg);
}


static char*
get_user_string (void) {

  int   capacity = 8192;
  char* pt = malloc (capacity);
  int   got = 0;

  memset (pt, 0, 256);

  if (fgets (pt, 8192, stdin) != NULL) {
    if (strlen (pt) > 1) {
      pt [strlen (pt) - 1] = 0; /* Strip off last /n */
      return pt;
    }
  }
  free (pt);
  return NULL;
}

static void
process_user_input (Jxta_outputpipe* pipe, char* userName) {

  char* userMessage = NULL;

  /**
   ** Let every else start...
   **/
  printf ("Welcome to JXTA-C Chat, %s\n", userName);
  printf ("Type a '.' at begining of line to quit.\n");
  for(;;) {

    userMessage = get_user_string ();
    if (userMessage != NULL)
    if (userMessage[0] == '.') {
      free (userMessage);
      break;
    }
    if (userMessage != NULL){
	    send_message (pipe, userName, userMessage);
    }
  }
}


Jxta_boolean 
chat_run (int argc, char **argv) {

  Jxta_pipe_service * pipe_service = NULL;
  Jxta_pipe*          pipe = NULL;
  Jxta_inputpipe    * ip   = NULL;
  Jxta_outputpipe   * op   = NULL;
  char              * userName = NULL;
  char              * rdvAddr = NULL;
  Jxta_PG           * pg = NULL;
  Jxta_pipe_adv     * adv = NULL;
  Jxta_rdv_service  * rdv = NULL;
  FILE *file;
  Jxta_status res = 0;

  if ((argc < 2) || (argc > 3)) {
    printf ("Syntax: chat user_name [rdv]\n");
    exit (0);
  }

  userName = argv[1];
  if (argc == 3) {
    rdvAddr = argv[2];
  } else {
    rdvAddr = "127.0.0.1:9700";
  }

  res = jxta_PG_new_netpg (&pg);


  if (res != JXTA_SUCCESS) {
    printf("jxta_PG_netpg_new failed with error: %ld\n", res);
  }

  jxta_PG_get_pipe_service (pg, &pipe_service);
  jxta_PG_get_rendezvous_service (pg, &rdv);

  /**
   ** If a rendezvous has been specified, add it to the rendezvous service.
   **/
  if (rdvAddr != NULL) {
    Jxta_peer* peer = jxta_peer_new();
    Jxta_endpoint_address* addr = jxta_endpoint_address_new2 ((char*) "http",
							      rdvAddr,
							      NULL,
							      NULL);
    jxta_peer_set_address (peer, addr);

    jxta_rdv_service_add_peer (rdv, peer);
    JXTA_OBJECT_RELEASE (addr);
    JXTA_OBJECT_RELEASE (peer);
  }

  /**
   ** Create a listener for the myJXTA messages
   **/
  listener = jxta_listener_new ((Jxta_listener_func) message_listener,
				NULL,
				1,
				200);
  if (listener == NULL) {
/*
     JXTA_LOG ("Cannot create listener\n");
     */
     printf("Cannot create listener\n");

    return FALSE;
  }

  /**
   ** Get the pipe advertisement.
   **/

  adv = jxta_pipe_adv_new();
  file = fopen (DEFAULT_ADV_FILENAME, "r");
  jxta_pipe_adv_parse_file (adv, file);
  fclose(file);
  
  /**
   ** Get the pipe
   **/
  res = jxta_pipe_service_timed_accept (pipe_service,
					adv,
					1000,
					&pipe);

  if (res != JXTA_SUCCESS) {
     /*
    JXTA_LOG ("Cannot get pipe reason= %d\n", res);
    */
     printf("Cannot get pipe reason= %d\n", res);

    return FALSE;
  }

  res = jxta_pipe_get_outputpipe (pipe, &op);
  
  if (res != JXTA_SUCCESS) {
    //JXTA_LOG ("Cannot get outputpipe reason= %d\n", res);
    printf ("Cannot get outputpipe reason= %d\n", res);
    return FALSE;
  }


  res = jxta_pipe_get_inputpipe (pipe,	&ip);
  
  if (res != JXTA_SUCCESS) {
    //JXTA_LOG ("Cannot get inputpipe reason= %d\n", res);
    printf ("Cannot get inputpipe reason= %d\n", res);
    return FALSE;
  }

  res = jxta_inputpipe_add_listener (ip, listener);

  if (res != JXTA_SUCCESS) {
    //JXTA_LOG ("Cannot add listener reason= %d\n", res);
    printf ("Cannot add listener reason= %d\n", res);
    return FALSE;
  }

  jxta_listener_start (listener);
  
  process_user_input (op, userName);

  return TRUE;
}


#ifdef STANDALONE
int
main (int argc, char **argv) {
    int r;

    jxta_initialize();
    r = (int)chat_run(argc, argv);
    jxta_terminate();
    return r;
}
#endif






