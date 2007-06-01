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
 *    permission, please contact Project JXTA at tls://www.jxta.org.
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
 * <tls://www.jxta.org/>.
 *
 * This license is based on the BSD license adopted by the Apache Foundation.
 *
 * $Id: jxta_transport_tls.c,v 1.2 2007/04/24 23:46:42 mmx2005 Exp $
 */

static const char *__log_cat = "TLS_TRANSPORT";

#include <stdlib.h> /* for atoi */

#ifndef WIN32
#include <signal.h> /* for sigaction */
#endif

#include "jxta_apr.h"
#include "jpr/jpr_excep_proto.h"

#include "apr_lib.h"

#include "jstring.h"
#include "jxta_types.h"
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_transport_private.h"
#include "jxta_peergroup.h"
#include "trailing_average.h"
#include "jxta_pa.h"
#include "jxta_svc.h"

#include "jxta_transport_tls_private.h"
#include "jxta_transport_tls.h"

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "jxta_tls_config_adv.h"

#include "apr_file_info.h"

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

    Jxta_PG *group;
    Jxta_vector *clientMessengers;
    Jxta_endpoint_service *endpoint;
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
    Jxta_listener *listener_tlstransport;

    Jxta_endpoint_address *public_address;

    Jxta_transport_tls_connections *tls_connections;

    apr_hash_t *handshakes;

    SSL_CTX *ctx;

} Jxta_transport_tls;

/**
 ** The TLS client JxtaEndpointMessenger structure
 ** This structure extends JxtaEndpointMessenger, which is
 ** itself a Jxta_object, so, the TlsClientMessenger is also a
 ** Jxta_object (needs to be released when not used anymore.
 **/

typedef struct {
    struct _JxtaEndpointMessenger messenger;
    char *host;
    Jxta_boolean connected;
    apr_thread_mutex_t *mutex;
    Jxta_transport_tls *tp;
    apr_pool_t *pool;
} TlsClientMessenger;

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static TlsClientMessenger *tls_client_messenger_new(Jxta_transport_tls * tp, const char *host);
static TlsClientMessenger *get_tls_client_messenger(Jxta_transport_tls * tp, char *host);

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
static void JXTA_STDCALL tlstransport_listener(Jxta_object * obj, void *arg);

static void tls_free(Jxta_object * obj);

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
typedef Jxta_transport_methods Jxta_transport_tls_methods;

const Jxta_transport_tls_methods jxta_transport_tls_methods = {
    {
     "Jxta_module_methods",
     init,
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

/******************
 * Module methods *
 *****************/

/******************************************************************************/
/*                                                                            */
/******************************************************************************/


static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    apr_status_t res;
    Jxta_transport_tls *self;
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    JString *isclient = NULL;

    Jxta_id *PID = NULL;

    char *addr_string;

    JString *addr_jstring;

    self = (Jxta_transport_tls *) module;
    PTValid(self, Jxta_transport_tls);

    /* Allocate a pool for our apr needs */
    res = apr_pool_create(&self->pool, jxta_PG_pool_get(group));
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

    jxta_PG_get_configadv(group, &conf_adv);
    if (conf_adv == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to get config adv.\n");
        return JXTA_CONFIG_NOTFOUND;
    }

    /* generate public address */
    PID = jxta_PA_get_PID(conf_adv);
    jxta_id_get_uniqueportion(PID, &addr_jstring);
    addr_string = (char *) jstring_get_string(addr_jstring);

    self->public_address = jxta_endpoint_address_new_2("jxtatls", addr_string, NULL, NULL);

    JXTA_OBJECT_RELEASE(conf_adv);
    JXTA_OBJECT_RELEASE(PID);
    JXTA_OBJECT_RELEASE(addr_jstring);


    self->group = group;
    jxta_PG_get_endpoint_service(group, &(self->endpoint));


    self->listener_tlstransport = jxta_listener_new(tlstransport_listener, (void *) self, 10, 100);


    /*jxta_endpoint_service_add_transport(self->endpoint, self); */

    jxta_endpoint_service_add_listener(self->endpoint, TLS_SERVICE_NAME, NULL, self->listener_tlstransport);


    apr_thread_yield();

    self->tls_connections = tls_connections_new(self->group, self->endpoint);

    self->handshakes = apr_hash_make(self->pool);

    /* SSL-stuff */
    SSL_library_init();
    SSL_load_error_strings();

    self->ctx = NULL;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_transport_tls_init_certificates(Jxta_PG * group, const char *pwd)
{
    Jxta_TlsConfigAdvertisement *tlsConfig = NULL;

    Jxta_PA *conf_adv = NULL;
    EVP_PKEY *pub_key = NULL;
    SSL_CTX *ctx = NULL;

    Jxta_transport_tls *myself = NULL;
    Jxta_endpoint_service *endpoint = NULL;

    jxta_PG_get_endpoint_service(group, &endpoint);

    JXTA_OBJECT_CHECK_VALID(endpoint);

    myself = (Jxta_transport_tls*)jxta_endpoint_service_lookup_transport(endpoint, "jxtatls");

    JXTA_OBJECT_RELEASE(endpoint);

    if (myself == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to get tls service.\n");
        return JXTA_FAILED;
    }

    if (pwd == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "No password provided.\n");
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_PG_get_configadv(myself->group, &conf_adv);
    if (conf_adv == NULL) {
        JXTA_OBJECT_RELEASE(myself);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to get config adv.\n");

        return JXTA_CONFIG_NOTFOUND;
    }

    {
        Jxta_svc *svc = NULL;

        jxta_PA_get_Svc_with_id(conf_adv, jxta_tlsproto_classid, &svc);

        if (NULL != svc) {
            tlsConfig = jxta_svc_get_TlsConfig(svc);
            JXTA_OBJECT_RELEASE(svc);
        } else {
            JXTA_OBJECT_RELEASE(myself);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to get tls service configuration...\n");

            return JXTA_FAILED;
        }
    }

    ctx = SSL_CTX_new(TLSv1_method());

    if (ctx == NULL) {
        JXTA_OBJECT_RELEASE(myself);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_FATAL, "Can't create SSL-CTX-structure!\n");

        return JXTA_FAILED;
    }

    if (tlsConfig != NULL) {
        {
            EVP_PKEY *pkey = NULL;

            pkey = jxta_TLSConfigAdvertisement_get_PrivateKey(tlsConfig, pwd);

            if (pkey == NULL) {
                unsigned long err;

                SSL_CTX_free(ctx);
                JXTA_OBJECT_RELEASE(myself);

                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "can't read private key\n");

                err = ERR_GET_REASON(ERR_peek_error());

                if (err == PEM_R_BAD_BASE64_DECODE)
                    return JXTA_INVALID_ARGUMENT;
                else
                    return JXTA_NOT_CONFIGURED;
            }

            SSL_CTX_use_PrivateKey(ctx, pkey);
        }

        {
            X509 *x509 = NULL;

            x509 = jxta_TLSConfigAdvertisement_get_Certificate(tlsConfig);

            if (x509 == NULL) {
                SSL_CTX_free(ctx);
                JXTA_OBJECT_RELEASE(myself);

                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "can't read certificate\n");
                return JXTA_NOT_CONFIGURED;
            }

            pub_key = X509_get_pubkey(x509);

            SSL_CTX_use_certificate(ctx, x509);
        }

        if (!SSL_CTX_check_private_key(ctx)) {
            SSL_CTX_free(ctx);
            JXTA_OBJECT_RELEASE(myself);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_FATAL,
                            "Public- and Private-keys dont match! You might have a serious security-problem!\n");
            return JXTA_FAILED;
        }

        {
            JString *ca_file = jxta_TLSConfigAdvertisement_get_CAChainFile(tlsConfig);

            if (ca_file != NULL) {
                void *data = NULL;
                int size = 0;

                {
                    apr_file_t *file = NULL;
                    apr_finfo_t *info = malloc(sizeof(apr_finfo_t));

                    apr_file_open(&file, jstring_get_string(ca_file), APR_READ, APR_OS_DEFAULT, myself->pool);

                    apr_file_info_get(info, APR_FINFO_SIZE, file);
                    size = (int) info->size + 2;

                    data = malloc(size);

                    apr_file_read(file, data, &size);

                    apr_file_close(file);

                    free(info);
                }

                {
                    EVP_MD_CTX *evp_ctx = NULL;
                    BIGNUM *bn_sig = NULL;
                    JString *sig = NULL;
                    int sig_size;
                    unsigned char *bin_sig = NULL;

                    evp_ctx = EVP_MD_CTX_create();
                    bn_sig = BN_new();;

                    sig = jstring_new_0();

                    {
                        JString *tmp = jxta_TLSConfigAdvertisement_get_Signature(tlsConfig);

                        jstring_append_1(sig, tmp);

                        JXTA_OBJECT_RELEASE(tmp);
                    }

                    apr_collapse_spaces((char *) jstring_get_string(sig), jstring_get_string(sig));

                    BN_hex2bn(&bn_sig, jstring_get_string(sig));

                    bin_sig = malloc(BN_num_bytes(bn_sig));

                    sig_size = BN_bn2bin(bn_sig, bin_sig);

                    EVP_VerifyInit(evp_ctx, EVP_md5());
                    EVP_VerifyUpdate(evp_ctx, data, size);

                    if (EVP_VerifyFinal(evp_ctx, bin_sig, sig_size, pub_key) != 1) {
                        SSL_CTX_free(ctx);
                        JXTA_OBJECT_RELEASE(myself);

                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "ChainFile-Signature wrong!!\n");
                        return JXTA_FAILED;
                    }

                    BN_free(bn_sig);
                    EVP_MD_CTX_destroy(evp_ctx);

                    free(bin_sig);

                    JXTA_OBJECT_RELEASE(sig);
                }

                SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, NULL);

                if (!SSL_CTX_load_verify_locations(ctx, jstring_get_string(ca_file), NULL)) {
                    SSL_CTX_free(ctx);
                    JXTA_OBJECT_RELEASE(myself);

                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "can't load trusted CAs\n");
                    return JXTA_FAILED;
                }

                SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(jstring_get_string(ca_file)));

                JXTA_OBJECT_RELEASE(ca_file);

                free(data);
            }
        }

        JXTA_OBJECT_RELEASE(tlsConfig);
    }

    myself->ctx = ctx;

    JXTA_OBJECT_RELEASE(myself);

    return JXTA_SUCCESS;
}

static Jxta_boolean tls_transport_is_handshake(Jxta_transport_tls * me, Jxta_message * msg, const char *source)
{
    Jxta_message_element *el;
    Jxta_boolean ret = FALSE;

    Jxta_transport_tls *myself = PTValid(me, Jxta_transport_tls);

    jxta_message_get_element_2(msg, TLS_NAMESPACE, "1", &el);

    if (el != NULL) {
        unsigned int last_handshake_time = 0;
        unsigned int handshake_time = 0;

        {
            Jxta_bytevector *vec = jxta_message_element_get_value(el);

            const char *hs_ptr = jxta_bytevector_content_ptr(vec);

            if ((hs_ptr[0] == 22) && (hs_ptr[5] == 1)) {
                last_handshake_time = (unsigned int) apr_hash_get(myself->handshakes, source, APR_HASH_KEY_STRING);
                memcpy(&handshake_time, hs_ptr + 11, 4);

                handshake_time = ntohl(handshake_time);

                if (last_handshake_time < handshake_time) {
                    apr_hash_set(myself->handshakes, source, APR_HASH_KEY_STRING, (void *) handshake_time);

                    ret = TRUE;
                }
            }

            JXTA_OBJECT_RELEASE(vec);
            JXTA_OBJECT_RELEASE(el);
        }
    }

    return ret;
}

int dcounter = 0;

static void JXTA_STDCALL tlstransport_listener(Jxta_object * obj, void *arg)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_transport_tls *self = PTValid(arg, Jxta_transport_tls);

    Jxta_message *msg = (Jxta_message *) obj;

    Jxta_transport_tls_connection *connection;

    Jxta_endpoint_address *source = jxta_message_get_source(msg);

    const char *dest_address = jxta_endpoint_address_get_protocol_address(source);

    if (self->ctx == NULL)
        return;

    if (tls_transport_is_handshake(self, msg, dest_address)) {
        tls_connections_remove_connection(self->tls_connections, dest_address);

        connection = tls_connection_new(self->group, self->endpoint, dest_address, self->tls_connections, self->ctx);

        if (connection == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Could not create tls-connection...\n");

            return;
        }

        tls_connections_add_connection(self->tls_connections, connection);
    } else {
        connection = tls_connections_get_connection(self->tls_connections, dest_address);
    }

    if (connection != NULL)
        tls_connection_receive(connection, msg);

    JXTA_OBJECT_RELEASE(connection);
    JXTA_OBJECT_RELEASE(source);
}

Jxta_status jxta_transport_tls_connect(Jxta_transport_tls * me, Jxta_endpoint_address * dest_addr, apr_interval_time_t timeout)
{
    Jxta_transport_tls *myself = (Jxta_transport_tls *) me;
    Jxta_transport_tls_connection *connection;

    Jxta_status res = JXTA_SUCCESS;

    const char *dest = jxta_endpoint_address_get_protocol_address(dest_addr);

    if (myself->ctx == NULL) {
        return JXTA_NOT_CONFIGURED;
    }

    connection = tls_connections_get_connection(myself->tls_connections, dest);

    if (connection == NULL) {
        connection = tls_connection_new(myself->group, myself->endpoint, dest, myself->tls_connections, myself->ctx);
        tls_connections_add_connection(myself->tls_connections, connection);

        tls_connection_initiate_handshake(connection);

        res = tls_connection_wait_for_handshake(connection, 3000000);

        if (res != JXTA_SUCCESS)
            return res;
    }

    JXTA_OBJECT_RELEASE(connection);

    return res;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status start(Jxta_module * module, const char *argv[])
{
    Jxta_transport_tls *self;

    self = (Jxta_transport_tls *) module;
    PTValid(self, Jxta_transport_tls);

    jxta_endpoint_service_add_transport(self->endpoint, (Jxta_transport *) self);

    jxta_listener_start(self->listener_tlstransport);

    return JXTA_SUCCESS;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void stop(Jxta_module * module)
{
    Jxta_transport_tls *self;

    self = (Jxta_transport_tls *) module;
    PTValid(self, Jxta_transport_tls);

    JXTA_OBJECT_RELEASE(self->tls_connections);

    SSL_CTX_free(self->ctx);

    jxta_endpoint_service_remove_transport(self->endpoint, (Jxta_transport *) self);
    jxta_listener_stop(self->listener_tlstransport);
}

/**************************
 * TLS Transport methods *
 *************************/

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static JString *name_get(Jxta_transport * self)
{
    PTValid(self, Jxta_transport_tls);

    return jstring_new_2("jxtatls");
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * t)
{
    Jxta_transport_tls *self = (Jxta_transport_tls *) t;
    PTValid(self, Jxta_transport_tls);

    return JXTA_OBJECT_SHARE(self->public_address);
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

    Jxta_transport_tls *self = (Jxta_transport_tls *) t;
    PTValid(self, Jxta_transport_tls);

    protocol_address = (char *) jxta_endpoint_address_get_protocol_address(dest);

    colon = strchr(protocol_address, ':');
    if (colon && (*(colon + 1))) {
        host = (char *) malloc(colon - protocol_address + 1);
        host[0] = 0;
        strncat(host, protocol_address, colon - protocol_address);
    } else {
        host = strdup(protocol_address);
    }

    messenger = (JxtaEndpointMessenger *) get_tls_client_messenger(self, host);

    JXTA_OBJECT_RELEASE(messenger->address);

    messenger->address = JXTA_OBJECT_SHARE(dest);
    free(host);
    return messenger;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean ping(Jxta_transport * t, Jxta_endpoint_address * addr)
{
    PTValid(t, Jxta_transport_tls);
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
    Jxta_transport_tls *self = (Jxta_transport_tls *) tr;
    PTValid(self, Jxta_transport_tls);
    return FALSE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean allow_routing_p(Jxta_transport * tr)
{
    Jxta_transport_tls *self = (Jxta_transport_tls *) tr;
    PTValid(self, Jxta_transport_tls);
    return TRUE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean connection_oriented_p(Jxta_transport * tr)
{
    Jxta_transport_tls *self = (Jxta_transport_tls *) tr;
    PTValid(self, Jxta_transport_tls);
    return TRUE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void jxta_transport_tls_construct(Jxta_transport_tls * self, Jxta_transport_tls_methods * methods)
{
    PTValid(methods, Jxta_transport_methods);
    jxta_transport_construct((Jxta_transport *) self, (Jxta_transport_methods *) methods);
    self->_super.metric = 7;    /* higher than tcp-metric */
    self->_super.direction = JXTA_OUTBOUND;

    self->thisType = "Jxta_transport_tls";

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
void jxta_transport_tls_destruct(Jxta_transport_tls * self)
{
    PTValid(self, Jxta_transport_tls);

    JXTA_OBJECT_RELEASE(self->endpoint);

    JXTA_OBJECT_RELEASE(self->group);

    JXTA_OBJECT_RELEASE(self->tls_connections);

    jxta_endpoint_service_remove_listener(self->endpoint, TLS_SERVICE_NAME, NULL);

    JXTA_OBJECT_RELEASE(self->listener_tlstransport);

    JXTA_OBJECT_RELEASE(self->public_address);

    JXTA_OBJECT_RELEASE(self->clientMessengers);

    if (self->pool) {
        apr_thread_mutex_destroy(self->mutex);
        apr_pool_destroy(self->pool);
    }

    jxta_transport_destruct((Jxta_transport *) self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Destruction finished\n");
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void tls_free(Jxta_object * obj)
{
    jxta_transport_tls_destruct((Jxta_transport_tls *) obj);
    free((void *) obj);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
Jxta_transport_tls *jxta_transport_tls_new_instance(void)
{
    Jxta_transport_tls *self = (Jxta_transport_tls *) malloc(sizeof(Jxta_transport_tls));
    memset(self, 0, sizeof(Jxta_transport_tls));
    JXTA_OBJECT_INIT(self, tls_free, NULL);
    jxta_transport_tls_construct(self, (Jxta_transport_tls_methods *) & jxta_transport_tls_methods);
    return self;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status messenger_send(JxtaEndpointMessenger * mes, Jxta_message * msg)
{
    TlsClientMessenger *self = (TlsClientMessenger *) mes;
    Jxta_transport_tls_connection *connection;

    Jxta_status res;

    Jxta_endpoint_address *src = jxta_transport_publicaddr_get((Jxta_transport *) self->tp);

    jxta_message_set_destination(msg, self->messenger.address);
    jxta_message_set_source(msg, src);

    JXTA_OBJECT_RELEASE(src);

    if (self->tp->ctx == NULL) {
        return JXTA_NOT_CONFIGURED;
    }

    connection =
        tls_connections_get_connection(self->tp->tls_connections,
                                       (char *) jxta_endpoint_address_get_protocol_address(self->messenger.address));

    if (connection == NULL) {
        connection = tls_connection_new(self->tp->group, self->tp->endpoint,
                                        (char *) jxta_endpoint_address_get_protocol_address(self->messenger.address),
                                        self->tp->tls_connections, self->tp->ctx);

        if (connection == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Could not create tls-connection!");
            return JXTA_FAILED;
        }

        tls_connections_add_connection(self->tp->tls_connections, connection);

        tls_connection_initiate_handshake(connection);

        res = tls_connection_wait_for_handshake(connection, 3000000);

        if (res != JXTA_SUCCESS)
            return res;
    }

    res = tls_connection_send(connection, msg, 3000000);

    JXTA_OBJECT_RELEASE(connection);

    return res;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void tls_client_messenger_free(Jxta_object * addr)
{
    TlsClientMessenger *self = (TlsClientMessenger *) addr;

    if (self == NULL) {
        return;
    }

    /*
     * We need to take the lock in order to make sure that nobody
     * else is using the object. However, since we are destroying
     * the object, there is no need to unlock. 
     */
    apr_thread_mutex_lock(self->mutex);

    /*trailing_average_free(self->trailing_average); */


    JXTA_OBJECT_RELEASE(self->messenger.address);

    /* Free the pool containing the mutex */
    apr_pool_destroy(self->pool);

    /* Free the host */
    free(self->host);

    /* Free the object itself */
    free(self);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static TlsClientMessenger *tls_client_messenger_new(Jxta_transport_tls * tp,    /*const char *proxy_host, int proxy_port, */
                                                    const char *host)
{   /*, int port) */
    TlsClientMessenger *self;
    apr_status_t res;

    /* Create the object */
    self = (TlsClientMessenger *) calloc(1, sizeof(TlsClientMessenger));

    if (self == NULL) {
        /* No more memory */
        return NULL;
    }

    /* Initialize it. */
    JXTA_OBJECT_INIT(self, tls_client_messenger_free, NULL);

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
    self->host = malloc(strlen(host) + 1);
    strcpy(self->host, host);

    return self;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
/**
 ** Returns a messenger for a particular host (ip/port).
 ** If there was not yet a messenger for this host, create a new
 ** one. Otherwise, return the existing messenger.
 ** (2/14/2002 lomax@jxta.org)
 **/
static TlsClientMessenger *get_tls_client_messenger(Jxta_transport_tls * tp, char *host)
{
    unsigned int i;
    Jxta_boolean found = FALSE;
    TlsClientMessenger *messenger;
    Jxta_status res;

    if ((tp == NULL) || (host == NULL)) {
        return NULL;
    }

    apr_thread_mutex_lock(tp->mutex);
    /**
     ** Search in the vector if there is a messenger for this host.
     **/
    for (i = 0; i < jxta_vector_size(tp->clientMessengers); ++i) {
        /**
         ** Note that jxta_vector_get_object_at automatically shares the object.
         ** If the messenger retrieved is not the right messenger, the object
         ** needs to be released.
         **/
        res = jxta_vector_get_object_at(tp->clientMessengers, JXTA_OBJECT_PPTR(&messenger), i);
        if (res != JXTA_SUCCESS) {
            continue;
        }

        if (strcmp(messenger->host, host) == 0) {
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
        messenger = tls_client_messenger_new(tp, host);

        if (messenger != NULL) {
            /**
             ** We actually got a new messenger, insert it into the vector.
             **/
            res = jxta_vector_add_object_first(tp->clientMessengers, (Jxta_object *) messenger);
            if (res != JXTA_SUCCESS) {
                /* just log a warning message */
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot insert TlsClientMessenger into vector\n");
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

    return messenger;
}

/* vim: set ts=4 sw=4 tw=130 et: */
