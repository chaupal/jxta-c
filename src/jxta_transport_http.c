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
 * $Id: jxta_transport_http.c,v 1.71 2006/02/17 23:26:11 slowhog Exp $
 */

static const char *__log_cat = "HTTP_TRANSPORT";

#include <stdlib.h> /* for atoi */

#ifndef WIN32
#include <signal.h> /* for sigaction */
#endif

#include "jxta_apr.h"
#include "jpr/jpr_excep_proto.h"

#include "jstring.h"
#include "jxta_types.h"
#include "jxta_transport_http_client.h"
#include "jxta_transport_http_poller.h"
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_transport_private.h"
#include "jxta_peergroup.h"
#include "trailing_average.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_hta.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define PACKAGE_STRING "jxta 2.1+"
#endif

/******************************************************************************/
/*                                                                            */
/******************************************************************************/

typedef struct {

    Extends(Jxta_transport);

    Jxta_endpoint_address *address;
    char *peerid;
    const char *srv_host;       /* our public host name (relay only) */
    int srv_port;               /* our public port nb (relay only) */
    const char *proxy_host;     /* the proxy host we use */
    int proxy_port;             /* the port nb there */
    Jxta_boolean is_relay;
    Jxta_PG *group;
    Jxta_vector *clientMessengers;
    Jxta_endpoint_service *endpoint;
    apr_thread_mutex_t *mutex;
    TrailingAverage *trailing_average;
    apr_pool_t *pool;

} Jxta_transport_http;

#define DEFAULT_PORT 9700

/**
 ** The HTTP client JxtaEndpointMessenger structure
 ** This structure extends JxtaEndpointMessenger, which is
 ** itself a Jxta_object, so, the HttpClientMessenger is also a
 ** Jxta_object (needs to be released when not used anymore.
 **/

typedef struct {
    struct _JxtaEndpointMessenger messenger;
    char *name;
    char *proxy_host;
    int proxy_port;
    char *host;
    int port;
    Jxta_boolean connected;
    HttpPoller *poller;
    TrailingAverage *trailing_average;
    apr_thread_mutex_t *mutex;
    Jxta_transport_http *tp;
    apr_pool_t *pool;
} HttpClientMessenger;


/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static HttpClientMessenger *http_client_messenger_new(Jxta_transport_http * tp, const char *proxy_host, int proxy_port,
                                                      const char *host, int port);
static HttpClientMessenger *get_http_client_messenger(Jxta_transport_http * tp, char *host, int port);

static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);
static void init_e(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv, Throws);
static Jxta_status start(Jxta_module * module, const char *argv[]);
static void stop(Jxta_module * module);
static JString *name_get(Jxta_transport * self);
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * self);
static JxtaEndpointMessenger *messenger_get(Jxta_transport * t, Jxta_endpoint_address * dest);
static Jxta_boolean ping(Jxta_transport * t, Jxta_endpoint_address * addr);
static void propagate(Jxta_transport * t, Jxta_message * msg, const char *service_name, const char *service_params);
static Jxta_boolean allow_overload_p(Jxta_transport * tr);
static Jxta_boolean allow_routing_p(Jxta_transport * tr);
static Jxta_boolean connection_oriented_p(Jxta_transport * tr);

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
typedef Jxta_transport_methods Jxta_transport_http_methods;

const Jxta_transport_http_methods jxta_transport_http_methods = {
    {
     "Jxta_module_methods",
     init,
     jxta_module_init_e_impl,
     start,
     stop},
    "Jxta_transport_methods",
    name_get,
    publicaddr_get,
    messenger_get,
    ping,
    propagate,
    allow_overload_p,
    allow_routing_p,
    connection_oriented_p
};

/******************************************************************************/
/*                                                                            */
/******************************************************************************/

/*********************
 * Messenger methods.*
 *********************/

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
Jxta_status JXTA_STDCALL msg_wireformat_size(void *arg, char const *buf, size_t len)
{
    size_t *size = (size_t *) arg;
    *size += len;
    return JXTA_SUCCESS;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
Jxta_status JXTA_STDCALL write_to_http_request(void *stream, char const *buf, size_t len)
{
    HttpRequest *req = (HttpRequest *) stream;
    return http_request_write(req, buf, len);
}


/******************
 * Module methods *
 *****************/

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    apr_status_t res;
    char *address_str;
    Jxta_id *id;
    JString *uniquePid;
    char *tmp;
    Jxta_transport_http *self;
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    Jxta_HTTPTransportAdvertisement *hta;
    JString *isclient = NULL;

#ifndef WIN32

    struct sigaction sa;

    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
#endif

    self = (Jxta_transport_http *) module;
    PTValid(self, Jxta_transport_http);

    self->trailing_average = trailing_average_new(60);

    /* Allocate a pool for our apr needs */
    res = apr_pool_create(&self->pool, NULL);
    if (res != APR_SUCCESS)
        return res;

    /**
     ** Allocate a new vector for the client messengers.
     ** 2/14/2002 lomax@jxta.org FIXME: when hastable will be implemented
     ** this vector should become an hashtable.
     **/
    self->clientMessengers = jxta_vector_new(0);
    if (self->clientMessengers == NULL)
        return JXTA_NOMEM;


    /**
     ** Create our mutex.
     **/
    res = apr_thread_mutex_create(&(self->mutex), APR_THREAD_MUTEX_NESTED,  /* nested */
                                  self->pool);
    if (res != APR_SUCCESS)
        return res;


    /*
     * following falls-back on backdoor config if needed only.
     * So it can be just be removed when no longer useful.
     */

    self->is_relay = FALSE;
    self->srv_host = "";
    self->srv_port = 0;
    self->group = group;

    /*
     * Here is the normal way of configuring. when we have been given a group.
     */

    /**
     ** Extract our configuration for the config adv.
     ** FIXME - jice@jxta.org 20020320 : painful.
     **/

    jxta_PG_get_configadv(group, &conf_adv);
    if (conf_adv == NULL)
        return JXTA_CONFIG_NOTFOUND;

    jxta_PA_get_Svc_with_id(conf_adv,assigned_id,&svc);

    if (svc == NULL) {
        JXTA_OBJECT_RELEASE(conf_adv);
        return JXTA_NOT_CONFIGURED;
    }

    hta = jxta_svc_get_HTTPTransportAdvertisement(svc);
    JXTA_OBJECT_RELEASE(svc);

    if (hta == NULL) {
        JXTA_OBJECT_RELEASE(conf_adv);
        return JXTA_CONFIG_NOTFOUND;
    }

    /*
     * FIXME - jice@jxta.org 20020320 : for now, we ignore the following
     * config items: Protocol, InterfaceAddress, Routerlist, server.
     *
     * FIXME - tra@jxta.org 20030130 : temporary transfer of relay function
     * configuration to the Relay service config, until we fully separate
     * relay and HTTP transport
     */

    jxta_PA_get_Svc_with_id(conf_adv,jxta_relayproto_classid_get(),&svc);
    JXTA_OBJECT_RELEASE(conf_adv);
    if (svc == NULL) {
        return JXTA_CONFIG_NOTFOUND;
    }

    isclient = jxta_svc_get_IsClient(svc);
    JXTA_OBJECT_RELEASE(svc);

    if (isclient == NULL) {
        return JXTA_CONFIG_NOTFOUND;
    }

    if (strcmp(jstring_get_string(isclient), "true") != 0) {
        JXTA_OBJECT_RELEASE(isclient);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Relay is disabled, so HTTP Transport is disabled\n");
        return JXTA_NOT_CONFIGURED;
    }

    JXTA_OBJECT_RELEASE(isclient);

    if (jxta_HTTPTransportAdvertisement_get_ProxyOff(hta)) {
        self->proxy_host = NULL;
        self->proxy_port = 0;
    } else {
        JString *proxy;
        const char *sproxy;
        const char *p;
        char *tmp_host;
        int len;

        proxy = jxta_HTTPTransportAdvertisement_get_Proxy(hta);
        sproxy = jstring_get_string(proxy);
        p = strchr(sproxy, ':');
        if (p == NULL) {
            p = sproxy + strlen(sproxy);
        }
        len = p - sproxy;
        tmp_host = malloc(len + 1);
        strncpy(tmp_host, sproxy, len);
        tmp_host[len] = '\0';
        self->proxy_host = tmp_host;
        self->proxy_port = atoi(p + 1);
        JXTA_OBJECT_RELEASE(proxy);
    }
    JXTA_OBJECT_RELEASE(hta);

    jxta_PG_get_endpoint_service(group, &(self->endpoint));

    /**
     ** Only the unique portion of the peerid is used by the HTTP Transport.
     **/
    jxta_PG_get_PID(group, &id);

    uniquePid = NULL;
    jxta_id_get_uniqueportion(id, &uniquePid);
    tmp = (char *) jstring_get_string(uniquePid);

    /**
     ** ..., the unique portion of the peer id is stored into
     ** peerid, which is not appropriate. To be revisited.
     **/

    self->peerid = strdup(tmp);

    /**
     ** We can now safely release all the intermediate objects
     **/
    JXTA_OBJECT_RELEASE(uniquePid);
    JXTA_OBJECT_RELEASE(id);
    
    if (self->peerid == NULL)
        return JXTA_NOMEM;

    if (self->is_relay)
        address_str = apr_psprintf(self->pool, "http://%s:%d", self->srv_host, self->srv_port);
    else
        address_str = apr_psprintf(self->pool, "http://JxtaHttpClient%s", self->peerid);

    self->address = jxta_endpoint_address_new(address_str);

    if (self->address == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not generate valid local address");
        return JXTA_NOMEM;
    }

    apr_thread_yield();

    return JXTA_SUCCESS;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status start(Jxta_module * module, const char *argv[])
{
    Jxta_transport_http *self;

    self = (Jxta_transport_http *) module;
    PTValid(self, Jxta_transport_http);

    jxta_endpoint_service_add_transport(self->endpoint, (Jxta_transport *) self);

    return JXTA_SUCCESS;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void stop(Jxta_module * module)
{
    Jxta_transport_http *self;

    self = (Jxta_transport_http *) module;
    PTValid(self, Jxta_transport_http);

    jxta_endpoint_service_remove_transport(self->endpoint, (Jxta_transport *) self);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "don't know how to stop yet.\n");
}

/**************************
 * Http Transport methods *
 *************************/

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static JString *name_get(Jxta_transport * self)
{
    PTValid(self, Jxta_transport_http);

    return jstring_new_2("http");
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * t)
{
    Jxta_transport_http *self = (Jxta_transport_http *) t;
    PTValid(self, Jxta_transport_http);

    return JXTA_OBJECT_SHARE(self->address);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static JxtaEndpointMessenger *messenger_get(Jxta_transport * t, Jxta_endpoint_address * dest)
{
    JxtaEndpointMessenger *messenger;
    char *protocol_address;
    char *host;
    char *colon;
    int port = DEFAULT_PORT;

    Jxta_transport_http *self = (Jxta_transport_http *) t;
    PTValid(self, Jxta_transport_http);

    protocol_address = (char*) jxta_endpoint_address_get_protocol_address(dest);

    colon = strchr(protocol_address, ':');
    if (colon && (*(colon + 1))) {
        port = atoi(colon + 1);
        host = (char *) malloc(colon - protocol_address + 1);
        host[0] = 0;
        strncat(host, protocol_address, colon - protocol_address);
    } else {
        host = strdup(protocol_address);
    }

    messenger = (JxtaEndpointMessenger *) get_http_client_messenger(self, host, port);
    messenger->address = dest;
    free(host);
    return messenger;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean ping(Jxta_transport * t, Jxta_endpoint_address * addr)
{
    PTValid(t, Jxta_transport_http);
    return FALSE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void propagate(Jxta_transport * t, Jxta_message * msg, const char *service_name, const char *service_params)
{
    /* this code intentionally left blank */
}


/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean allow_overload_p(Jxta_transport * tr)
{
    Jxta_transport_http *self = (Jxta_transport_http *) tr;
    PTValid(self, Jxta_transport_http);
    return FALSE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean allow_routing_p(Jxta_transport * tr)
{
    Jxta_transport_http *self = (Jxta_transport_http *) tr;
    PTValid(self, Jxta_transport_http);
    return TRUE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean connection_oriented_p(Jxta_transport * tr)
{
    Jxta_transport_http *self = (Jxta_transport_http *) tr;
    PTValid(self, Jxta_transport_http);
    return TRUE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void jxta_transport_http_construct(Jxta_transport_http * self, Jxta_transport_http_methods * methods)
{
    PTValid(methods, Jxta_transport_methods);
    jxta_transport_construct((Jxta_transport *) self, (Jxta_transport_methods *) methods);
    self->_super.metric = 2; /* value decided as JSE implemetnation */
    self->_super.direction = JXTA_OUTBOUND;

    self->thisType = "Jxta_transport_http";

    self->address = NULL;
    self->peerid = NULL;
    self->clientMessengers = NULL;
    self->endpoint = NULL;
    self->mutex = NULL;
    self->pool = NULL;
    self->group = NULL;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
/*
 * The destructor is never called explicitly (except by subclass' code).
 * It is called by the free function (installed by the allocator) when the object
 * becomes un-referenced.
 * Therefore, we not do need the destructor to be virtual.
 * The free function plays that role.
 */
void jxta_transport_http_destruct(Jxta_transport_http * self)
{
    PTValid(self, Jxta_transport_http);
    if (self->address != NULL) {
        JXTA_OBJECT_RELEASE(self->address);
    }
    if (self->endpoint) {
        JXTA_OBJECT_RELEASE(self->endpoint);
    }
    self->group = NULL;

    if (self->clientMessengers != NULL) {
        JXTA_OBJECT_RELEASE(self->clientMessengers);
    }
    if (self->peerid != NULL) {
        free(self->peerid);
    }

    if (self->pool) {
        apr_thread_mutex_destroy(self->mutex);
        apr_pool_destroy(self->pool);
    }

    trailing_average_free(self->trailing_average);

    jxta_transport_destruct((Jxta_transport *) self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Destruction finished\n");
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void http_free(Jxta_object * obj)
{
    jxta_transport_http_destruct((Jxta_transport_http *) obj);
    free((void *) obj);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
Jxta_transport_http *jxta_transport_http_new_instance(void)
{
    Jxta_transport_http *self = (Jxta_transport_http *) malloc(sizeof(Jxta_transport_http));
    memset(self, 0, sizeof(Jxta_transport_http));
    JXTA_OBJECT_INIT(self, http_free, NULL);
    jxta_transport_http_construct(self, (Jxta_transport_http_methods *) &jxta_transport_http_methods);
    return self;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status messenger_send(JxtaEndpointMessenger * mes, Jxta_message * msg)
{
    Jxta_status status;
    HttpClient *con;
    HttpClientMessenger *self = (HttpClientMessenger *) mes;
    int i;

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_CHECK_VALID(mes);

    JXTA_OBJECT_SHARE(msg);

    /**
     ** Create an http client connection
     **/

    if (self->proxy_host != NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "proxy %s:%d\n", self->proxy_host, self->proxy_port);
    }

    con = http_client_new(self->proxy_host, (Jxta_port)self->proxy_port, self->host, (Jxta_port)self->port, NULL);
    if (con == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "http: messenger_send failed creating a new http_client\n");
        JXTA_OBJECT_RELEASE(msg);
        return JXTA_FAILED;
    }

    if (http_client_connect(con) == APR_SUCCESS) {
        for (i = 0; i < 2; i++) {
            HttpRequest *req = NULL;
            HttpResponse *res = NULL;
            apr_size_t msgSize = 0;
            char *stringBuffer = (char *) malloc(1024);

            req = http_client_start_request(con, "POST", "/", NULL);
            http_request_set_header(req, "User-Agent", PACKAGE_STRING);

            /* Sets the source address into the message */

            JXTA_OBJECT_CHECK_VALID(msg);
            JXTA_OBJECT_CHECK_VALID(self->tp->address);

            jxta_message_set_source(msg, self->tp->address);

            JXTA_OBJECT_CHECK_VALID(msg);
            JXTA_OBJECT_CHECK_VALID(self->tp->address);

            jxta_message_write(msg, "application/x-jxta-msg", msg_wireformat_size, &msgSize);
            apr_snprintf(stringBuffer, 1024, "%d", msgSize);
            http_request_set_header(req, "Content-Length", stringBuffer);
            http_request_write(req, "\r\n", 2);
            free(stringBuffer);
            stringBuffer = NULL;

            jxta_message_write(msg, "application/x-jxta-msg", write_to_http_request, req);

            res = http_request_done(req);
            http_request_free(req);

            if (http_response_get_status(res) == HTTP_NOT_CONNECTED) {
                http_client_close(con);
                if (http_client_connect(con) == APR_SUCCESS) {
                    continue;
                } else {
                    break;
                }
            } else {
                trailing_average_inc(self->trailing_average);
                trailing_average_inc(self->tp->trailing_average);
            }

            http_response_free(res);
            break;
        }
        status = JXTA_SUCCESS;
    } else {
        status = JXTA_FAILED;
    }
    http_client_free(con);

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_CHECK_VALID(self->tp->address);

    JXTA_OBJECT_RELEASE(msg);

    return JXTA_SUCCESS;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void http_client_messenger_free(Jxta_object * addr)
{
    HttpClientMessenger *self = (HttpClientMessenger *) addr;

    if (self == NULL) {
        return;
    }

    if (NULL != self->poller) {
        http_poller_stop(self->poller);
        JXTA_OBJECT_RELEASE(self->poller);
        self->poller = NULL;
    }

    /*
     * We need to take the lock in order to make sure that nobody
     * else is using the object. However, since we are destroying
     * the object, there is no need to unlock. 
     */
    apr_thread_mutex_lock(self->mutex);

    trailing_average_free(self->trailing_average);

    /* Free the pool containing the mutex */
    apr_pool_destroy(self->pool);

    /* Free the host */
    free(self->name);
    free(self->host);

    /* Free the object itself */
    free(self);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static HttpClientMessenger *http_client_messenger_new(Jxta_transport_http * tp, const char *proxy_host, int proxy_port,
                                                      const char *host, int port)
{
    HttpClientMessenger *self;
    apr_status_t res;
    int len;

    /* Create the object */
    self = (HttpClientMessenger *) calloc(1, sizeof(HttpClientMessenger));

    if (self == NULL) {
        /* No more memory */
        return NULL;
    }

    /* Initialize it. */
    JXTA_OBJECT_INIT(self, http_client_messenger_free, NULL);

    self->trailing_average = trailing_average_new(60);

    /*
     * Allocate the mutex 
     * Note that a pool is created for each object. This allows to have a finer
     * granularity when allocated the memory. This constraint comes from the APR.
     */
    res = apr_pool_create(&self->pool, NULL);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        free(self);
        return NULL;
    }
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,    /* nested */
                                  self->pool);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(self->pool);
        free(self);
        return NULL;
    }

    self->connected = FALSE;
    self->tp = tp;
    self->messenger.jxta_send = messenger_send;
    self->proxy_host = proxy_host ? strdup(proxy_host) : NULL;
    self->proxy_port = proxy_port;
    self->host = malloc(strlen(host) + 1);
    strcpy(self->host, host);
    self->port = port;

    /* Duplicate the address. */
    /**
     ** 2/14/2002 lomax@jxta.org FIXME
     ** a copy of the address is needed only because we are using a Jxta_vector
     ** to store messenger instead of hashtable. See other comments on that topic
     ** below.
     **/
    len = strlen(host) + 20;
    self->name = malloc(len); /* we need to reserve space for the port */
    apr_snprintf(self->name, len, "%s:%d", host, port);

    self->poller = http_poller_new(self->tp->group,
                                   self->tp->endpoint,
                                   self->proxy_host, (Jxta_port)self->proxy_port,
                                   self->host, (Jxta_port)self->port, "/", 
                                   self->tp->peerid, (Jxta_pool*) self->tp->pool);

    if (APR_SUCCESS != http_poller_start(self->poller)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "HTTP poller failed in http_transport_init\n");
        JXTA_OBJECT_RELEASE(self->poller);
        self->poller = NULL;
        self->connected = FALSE;
    }

    return self;
}

/**
 ** Client Messengers management functions.
 **
 ** NOTE 2/14/2002 lomax@jxta.org TO BE REVISITED
 **
 ** The current implementation keeps HttpClientMessenger that has
 ** been created around. The reason for not closing them when there
 ** is no more "users" of the messenger is that a peer may not have
 ** to send any message, but still wants to receive.
 ** Some sort of garbage collection should be done in the future.
 **/


/**
 ** 2/14/2002 lomax@jxta.org FIXME
 **
 ** The following implementation uses Jxta_vector in order to store the
 ** existing HttpClientMessenger objects. It would be more efficient to
 ** use an hashtable, but since jice@jxta.org is currently writing one
 ** for jxta-c, this temporary implementation uses the existing Jxta_vector.
 ** When hastable will be available, this implementation should use it.
 **/

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
/**
 ** Returns a messenger for a particular host (ip/port).
 ** If there was not yet a messenger for this host, create a new
 ** one. Otherwise, return the existing messenger.
 ** (2/14/2002 lomax@jxta.org)
 **/
static HttpClientMessenger *get_http_client_messenger(Jxta_transport_http * tp, char *host, int port)
{
    int i;
    int nbOfMessengers;
    Jxta_boolean found = FALSE;
    HttpClientMessenger *messenger;
    char *key;
    Jxta_status res;

    if ((tp == NULL) || (host == NULL)) {
        return NULL;
    }

    key = malloc(1024);
    apr_snprintf(key, 1024, "%s:%d", host, port);

    apr_thread_mutex_lock(tp->mutex);
    /**
     ** Search in the vector if there is a messenger for this host.
     **/
    nbOfMessengers = jxta_vector_size(tp->clientMessengers);
    for (i = 0; i < nbOfMessengers; ++i) {
        /**
         ** Note that jxta_vector_get_object_at automatically shares the object.
         ** If the messenger retrieved is not the right messenger, the object
         ** needs to be released.
         **/
        res = jxta_vector_get_object_at(tp->clientMessengers, JXTA_OBJECT_PPTR(&messenger), i);
        if (res != JXTA_SUCCESS) {
            continue;
        }

        if (strcmp(messenger->name, key) == 0) {
            /* Found the proper messenger */
            found = TRUE;
            break;
        }
        /* Release the messenger object */
        JXTA_OBJECT_RELEASE(messenger);
    }

    if (!found) {
        /**
         ** We did not find any messenger. Allocate a new one.
         **/
        messenger = http_client_messenger_new(tp, tp->proxy_host, tp->proxy_port, host, port);

        if (messenger != NULL) {
            /**
             ** We actually got a new messenger, insert it into the vector.
             **/
            res = jxta_vector_add_object_first(tp->clientMessengers, (Jxta_object *) messenger);
            if (res != JXTA_SUCCESS) {
                /* just log a warning message */
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot insert HttpClientMessenger into vector\n");
            }
        }
    }
    /**
     ** Return the messenger.
     **
     ** NOTE: there is no need here to share the object, since
     ** the handle we have is already shared and that this function
     ** is not going to use a reference to it anymore. The caller
     ** of get_client_messenger is responsible for releasing the object.
     ** (2/14/2002 lomax@jxta.org)
     **/

    apr_thread_mutex_unlock(tp->mutex);

    free(key);

    return messenger;
}

/* vim: set ts=4 sw=4 tw=130 et: */
