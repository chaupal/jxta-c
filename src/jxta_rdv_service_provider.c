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
 * $Id: jxta_rdv_service_provider.c,v 1.21 2005/11/21 01:19:23 mmx2005 Exp $
 */

static const char *__log_cat = "RdvProvider";

#include <stddef.h>

#include "jxta_log.h"
#include "jxta_peer_private.h"
#include "jxta_peergroup.h"
#include "jxta_pm.h"
#include "jxta_rdv_service_provider.h"
#include "jxta_rdv_service_provider_private.h"

_jxta_rdv_service_provider *jxta_rdv_service_provider_construct(_jxta_rdv_service_provider * self,
                                                                const _jxta_rdv_service_provider_methods * methods)
{
    apr_status_t res = APR_SUCCESS;

    self->methods = PTValid(methods, _jxta_rdv_service_provider_methods);
    self->thisType = "_jxta_rdv_service_provider";

    apr_pool_create(&self->pool, NULL);
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,        /* nested */
                                  self->pool);

    self->service = NULL;

    return self;
}

void jxta_rdv_service_provider_destruct(_jxta_rdv_service_provider * self)
{
    JXTA_OBJECT_RELEASE(self->service);

    JXTA_OBJECT_RELEASE(self->peerview);
    JXTA_OBJECT_RELEASE(self->localPeerIdJString);

    if (NULL != self->listener_propagate) {
        JXTA_OBJECT_RELEASE(self->listener_propagate);
        self->listener_propagate = NULL;
    }

    if (self->messageElementName) {
        free(self->messageElementName);
    }
    free(self->groupiduniq);

    /* Free the pool used to allocate the thread and mutex */
    apr_thread_mutex_destroy(self->mutex);
    apr_pool_destroy(self->pool);

    self->thisType = NULL;
}

Jxta_status jxta_rdv_service_provider_init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service)
{
    _jxta_rdv_service_provider *self = PTValid(provider, _jxta_rdv_service_provider);
    Jxta_PG *group = jxta_service_get_peergroup_priv((_jxta_service *) service);
    JString *string;
    Jxta_PGID *gid;
    Jxta_PID *pid;

    self->service = JXTA_OBJECT_SHARE(service);

    self->peerview = JXTA_OBJECT_SHARE(jxta_rdv_service_get_peerview_priv(service));

    jxta_PG_get_GID(group, &gid);
    jxta_id_get_uniqueportion(gid, &string);
    JXTA_OBJECT_RELEASE(gid);
    self->groupiduniq = strdup(jstring_get_string(string));
    JXTA_OBJECT_RELEASE(string);

    jxta_PG_get_PID(group, &pid);
    jxta_id_to_jstring(pid, &self->localPeerIdJString);
    JXTA_OBJECT_RELEASE(pid);

    /**
     ** Builds the element name contained into each propagated message.
     **/
    self->messageElementName = malloc(strlen(JXTA_RDV_PROPAGATE_ELEMENT_NAME) + strlen(self->groupiduniq) + 1);
    strcpy(self->messageElementName, JXTA_RDV_PROPAGATE_ELEMENT_NAME);
    strcat(self->messageElementName, self->groupiduniq);

    return JXTA_SUCCESS;
}

Jxta_status jxta_rdv_service_provider_start(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *self = PTValid(provider, _jxta_rdv_service_provider);

    /**
     ** Add the Rendezvous Service Message receiver listener
     **/

    self->listener_propagate = jxta_listener_new(jxta_rdv_service_provider_prop_listener, (void *) self, 10, 100);

    jxta_endpoint_service_add_listener(provider->service->endpoint,
                                       JXTA_RDV_PROPAGATE_SERVICE_NAME, self->groupiduniq, self->listener_propagate);

    jxta_listener_start(self->listener_propagate);

    return JXTA_SUCCESS;
}

Jxta_status jxta_rdv_service_provider_stop(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *self = PTValid(provider, _jxta_rdv_service_provider);

    jxta_endpoint_service_remove_listener(self->service->endpoint, JXTA_RDV_PROPAGATE_SERVICE_NAME, self->groupiduniq);

    jxta_listener_stop(self->listener_propagate);

    if (self->listener_propagate) {
        JXTA_OBJECT_RELEASE(self->listener_propagate);
        self->listener_propagate = NULL;
    }

    return JXTA_SUCCESS;
}

apr_pool_t *jxta_rdv_service_provider_get_pool_priv(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *self = (_jxta_rdv_service_provider *) provider;

    return self->pool;
}

void jxta_rdv_service_provider_lock_priv(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *self = (_jxta_rdv_service_provider *) provider;

    apr_thread_mutex_lock(self->mutex);
}

void jxta_rdv_service_provider_unlock_priv(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *self = (_jxta_rdv_service_provider *) provider;

    apr_thread_mutex_unlock(self->mutex);
}

_jxta_rdv_service *jxta_rdv_service_provider_get_service_priv(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *self = (_jxta_rdv_service_provider *) provider;

    return self->service;
}

/**
 ** This listener is called when a propagated message is received.
 **/
void JXTA_STDCALL jxta_rdv_service_provider_prop_listener(Jxta_object * obj, void *arg)
{
    Jxta_status res;
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_rdv_service_provider *self = PTValid(arg, _jxta_rdv_service_provider);
    Jxta_message_element *el = NULL;
    RendezVousPropagateMessage *pmsg;
    Jxta_endpoint_address *realDest;
    Jxta_bytevector *bytes;
    JString *string;
    Jxta_boolean message_seen = FALSE;

    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rendezvous received a propagated message [%p].\n", msg);

    res = jxta_message_get_element_1(msg, self->messageElementName, &el);

    if ((JXTA_SUCCESS != res) || (NULL == el)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "PropHdr element missing. Dropping msg [%p].\n", msg);
        return;
    }

    bytes = jxta_message_element_get_value(el);
    JXTA_OBJECT_RELEASE(el);
    string = jstring_new_3(bytes);
    JXTA_OBJECT_RELEASE(bytes);

    pmsg = RendezVousPropagateMessage_new();

    res = RendezVousPropagateMessage_parse_charbuffer(pmsg, jstring_get_string(string), jstring_length(string));
    JXTA_OBJECT_RELEASE(string);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Damaged PropHdr. Dropping msg [%p].\n", msg);
    } else {
        JString *msgid = RendezVousPropagateMessage_get_MessageId(pmsg);
        JString *svc_name = RendezVousPropagateMessage_get_DestSName(pmsg);
        JString *svc_param = RendezVousPropagateMessage_get_DestSParam(pmsg);
        Jxta_vector *vector = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Demux propagated message [%p] MessageId [%s] -> %s/%s\n",
                        msg, jstring_get_string(msgid), jstring_get_string(svc_name), jstring_get_string(svc_param));

        /* XXX 20050516 bondolo Do duplicate message removal based upon msgid here */


#if 0
        /* Check if we are already in the path */
        vector = RendezVousPropagateMessage_get_Path(pmsg);

        JXTA_OBJECT_CHECK_VALID(vector);

        if (vector != NULL) {
            unsigned int eachVisited;

            for (eachVisited = 0; eachVisited < jxta_vector_size(vector); eachVisited++) {
                JString *aPeer;

                res = jxta_vector_get_object_at(vector, JXTA_OBJECT_PPTR(&aPeer), eachVisited);

                if (res != JXTA_SUCCESS) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot retrieve peer_id from vector.\n");
                    continue;
                }

                if (0 == strcmp(jstring_get_string(aPeer), jstring_get_string(self->localPeerIdJString))) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Removing previously seen meessage [%p] msgid [%s]\n", msg,
                                    jstring_get_string(msgid));
                    message_seen = TRUE;
                    break;
                }

                JXTA_OBJECT_RELEASE(aPeer);
            }
            JXTA_OBJECT_RELEASE(vector);
        }
#endif

        JXTA_OBJECT_RELEASE(msgid);

        /* set the message destination */
        realDest =
            jxta_endpoint_address_new_2("jxta", self->groupiduniq, jstring_get_string(svc_name), jstring_get_string(svc_param));

        JXTA_OBJECT_RELEASE(svc_name);
        JXTA_OBJECT_RELEASE(svc_param);

        /* Invoke the endpoint service demux again with the new destination */
        if (!message_seen) {
            jxta_endpoint_service_demux_addr(self->service->endpoint, realDest, msg);
        }
        JXTA_OBJECT_RELEASE(realDest);
                
        /* XXX bondolo 20050925 handle adhoc repropagation here */
    }

    JXTA_OBJECT_RELEASE(pmsg);
}

Jxta_status jxta_rdv_service_provider_update_prophdr(Jxta_rdv_service_provider * provider, Jxta_message * msg,
                                                     const char *serviceName, const char *serviceParam, int ttl)
{
    Jxta_status res;
    Jxta_rdv_service_provider *self = PTValid(provider, _jxta_rdv_service_provider);
    Jxta_message_element *el = NULL;
    RendezVousPropagateMessage *pmsg = NULL;
    JString *propMsgStr = NULL;
    Jxta_vector *vector = NULL;
    JString *messageId;

    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Updating message [%p].\n", msg);

    /* Test arguments first */
    if ((ttl <= 0) || (NULL == serviceName)) {
        /* Invalid args. */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid arguments. Message is dropped\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /* Retrieve (if any) the propagate message element */

    res = jxta_message_get_element_1(msg, self->messageElementName, &el);

    if ((JXTA_SUCCESS == res) && (NULL != el)) {
        /* Recover the prop header element from the message */
        Jxta_bytevector *bytes = NULL;
        JString *propHdrStr = NULL;
        int hdrTTL;

        jxta_message_remove_element(msg, el);

        JXTA_OBJECT_CHECK_VALID(el);

        bytes = jxta_message_element_get_value(el);
        propHdrStr = jstring_new_3(bytes);
        JXTA_OBJECT_RELEASE(bytes);
        bytes = NULL;
        pmsg = RendezVousPropagateMessage_new();
        RendezVousPropagateMessage_parse_charbuffer(pmsg, jstring_get_string(propHdrStr), jstring_length(propHdrStr));
        JXTA_OBJECT_RELEASE(propHdrStr);
        JXTA_OBJECT_RELEASE(el);
        
        messageId = RendezVousPropagateMessage_get_MessageId(pmsg);

        hdrTTL = RendezVousPropagateMessage_get_TTL(pmsg);
        hdrTTL--;
        hdrTTL = (hdrTTL < ttl) ? hdrTTL : ttl;
        RendezVousPropagateMessage_set_TTL(pmsg, hdrTTL);
        ttl = hdrTTL;
    } else {
        messageId = message_id_new();

        /* Create a new propagate message element */
        pmsg = RendezVousPropagateMessage_new();
        /* Initialize it */
        RendezVousPropagateMessage_set_DestSName(pmsg, serviceName);
        RendezVousPropagateMessage_set_DestSParam(pmsg, serviceParam);
        RendezVousPropagateMessage_set_MessageId(pmsg, jstring_get_string(messageId));
        RendezVousPropagateMessage_set_TTL(pmsg, ttl);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Updating message [%p] ID [%s] ttl=%d \n", msg,
                    jstring_get_string(messageId), ttl);
    JXTA_OBJECT_RELEASE(messageId);

    if (ttl <= 0) {
        JXTA_OBJECT_RELEASE(pmsg);
        return JXTA_TTL_EXPIRED;
    }

    /* Add ourselves into the path */
    vector = RendezVousPropagateMessage_get_Path(pmsg);

    JXTA_OBJECT_CHECK_VALID(vector);

    if (vector == NULL) {
        vector = jxta_vector_new(1);
    }

    res = jxta_vector_add_object_first(vector, (Jxta_object *) self->localPeerIdJString);

    if (res != JXTA_SUCCESS) {
        /* We just display an error LOG message and keep going. */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed adding local peer into the path\n");
    }

    RendezVousPropagateMessage_set_Path(pmsg, vector);

    JXTA_OBJECT_RELEASE(vector);

    /* Add pmsg into the message */
    res = RendezVousPropagateMessage_get_xml(pmsg, &propMsgStr);
    JXTA_OBJECT_RELEASE(pmsg);

    JXTA_OBJECT_CHECK_VALID(propMsgStr);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot retrieve XML from advertisement.\n");
        return JXTA_INVALID_ARGUMENT;
    }

    el = jxta_message_element_new_1(self->messageElementName, "text/xml", jstring_get_string(propMsgStr),
                                    jstring_length(propMsgStr), NULL);
    if (NULL == el) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot create propagate message element.\n");
        JXTA_OBJECT_RELEASE(propMsgStr);
        return JXTA_FAILED;
    }

    JXTA_OBJECT_RELEASE(propMsgStr);
    JXTA_OBJECT_CHECK_VALID(el);

    jxta_message_add_element(msg, el);

    JXTA_OBJECT_RELEASE(el);

    return JXTA_SUCCESS;
}

Jxta_status jxta_rdv_service_provider_prop_to_peers(Jxta_rdv_service_provider * provider, Jxta_message * msg,
                                                    Jxta_boolean andEndpoint)
{
    Jxta_status res;
    Jxta_rdv_service_provider *self = PTValid(provider, _jxta_rdv_service_provider);
    Jxta_vector *vector = NULL;

    JXTA_OBJECT_CHECK_VALID(msg);

    /* We are going to modify the message so we clone it */
    msg = jxta_message_clone(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Propagating message [%p].\n", msg);

    /*
     * propagate via physical transport if any transport
     * is available and support multicast
     */
    if (andEndpoint) {
        res =
            jxta_endpoint_service_propagate(provider->service->endpoint, msg, JXTA_RDV_PROPAGATE_SERVICE_NAME, self->groupiduniq);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint service propagation status= %d\n", res);
    }

    /* propagate to the connected peers (either our clients or our rdv) */
    res = PROVIDER_VTBL(self)->get_peers((Jxta_rdv_service_provider *) self, &vector);

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
                destAddr =
                    jxta_endpoint_address_new_2(jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                                                jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address),
                                                JXTA_RDV_PROPAGATE_SERVICE_NAME, self->groupiduniq);

                /* Send the message */
                res =
                    jxta_endpoint_service_send(jxta_service_get_peergroup_priv
                                               ((_jxta_service *) provider->service), provider->service->endpoint, msg, destAddr);
                if (res != JXTA_SUCCESS) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to propagate message [%p]. Expiring %s://%s\n",
                                    msg,
                                    jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                                    jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address));

                    jxta_peer_set_expires((Jxta_peer *) peer, 0);
                } else {
                    peerCount++;
                }

                JXTA_OBJECT_RELEASE(destAddr);
            }

            JXTA_OBJECT_RELEASE(peer);
        }
        JXTA_OBJECT_RELEASE(vector);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Propagated message [%p] to %d peers.\n", msg, peerCount);
    }

    JXTA_OBJECT_RELEASE(msg);

    return JXTA_SUCCESS;
}

/* vi: set ts=4 sw=4 tw=130 et: */
