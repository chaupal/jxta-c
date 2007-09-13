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
 * $Id$
 */

static const char *__log_cat = "HTTP_POLLER";

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h>

#include "jxta_apr.h"
#include <apr_uri.h>

#include "jpr/jpr_types.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_string.h"
#include "jxta_private.h"
#include "jxta_relay.h"
#include "jxta_transport_http_client.h"
#include "jxta_transport_http_poller.h"
#include "jxta_endpoint_service.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define PACKAGE_STRING "jxta 2.1+"
#endif

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
struct _HttpPoller {
    JXTA_OBJECT_HANDLE;

    apr_pool_t *pool;
    volatile Jxta_boolean running;
    apr_thread_t *tid;

    Jxta_PG *group;
    char *relay_address;
    Jxta_endpoint_service *service;
    HttpClient *htcli;
    char *addr;
    const char *uri;
    const char *peerid;
    volatile const char *leaseid;
    Jxta_time leaseRenewalTime;
};

#define PROTOCOL_NAME "http"

static void *APR_THREAD_FUNC http_poller_body(apr_thread_t * t, void *arg);
static void http_poller_free(Jxta_object * obj);
static Jxta_status JXTA_STDCALL read_from_http_request(void *stream, char *buf, apr_size_t len);

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(HttpPoller *) http_poller_new(Jxta_PG * group,
                                           Jxta_endpoint_service * service,
                                           const char *proxy_host,
                                           Jxta_port proxy_port, const char *host, Jxta_port port, const char *uri,
                                           const char *peerid, Jxta_pool * pool1)
{
    HttpPoller *poller = (HttpPoller *) calloc(1, sizeof(HttpPoller));

    if (NULL != poller) {
        Jxta_id *groupId;
        JString *tmp;

        JXTA_OBJECT_INIT(poller, http_poller_free, 0);
        poller->group = group;
	jxta_PG_get_GID(group, &groupId);
	if (jxta_id_equals(groupId, jxta_id_defaultNetPeerGroupID)) {
	  tmp = jstring_new_2("jxta-NetGroup");
	} else {
	  jxta_id_get_uniqueportion(groupId, &tmp);
	}
	JXTA_OBJECT_RELEASE(groupId);
	poller->relay_address = malloc(strlen(RELAY_PREFIX_ADDRESS) + jstring_length(tmp) + strlen(RELAY_SUFFIX_ADDRESS) + 1);
	strcpy(poller->relay_address, RELAY_PREFIX_ADDRESS);
	strcat(poller->relay_address, jstring_get_string(tmp));
	strcat(poller->relay_address, RELAY_SUFFIX_ADDRESS);
	JXTA_OBJECT_RELEASE(tmp);
        poller->service = service;
        poller->htcli = http_client_new(proxy_host, proxy_port, host, port, NULL);
        poller->uri = uri;
        poller->addr = (char *) malloc(256);
        apr_snprintf(poller->addr, 256, "%s:%d", host, port);
        poller->peerid = peerid;
        poller->leaseid = NULL;
        poller->running = FALSE;
        poller->tid = NULL;
        poller->pool = (apr_pool_t *) pool1;
        poller->leaseRenewalTime = 0;
    }

    return poller;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void http_poller_free(Jxta_object * obj)
{
    HttpPoller *poller = (HttpPoller *) obj;

    if (poller->running) {
        http_poller_stop(poller);
    }

    if (poller->htcli != NULL)
        http_client_free(poller->htcli);

    free(poller->relay_address);
    free(poller->addr);
    free(poller);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_status) http_poller_start(HttpPoller * poller)
{
    apr_status_t status;

    if (poller->htcli == NULL)
        return JXTA_FAILED;

    poller->running = TRUE;

    status = apr_thread_create(&poller->tid, NULL, http_poller_body, (void *) poller, poller->pool);
    if (APR_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "failed thread start");
        poller->running = FALSE;
        return JXTA_FAILED;
    }

    /**
     ** We need to wait until the poller thread had a chance to get a lease, otherwise
     ** sending messages and waiting for an answer may not work for the first very few
     ** messages. Explicit synchronization is what has to be done.
     **/
/*
    while (poller->running && (NULL == poller->leaseid)) {
*/
    /* sleep for 100ms */
/*
        apr_sleep(100 * 1000);
    }
*/
    apr_sleep(3L * 1000L * 1000L);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "started\n");

    return status;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(void) http_poller_stop(HttpPoller * poller)
{
    apr_status_t rv;

    if (!poller->running) {
        return;
    }

    poller->running = FALSE;
    http_client_close(poller->htcli);

    apr_thread_join(&rv, poller->tid);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_boolean) http_poller_is_running(HttpPoller * poller)
{
    return poller->running;
}

/* Private functions. */
/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void *APR_THREAD_FUNC http_poller_body(apr_thread_t * t, void *arg)
{
    HttpPoller *poller = (HttpPoller *) arg;
    apr_status_t status;
    HttpRequest *req;
    HttpResponse *res;
    char *cmd;
    char uri[1024];
    char uri_msg[1024];
    Jxta_message *msg = NULL;
    Jxta_status rv;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "HTTP poller thread starts\n");

    status = http_client_connect(poller->htcli);
    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed connect : %s \n", msg);
        poller->running = FALSE;
        apr_thread_exit(t, !APR_SUCCESS);
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
    apr_snprintf(uri, sizeof(uri),"%s%s?%s,%s://%s/%s/%s,%s,%s", poller->uri, poller->peerid, REQUEST_TIMEOUT,
            PROTOCOL_NAME, poller->addr, poller->relay_address, "connect", LEASE_REQUEST, LAZY_CLOSE);

    req = http_client_start_request(poller->htcli, "GET", uri, cmd);
    http_request_set_header(req, "User-Agent", PACKAGE_STRING);

    /*
     * need to send the end of header mark
     */
    http_request_write(req, "\r\n", 2);

    res = http_request_done(req);
    http_request_free(req);

    if (http_get_content_length(res) > 0) {
        msg = jxta_message_new();

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "received a lease response.\n");

        rv = jxta_message_read(msg, "application/x-jxta-msg", read_from_http_request, res);
        if (rv != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Failed to read message\n");
            JXTA_OBJECT_RELEASE(msg);
            msg = NULL;
        }

    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "received empty lease request message\n");
    }
    http_response_free(res);

    /*
     * leaseid are now defined as peerid in JXTA 2.0
     * however we are obtaining a real lease time
     * from the relay.
     */
    poller->leaseid = poller->peerid;

    if (msg) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "process lease: %s\n", poller->leaseid);
        jxta_endpoint_service_demux(poller->service, msg);
        JXTA_OBJECT_RELEASE(msg);
    }

    while (poller->running) {
        char *active_poller = jxta_endpoint_service_get_relay_addr(poller->service);

        /*
         * check if this poller is associated with the current active
         * relay poller. Only the active relay poller should do any
         * work. The other ones should stay in standby until a relay
         * failover occurs
         */
        if (active_poller != NULL) {
            if (strcmp(poller->addr, active_poller) != 0) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Inactive Poller %s, will stop\n", poller->addr);
                free(active_poller);
                break;
            }
            free(active_poller);
        }
        /* 
         * check if we got a message for us
         */
        apr_snprintf(uri_msg, sizeof(uri_msg),"%s%s?%s,%s://%s/%s/", poller->uri, poller->peerid,
                REQUEST_TIMEOUT, PROTOCOL_NAME, poller->addr, poller->relay_address);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Polling for a new message\n");
        req = http_client_start_request(poller->htcli, "POST", uri_msg, cmd);

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
            if (APR_SUCCESS != http_client_connect(poller->htcli)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed re-opening relay connection\n");
                /*
                 * Reset the relay info to the endpoint service. As this point
                 * no default route exists.
                 */
                jxta_endpoint_service_set_relay(poller->service, NULL, NULL);
                apr_thread_exit(t, JXTA_FAILED);
                return NULL;
            }
            continue;
        }

        http_request_set_header(req, "Content-Length", "0");
        http_request_set_header(req, "User-Agent", PACKAGE_STRING);
        http_request_write(req, "\r\n", 2);

        res = http_request_done(req);
        http_request_free(req);

        if (http_get_content_length(res) > 0) {
            msg = jxta_message_new();

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "HTTP POLLER: received a new message.\n");

            rv = jxta_message_read(msg, "application/x-jxta-msg", read_from_http_request, res);
            if (rv != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Failed to read message\n");
            } else {
                jxta_endpoint_service_demux(poller->service, msg);
            }
            JXTA_OBJECT_RELEASE(msg);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Received empty message\n");
        }

        http_response_free(res);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "http poller check ok\n");
    }

    /*
     * Reset the relay info to the endpoint service. As this point
     * no default route exists.
     */
    jxta_endpoint_service_set_relay(poller->service, NULL, NULL);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Poller thread is stopping\n");

    /* NOTREACHED */
    return NULL;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status JXTA_STDCALL read_from_http_request(void *stream, char *buf, apr_size_t len)
{
    Jxta_status res = http_response_read_n((HttpResponse *) stream, buf, len);

    /**
     ** 2/19/2002 lomax@jxta.org
     ** FIX ME: http_client.c uses APR errors, while this code returns JPR error.
     **/
    if (res == APR_SUCCESS) {
        res = JXTA_SUCCESS;
    } else {
        char msg[256];
        apr_strerror(res, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "HTTP_REQUEST reading of message failed : %s\n", msg);
    }
    return res;
}

/* vim: set ts=4 sw=4 tw=130 et: */
