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

static const char *__log_cat = "RdvProvider";

#include <stddef.h>

#include <openssl/sha.h>
#include <assert.h>

#include "jxta_log.h"
#include "jxta_peer_private.h"
#include "jxta_peergroup.h"
#include "jxta_lease_request_msg.h"
#include "jxta_rdv_diffusion_msg.h"
#include "jxta_rdv_service.h"
#include "jxta_rdv_service_provider.h"
#include "jxta_rdv_service_provider_private.h"

/**
*   The maximum ttl value we will allow applications/services to set.
*/
const static unsigned int DEFAULT_MAX_ALLOWED_TTL = 2;

/**
*   If true then only the src peer will broadcast propagate messages.
*/
const static Jxta_boolean SELECTIVE_BROADCAST = TRUE;

static Jxta_status JXTA_STDCALL rdv_service_provider_prop_cb(Jxta_object * obj, void *arg);


_jxta_rdv_service_provider *jxta_rdv_service_provider_construct(_jxta_rdv_service_provider * myself,
                                                                const _jxta_rdv_service_provider_methods * methods,
                                                                apr_pool_t * pool)
{
    apr_status_t res = APR_SUCCESS;

    myself->methods = PTValid(methods, _jxta_rdv_service_provider_methods);
    myself->thisType = "_jxta_rdv_service_provider";

    apr_pool_create(&myself->pool, pool);

    /* Create the mutex */
    res = apr_thread_mutex_create(&myself->mutex, APR_THREAD_MUTEX_NESTED, myself->pool);

    myself->thread_pool = NULL;

    myself->local_peer_id = NULL;
    myself->local_pa = NULL;

    myself->service = NULL;

    myself->parentgroup = NULL;
    myself->parentgid = NULL;

    myself->pipes = NULL;
    myself->seed_pipe = NULL;

    return myself;
}

void jxta_rdv_service_provider_destruct(_jxta_rdv_service_provider * myself)
{
    JXTA_OBJECT_RELEASE(myself->service);

    if (NULL != myself->seed_pipe) {
        JXTA_OBJECT_RELEASE(myself->seed_pipe);
    }

    if (NULL != myself->pipes) {
        JXTA_OBJECT_RELEASE(myself->pipes);
    }

    if (NULL != myself->parentgroup) {
        JXTA_OBJECT_RELEASE(myself->parentgroup);
    }

    if (NULL != myself->parentgid) {
        JXTA_OBJECT_RELEASE(myself->parentgid);
    }

    JXTA_OBJECT_RELEASE(myself->peerview);

    free(myself->assigned_id_str);

    JXTA_OBJECT_RELEASE(myself->local_peer_id);
    JXTA_OBJECT_RELEASE(myself->local_pa);

    free(myself->gid_uniq_str);

    myself->thread_pool = NULL;

    /* Free the pool used to allocate the thread and mutex */
    apr_thread_mutex_destroy(myself->mutex);
    apr_pool_destroy(myself->pool);

    myself->thisType = NULL;
}

Jxta_status jxta_rdv_service_provider_init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service)
{
    _jxta_rdv_service_provider *myself = PTValid(provider, _jxta_rdv_service_provider);
    JString *string;
    Jxta_PGID *gid;
    Jxta_id *assigned_id = jxta_service_get_assigned_ID_priv((Jxta_service *) service);

    myself->pg = jxta_service_get_peergroup_priv((Jxta_service *) service);
    myself->thread_pool = jxta_PG_thread_pool_get(myself->pg);

    myself->service = JXTA_OBJECT_SHARE(service);

    jxta_id_to_jstring(assigned_id, &string);
    myself->assigned_id_str = strdup(jstring_get_string(string));
    JXTA_OBJECT_RELEASE(string);

    jxta_PG_get_PID(myself->pg, &myself->local_peer_id);
    jxta_PG_get_PA(myself->pg, &myself->local_pa);

    jxta_PG_get_GID(myself->pg, &gid);
    jxta_id_get_uniqueportion(gid, &string);
    JXTA_OBJECT_RELEASE(gid);
    myself->gid_uniq_str = strdup(jstring_get_string(string));
    JXTA_OBJECT_RELEASE(string);

    myself->peerview = jxta_rdv_service_get_peerview((Jxta_rdv_service *) service);

    return JXTA_SUCCESS;
}

Jxta_status jxta_rdv_service_provider_start(Jxta_rdv_service_provider * me)
{
    Jxta_status res = JXTA_SUCCESS;
    PTValid(me, _jxta_rdv_service_provider);

    /*
     * Add the Rendezvous Service Message receiver
     */
    res = jxta_PG_add_recipient(me->pg, &me->ep_cookie, RDV_V3_MSID, JXTA_RDV_PROPAGATE_SERVICE_NAME,
                                rdv_service_provider_prop_cb, me);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not register propagate callback.[%pp].\n", me);
    }

    return res;
}

Jxta_status jxta_rdv_service_provider_stop(Jxta_rdv_service_provider * me)
{
    Jxta_status res = JXTA_SUCCESS;
    PTValid(me, _jxta_rdv_service_provider);

    assert(NULL != me->ep_cookie);
    res = jxta_PG_remove_recipient(me->pg, me->ep_cookie);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not deregister propagate callback.[%pp].\n", me);
    }

    return res;
}

apr_pool_t *jxta_rdv_service_provider_get_pool_priv(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *myself = (_jxta_rdv_service_provider *) provider;

    return myself->pool;
}

void jxta_rdv_service_provider_lock_priv(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *myself = (_jxta_rdv_service_provider *) provider;

    apr_thread_mutex_lock(myself->mutex);
}

void jxta_rdv_service_provider_unlock_priv(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *myself = (_jxta_rdv_service_provider *) provider;

    apr_thread_mutex_unlock(myself->mutex);
}

_jxta_rdv_service *jxta_rdv_service_provider_get_service_priv(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *myself = (_jxta_rdv_service_provider *) provider;

    return myself->service;
}

/**
 *  This listener is called when a propagated message is received.
 **/
static Jxta_status JXTA_STDCALL rdv_service_provider_prop_cb(Jxta_object * obj, void *arg)
{
    Jxta_status res;
    Jxta_rdv_service_provider *myself = PTValid(arg, _jxta_rdv_service_provider);
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_rdv_diffusion *header;

    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rendezvous received a propagated message [%pp].\n", msg);

    res = jxta_rdv_service_provider_get_diffusion_header(msg, &header);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Header unavailable. Dropping msg [%pp].\n", msg);
    } else {
        char const *svc_name = jxta_rdv_diffusion_get_dest_svc_name(header);
        char const *svc_param = jxta_rdv_diffusion_get_dest_svc_param(header);
        unsigned int use_ttl;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Received propagated msg [%pp] -> %s/%s\n", msg, svc_name, svc_param);

        jxta_message_remove_element_2(msg, JXTA_RDV_NS_NAME, JXTA_RDV_DIFFUSION_ELEMENT_NAME);

        /* Adjust the Propagate TTL */
        use_ttl = jxta_rdv_diffusion_get_ttl(header);

        if (0 < use_ttl) {
            use_ttl--;
        }

        use_ttl = use_ttl < DEFAULT_MAX_ALLOWED_TTL ? use_ttl : DEFAULT_MAX_ALLOWED_TTL;

        jxta_rdv_diffusion_set_ttl(header, use_ttl);

        /* call the handler */
        res = jxta_rdv_service_provider_prop_handler(myself, msg, header);
    }

    JXTA_OBJECT_RELEASE(header);

    return res;
}

Jxta_status jxta_rdv_service_provider_get_diffusion_header(Jxta_message * msg, Jxta_rdv_diffusion ** header)
{
    Jxta_status res;
    Jxta_message_element *el = NULL;
    Jxta_bytevector *bytes;
    JString *string;

    res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_RDV_DIFFUSION_ELEMENT_NAME, &el);

    if ((JXTA_SUCCESS != res) || (NULL == el)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "No Diffusion header element.\n");
        return res;
    }

    bytes = jxta_message_element_get_value(el);
    JXTA_OBJECT_RELEASE(el);
    string = jstring_new_3(bytes);
    JXTA_OBJECT_RELEASE(bytes);

    *header = jxta_rdv_diffusion_new();

    res = jxta_rdv_diffusion_parse_charbuffer(*header, jstring_get_string(string), jstring_length(string));
    JXTA_OBJECT_RELEASE(string);

    return res;
}

Jxta_status jxta_rdv_service_provider_set_diffusion_header(Jxta_message * msg, Jxta_rdv_diffusion * header)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *header_xml = NULL;
    Jxta_message_element *el = NULL;

    res = jxta_rdv_diffusion_get_xml(header, &header_xml);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could generate diffusion header XML [%pp]\n", header);
        return res;
    }

    el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_RDV_DIFFUSION_ELEMENT_NAME, "text/xml", jstring_get_string(header_xml),
                                    jstring_length(header_xml), NULL);
    JXTA_OBJECT_RELEASE(header_xml);
    if (NULL == el) {
        res = JXTA_FAILED;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could generate diffusion header element [%pp]\n", header);
        return res;
    }

    jxta_message_remove_element_2(msg, JXTA_RDV_NS_NAME, JXTA_RDV_DIFFUSION_ELEMENT_NAME);

    res = jxta_message_add_element(msg, el);

    JXTA_OBJECT_RELEASE(el);

    return res;
}

Jxta_status jxta_rdv_service_provider_prop_handler(Jxta_rdv_service_provider * myself, Jxta_message * msg,
                                                   Jxta_rdv_diffusion * header)
{
    Jxta_status res = JXTA_SUCCESS;
    char const *svc_name = jxta_rdv_diffusion_get_dest_svc_name(header);
    char const *svc_param = jxta_rdv_diffusion_get_dest_svc_param(header);
    Jxta_endpoint_address *realDest = jxta_endpoint_address_new_2("jxta", myself->gid_uniq_str, svc_name, svc_param);
    Jxta_id *src_peer_id = NULL;

    /* Attach the diffusion header to the message */
     res = jxta_rdv_service_provider_set_diffusion_header(msg, header);

    if (JXTA_SUCCESS != res) {
        goto FINAL_EXIT;
    }

    /* Invoke the endpoint service demux again with the "real" destination */
    jxta_endpoint_service_demux_addr(myself->service->endpoint, realDest, msg);
    JXTA_OBJECT_RELEASE(realDest);

    if (JXTA_RDV_DIFFUSION_POLICY_TRAVERSAL == jxta_rdv_diffusion_get_policy(header)) {
        /* Traversals don't flood. */
        goto FINAL_EXIT;
    }

    if (0 == jxta_rdv_diffusion_get_ttl(header)) {
        /* No TTL left? give up! */
        goto FINAL_EXIT;
    }

    /* broadcast it? */
    src_peer_id = jxta_rdv_diffusion_get_src_peer_id(header);
    if (!SELECTIVE_BROADCAST || jxta_id_equals(src_peer_id, myself->local_peer_id)) {
        res = jxta_endpoint_service_propagate(myself->service->endpoint, msg, RDV_V3_MSID, JXTA_RDV_PROPAGATE_SERVICE_NAME);
    }
    JXTA_OBJECT_RELEASE(src_peer_id);

    /* Send the message to each connected peer. (rdvs or clients) */
    res = jxta_rdv_service_provider_prop_to_peers(myself, msg);

  FINAL_EXIT:
    return res;
}

Jxta_status jxta_rdv_service_provider_prop_to_peers(Jxta_rdv_service_provider * provider, Jxta_message * msg)
{
    Jxta_status res;
    Jxta_rdv_service_provider *myself = PTValid(provider, _jxta_rdv_service_provider);
    Jxta_vector *vector = NULL;

    JXTA_OBJECT_CHECK_VALID(msg);

    /* We are going to modify the message so we clone it */
    msg = jxta_message_clone(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Propagating message [%pp].\n", msg);

    /* propagate to the connected peers (either our clients or our rdv) */
    res = PROVIDER_VTBL(myself)->get_peers((Jxta_rdv_service_provider *) myself, &vector);

    JXTA_OBJECT_CHECK_VALID(vector);
    if ((res == JXTA_SUCCESS) && (vector != NULL)) {
        unsigned int sz = jxta_vector_size(vector);
        unsigned int eachPeer;
        unsigned int peerCount = 0;

        for (eachPeer = 0; eachPeer < sz; eachPeer++) {
            Jxta_peer *peer = NULL;

            res = jxta_vector_get_object_at(vector, JXTA_OBJECT_PPTR(&peer), eachPeer);

            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot retrieve Jxta_peer from vector.\n");
                continue;
            }

            PTValid(peer, _jxta_peer_entry);

            if (jxta_peer_get_expires(peer) > jpr_time_now()) {
                /* This is a connected peer, send the message to this peer */
                Jxta_endpoint_address *destAddr = NULL;

                /*
                 * Set the destination address of the message.
                 */
                res = jxta_PG_get_recipient_addr(provider->pg, 
                                                 jxta_endpoint_address_get_protocol_name(peer->address),
                                                 jxta_endpoint_address_get_protocol_address(peer->address),
                                                 RDV_V3_MSID, JXTA_RDV_PROPAGATE_SERVICE_NAME, &destAddr);
                if (JXTA_SUCCESS == res) {
                    res = jxta_endpoint_service_send_ex(provider->service->endpoint, msg, destAddr, JXTA_FALSE);
                }

                if (res != JXTA_SUCCESS) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, 
                                    "Failed to propagate message [%pp]. Expiring %s://%s\n", msg,
                                    jxta_endpoint_address_get_protocol_name(peer->address),
                                    jxta_endpoint_address_get_protocol_address(peer->address));
                    jxta_peer_set_expires(peer, 0);
                } else {
                    peerCount++;
                }

                JXTA_OBJECT_RELEASE(destAddr);
            }

            JXTA_OBJECT_RELEASE(peer);
        }
        JXTA_OBJECT_RELEASE(vector);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Propagated message [%pp] to %d peers.\n", msg, peerCount);
    }

    JXTA_OBJECT_RELEASE(msg);

    return JXTA_SUCCESS;
}

/**
 * Construct the associated peergroup RDV seed prop pipe advertisement.
 *
 *  @param pv Ourself.
 * 
 **/
static Jxta_pipe_adv *provider_get_seed_pipeadv(Jxta_rdv_service_provider * myself, Jxta_id * dest_gid)
{
    Jxta_status status;
    Jxta_id *pid;
    JString *hash_string;
    JString *pipeId;
    Jxta_pipe_adv *adv = NULL;
    unsigned char hash[SHA_DIGEST_LENGTH];

    jxta_id_to_jstring(dest_gid, &hash_string);

    jstring_append_2(hash_string, "#");
    jstring_append_2(hash_string, RDV_V3_MSID);
    jstring_append_2(hash_string, "/");
    jstring_append_2(hash_string, JXTA_RDV_LEASING_SERVICE_NAME);

    SHA1((unsigned char const *) jstring_get_string(hash_string), jstring_length(hash_string), hash);

    /* create pipe id in the designated group */
    status = jxta_id_pipeid_new_2(&pid, (Jxta_PGID *) dest_gid, hash, SHA_DIGEST_LENGTH);

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Error creating RDV prop pipe adv\n");
        return NULL;
    }

    jxta_id_to_jstring(pid, &pipeId);
    JXTA_OBJECT_RELEASE(pid);

    adv = jxta_pipe_adv_new();

    jxta_pipe_adv_set_Id(adv, jstring_get_string(pipeId));
    JXTA_OBJECT_RELEASE(pipeId);
    jxta_pipe_adv_set_Type(adv, JXTA_PROPAGATE_PIPE);
    jxta_pipe_adv_set_Name(adv, jstring_get_string(hash_string));

    JXTA_OBJECT_RELEASE(hash_string);

    return adv;
}

Jxta_status provider_send_seed_request(Jxta_rdv_service_provider * me)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_rdv_service_provider *myself = PTValid(me, _jxta_rdv_service_provider);
    Jxta_outputpipe *op = NULL;
    Jxta_PA *pa = NULL;
    Jxta_lease_request_msg *lease_request;
    JString *lease_request_xml = NULL;
    Jxta_message_element *elem = NULL;
    Jxta_message *msg = NULL;

    /*
     * Create a message with a lease request.
     */
    lease_request = jxta_lease_request_msg_new();

    if (NULL == lease_request) {
        return JXTA_NOMEM;
    }

    jxta_lease_request_msg_set_client_id(lease_request, myself->local_peer_id);
    jxta_lease_request_msg_set_requested_lease(lease_request, 0);
    jxta_lease_request_msg_set_referral_advs(lease_request, 0);
    jxta_PG_get_PA(myself->service->group, &pa);
    jxta_lease_request_msg_set_client_adv(lease_request, pa);
    JXTA_OBJECT_RELEASE(pa);
    jxta_lease_request_msg_set_client_adv_exp(lease_request, 5 * 60 * 1000UL);

    jxta_lease_request_msg_get_xml(lease_request, &lease_request_xml);
    JXTA_OBJECT_RELEASE(lease_request);

    elem = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_LEASE_REQUEST_ELEMENT_NAME,
                                      "text/xml", jstring_get_string(lease_request_xml), jstring_length(lease_request_xml), NULL);
    JXTA_OBJECT_RELEASE(lease_request_xml);

    if (NULL == elem) {
        return JXTA_NOMEM;
    }

    msg = jxta_message_new();

    if (NULL == msg) {
        JXTA_OBJECT_RELEASE(elem);
        return JXTA_NOMEM;
    }

    jxta_message_add_element(msg, elem);
    JXTA_OBJECT_RELEASE(elem);

    if (myself->seed_pipe) {
        /*
         * Send the message via the announce pipe.
         */
        res = jxta_pipe_get_outputpipe(myself->seed_pipe, &op);

        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Cannot get peerview adv outputpipe\n");
            JXTA_OBJECT_RELEASE(msg);
            return res;
        }

        if (op == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Cannot get output pipe\n");
            JXTA_OBJECT_RELEASE(msg);
            return JXTA_FAILED;
        }

        res = jxta_outputpipe_send(op, msg);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Sending RDV prop pipe failed: %d\n", res);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sent RDV prop pipe\n");
        }

        JXTA_OBJECT_RELEASE(op);
    } else {
        /*
         *   Send the message via the endpoint.
         */
        res =
            jxta_endpoint_service_propagate(myself->service->endpoint, msg, RDV_V3_MSID, JXTA_RDV_LEASING_SERVICE_NAME);
    }

    JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status open_adv_pipe(Jxta_rdv_service_provider * myself)
{
    Jxta_status res = JXTA_SUCCESS;

    /* If we are not running in the top group then register advertisement pipe in the parent group */
    if (NULL != myself->parentgroup) {
        Jxta_pipe_adv *adv = NULL;
        adv = provider_get_seed_pipeadv(myself, myself->parentgid);
        jxta_PG_get_pipe_service(myself->parentgroup, &myself->pipes);

        if (NULL != myself->pipes) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Waiting to bind peerview prop pipe\n");
            res = jxta_pipe_service_timed_connect(myself->pipes, adv, 15 * JPR_INTERVAL_ONE_SECOND, NULL, &myself->seed_pipe);

            if ((res != JXTA_SUCCESS) || (NULL == myself->seed_pipe)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not connect to peerview pipe\n");
            }
        }

        JXTA_OBJECT_RELEASE(adv);
    }

    return JXTA_SUCCESS;
}

/* vi: set ts=4 sw=4 tw=130 et: */
