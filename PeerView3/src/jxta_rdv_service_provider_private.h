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

#ifndef __JXTA_RDV_SERVICE_PROVIDER_PRIVATE_H__
#define __JXTA_RDV_SERVICE_PROVIDER_PRIVATE_H__

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_object.h"
#include "jxta_object_type.h"
#include "jxta_rdv_service_private.h"
#include "jxta_rdv_service_provider.h"
#include "jxta_rdv_diffusion_msg.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
* The set of methods that a rdv provider object must implement.
**/
struct _jxta_rdv_service_provider_methods {
    Extends_nothing;            /* could extend Jxta_object but that'd be overkill */

    Jxta_status(*init) (Jxta_rdv_service_provider * provider, _jxta_rdv_service * service);
    Jxta_status(*start) (Jxta_rdv_service_provider * provider);
    Jxta_status(*stop) (Jxta_rdv_service_provider * provider);

    /**
     * Return the peer record for the specified peer id.
     * 
     * @param provider The instance of the Rendezvous Service Provider
     * @param peerid The peerID of the peer sought.
     * @param peer The result.
     * @return JXTA_SUCCESS if peer is returned otherwise JXTA_ITEM_NOTFOUND.
     **/
    Jxta_status(*get_peer) (Jxta_rdv_service_provider * provider, Jxta_id * peerid, Jxta_peer ** peer);

    /******
     ** Get the list of peers used by the Rendezvous Service with their status.
     * 
     * @param service a pointer to the instance of the Rendezvous Service
     * @param pAdv a pointer to a pointer that contains the list of peers
     * @return error code.
     *******/
    Jxta_status(*get_peers) (Jxta_rdv_service_provider * provider, Jxta_vector ** peerlist);

    /******
     * Propagates a message within the PeerGroup for which the instance of the 
     * Rendezvous Service is running in.
     *
     * @param service a pointer to the instance of the Rendezvous Service
     * @param msg the Jxta_message* to propagate.
     * @param addr An EndpointAddress that defines on which address the message is destinated
     * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
     * @param ttl Maximum number of peers the propagated message can go through.
     * @return error code.
     *******/
    Jxta_status(*propagate) (Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                             const char *serviceParam, int ttl);

     /**
     * Walk a message within the PeerGroup for which the instance of the 
     * Rendezvous Service is running in.
     *
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param msg the Jxta_message* to propagate.
     * @param serviceName pointer to a string containing the name of the service 
     * on which the listener is listening on.
     * @param serviceParam pointer to a string containing the parameter associated
     * to the serviceName.
     * @param target_hash The hash value which is being sought.
     * @return error code.
     **/
    Jxta_status(*walk) (Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                        const char *serviceParam, const char *target_hash);
};

typedef struct _jxta_rdv_service_provider_methods _jxta_rdv_service_provider_methods;

/**
 * Easy access to the vtbl.
 **/
#define PROVIDER_VTBL(provider) ((_jxta_rdv_service_provider_methods*) ((provider)->methods))

/**
    Base for all Rendezvous provider implementations.
**/
struct _jxta_rdv_service_provider {
    Extends(Jxta_object);
    _jxta_rdv_service_provider_methods const *methods;  /* Pointer to the implementation set of methods. */

    _jxta_rdv_service *service;

    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;

    Jxta_PG *pg;
    apr_thread_pool_t *thread_pool;

    char *gid_uniq_str;
    char *assigned_id_str;
    Jxta_id *local_peer_id;
    Jxta_PA *local_pa;

    Jxta_peerview *peerview;

    /** Pipe service of the *PARENT* peergroup **/
    Jxta_PG *parentgroup;
    Jxta_PGID *parentgid;
    Jxta_pipe_service *pipes;
    Jxta_pipe *seed_pipe;
    Jxta_object *prop_cookie;
};

typedef struct _jxta_rdv_service_provider _jxta_rdv_service_provider;

/**
    Cosntructor for Rendezvous providers.
**/
extern _jxta_rdv_service_provider *jxta_rdv_service_provider_construct(_jxta_rdv_service_provider * self,
                                                                       const _jxta_rdv_service_provider_methods * methods,
                                                                       apr_pool_t * pool);

/**
    Standard Destructor
**/
extern void jxta_rdv_service_provider_destruct(_jxta_rdv_service_provider * self);

/**

**/
extern Jxta_status jxta_rdv_service_provider_init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service);

extern Jxta_status jxta_rdv_service_provider_start(Jxta_rdv_service_provider * provider);

extern Jxta_status jxta_rdv_service_provider_stop(Jxta_rdv_service_provider * provider);

extern apr_pool_t *jxta_rdv_service_provider_get_pool_priv(Jxta_rdv_service_provider * provider);

extern void jxta_rdv_service_provider_lock_priv(Jxta_rdv_service_provider * provider);

extern void jxta_rdv_service_provider_unlock_priv(Jxta_rdv_service_provider * provider);

/**
    
**/
extern _jxta_rdv_service *jxta_rdv_service_provider_get_service_priv(Jxta_rdv_service_provider * provider);

/**
    
**/
extern Jxta_peerview *jxta_rdv_service_provider_get_peerview_priv(Jxta_rdv_service_provider * provider);

/**
    
**/
extern Jxta_status jxta_rdv_service_provider_get_diffusion_header(Jxta_message * msg, Jxta_rdv_diffusion ** header);

/**
    
**/
extern Jxta_status jxta_rdv_service_provider_set_diffusion_header(Jxta_message * msg, Jxta_rdv_diffusion * header);

/**
    
**/
extern Jxta_status jxta_rdv_service_provider_prop_handler(Jxta_rdv_service_provider * myself, Jxta_message * msg,
                                                          Jxta_rdv_diffusion * header);

/**
*   Propagates a message to the set of peers associated with this rendezvous
*   provider instance. These peers will typically be either the rdv server or
*   rdv clients.
**/
extern Jxta_status jxta_rdv_service_provider_prop_to_peers(Jxta_rdv_service_provider * provider, Jxta_message * msg);

/**
*   Broadcast a seed request.
**/
extern Jxta_status provider_send_seed_request(Jxta_rdv_service_provider * me);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif

/* vim: set ts=4 sw=4 et tw=130: */
