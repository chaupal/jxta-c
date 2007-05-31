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
 * $Id: jxta_transport_http_poller.c,v 1.12 2005/01/31 20:44:36 slowhog Exp $
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h>

#include <apr.h>
#include <apr_network_io.h>
#include <apr_uri.h>
#include <apr_strings.h>
#include <apr_thread_proc.h>

#include "jpr/jpr_types.h"
#include "jpr/jpr_thread.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_string.h"
#include "jxta_private.h"
#include "jxta_relay.h"
#include "jxta_transport_http_client.h"
#include "jxta_transport_http_poller.h"
#include "jxta_endpoint_service.h"

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
struct _HttpPoller {
    Jxta_PG* group;
    Jxta_endpoint_service* service;
    HttpClient* htcli;
    char* addr;
    char* uri;
    char* peerid;
    char* leaseid;
    Jxta_time leaseRenewalTime;
    int run;
    apr_thread_t* tid;
    apr_pool_t* pool;
};

#define PROTOCOL_NAME "http"
     
static void* APR_THREAD_FUNC http_poller_body (apr_thread_t* t, void* arg);
static Jxta_status read_from_http_request (void* stream, char* buf, apr_size_t len);

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
HttpPoller*
http_poller_new (Jxta_PG* group,
                 Jxta_endpoint_service* service,
                 const char* proxy_host,
                 Jxta_port   proxy_port,
                 const char* host,
                 Jxta_port   port,
                 char*       uri,
                 char*       peerid,
                 Jxta_pool* pool1) {

    HttpPoller* poller = (HttpPoller*) malloc(sizeof (HttpPoller));

    apr_pool_t* pool = (apr_pool_t*) pool1;

    poller->group   = group;
    poller->service = service;
    poller->htcli = http_client_new(proxy_host, proxy_port, host, port, NULL );
    poller->uri = uri;
    poller->addr = malloc(256);
    sprintf(poller->addr, "%s:%d", host, port);
    poller->peerid = peerid;
    poller->leaseid = NULL;
    poller->run = 0;
    poller->tid = NULL;
    poller->pool = pool;
    poller->leaseRenewalTime = 0;
    return poller;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void
http_poller_free (HttpPoller* poller) {

    if (poller->htcli != NULL)
      http_client_free(poller->htcli);
    if (poller->leaseid)
        free(poller->leaseid);
    poller->run = 0;
    free(poller);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
Jpr_status
http_poller_start (HttpPoller* poller) {
    apr_status_t status;

    if (poller->htcli == NULL)
      return JXTA_FAILED;
    
    status = apr_thread_create(&poller->tid,
                               NULL,
                               http_poller_body,
                               poller,
                               poller->pool);

    if (!APR_STATUS_IS_SUCCESS(status)) {
        fprintf(stderr, "failed thread start");
        poller->run = 0;
    }

    /**
     ** 2/21/2002 lomax@dioxine.com FIXME
     **
     ** We need to wait until the poller thread had a chance to get a lease, otherwise
     ** sending messages and waiting for an answer may not work for the first very few
     ** messages. Explicit synchronization is what has to be done.
     **/
    jpr_thread_delay (3 * 1000 * 1000);

    return status;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void
http_poller_stop (HttpPoller* poller) {

    poller->run = 0;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
int
http_poller_get_stopped (HttpPoller* poller) {
    return poller->run;
}

/* Private functions. */
/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void* APR_THREAD_FUNC
http_poller_body (apr_thread_t* t, void* arg) {

     HttpPoller* poller = (HttpPoller*) arg;
     HttpRequest* req;
     HttpResponse* res;
     char* cmd;
     char uri[1024];
     char uri_msg[1024];
     Jxta_message* msg = NULL;
     Jxta_time lease;
     char* active_poller;

     JXTA_LOG ("HTTP poller thread starts\n");
     poller->run = 1;
	    
     if (!APR_STATUS_IS_SUCCESS(http_client_connect(poller->htcli))) {
       JXTA_LOG ("failed connect\n");
       apr_thread_exit (t, !APR_SUCCESS);
     }

     cmd = NULL;
     
     /* 
      * query string is of the format ?{request timeout},{lazy close timeout}
      * the timeout's are expressed in seconds.
      *  -1 means do not wait at all, 0 means wait forever
      * default value is wait 2 minutes (120*1000 millis) for a message.
      *  Lazy close timeout is set to the same. Try -1 if multiple msgs is trbl.
      * That is typically the case for jdk <= 1.4, so we do it automatically
      * in that case.
      */
    sprintf(uri, "%s%s?%s,%s://%s/%s/%s,%s,%s", poller->uri, poller->peerid, REQUEST_TIMEOUT,
	    PROTOCOL_NAME, poller->addr, RELAY_ADDRESS, "connect", LEASE_REQUEST, LAZY_CLOSE);
    
    req = http_client_start_request(poller->htcli,
                                    "GET",
                                    uri,
				    cmd);
    http_request_set_header (req, "User-Agent", "JXTA-C 2.0");
    
    /*
     * need to send the end of header mark
     */
    http_request_write (req, "\r\n", 2);
    
    res = http_request_done(req);
    http_request_free (req);
    
    if (http_get_content_length (res) > 0) {
      msg = jxta_message_new();
      
      JXTA_LOG("received a lease response.\n");
      
      jxta_message_read(msg, "application/x-jxta-msg",
			read_from_http_request,
			res );
      
    } else {
      JXTA_LOG ("received empty lease request message\n");
    }
    http_response_free (res);
     
    /*
     * leaseid are now defined as peerid in JXTA 2.0
     * however we are obtaining a real lease time
     * from the relay.
     */
    poller->leaseid = poller->peerid;
    
    lease = 0;
    
    if (msg) {
      JXTA_LOG ("process lease: %s\n", poller->leaseid);
      jxta_endpoint_service_demux(poller->service, msg);
    }

    while (poller->run) {

      if (!poller->run) {	
		JXTA_LOG ("poller must stop\n");
		break;
      }	
       

      /*
       * check if this poller is associated with the current active
       * relay poller. Only the active relay poller should do any
       * work. The other ones should stay in standby until a relay
       * failover occurs
       */
      active_poller = jxta_endpoint_service_get_relay_addr(poller->service);
      if (active_poller != NULL) {
        if (strcmp(poller->addr, active_poller) != 0) {
          JXTA_LOG("Inactive Poller %s\n", poller->addr);
          free(active_poller);
          break;
        }
        free(active_poller);
      }
      /* 
       * check if we got a message for us
       */
      sprintf(uri_msg, "%s%s?%s,%s://%s/%s/", poller->uri, poller->peerid, 
	      REQUEST_TIMEOUT, PROTOCOL_NAME, poller->addr, RELAY_ADDRESS);
      JXTA_LOG("Pulling for a new message\n");
      req = http_client_start_request(poller->htcli,
                                      "POST",
                                      uri_msg,
                                      cmd);

	  /* 
	   * check if we successfully sent our request
	   */
      if (req == NULL) {
        /*
         * close our previous connection
         */
        http_client_close(poller->htcli);
        /* 
         * Try to open a new connection
         */
        if(!APR_STATUS_IS_SUCCESS(http_client_connect(poller->htcli))) {
          JXTA_LOG("Failed re-opening relay connection\n");
          /*
           * Reset the relay info to the endpoint service. As this point
           * no default route exists.
           */
          jxta_endpoint_service_set_relay(poller->service, NULL, NULL);
          return NULL;
        }
        continue;
	  }

      http_request_set_header (req, "User-Agent", "JXTA-C 2.0");
      http_request_write (req, "\r\n", 2);
      
      res = http_request_done(req);
      
      http_request_free (req);
      
      if (http_get_content_length (res) > 0) {
		msg = jxta_message_new();
	
		JXTA_LOG("HTTP POLLER: received a new message.\n");
	
		jxta_message_read(msg, "application/x-jxta-msg",
			  read_from_http_request,
			    res );
	
		jxta_endpoint_service_demux(poller->service, msg);
		} else {
		JXTA_LOG ("Received empty message\n");
		}

		http_response_free (res);
		JXTA_LOG("http poller check ok\n");
	}

	/*
	 * Reset the relay info to the endpoint service. As this point
	 * no default route exists.
	 */
	jxta_endpoint_service_set_relay(poller->service, NULL, NULL);
    return NULL;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
read_from_http_request (void* stream, char* buf, apr_size_t len) {
    Jxta_status res = -1;

    res =  http_response_read_n((HttpResponse*) stream, buf, len);
    /**
     ** 2/19/2002 lomax@jxta.org
     ** FIX ME: http_client.c uses APR errors, while this code returns JPR error.
     **/
    if (res == APR_SUCCESS) {
        res = JXTA_SUCCESS;
    } else {
        JXTA_LOG ("HTTP_REQUEST reading of message failed\n");
    }
    return res;
}
