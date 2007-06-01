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
 * $Id: jxta_peer.c,v 1.18 2005/10/27 01:55:27 slowhog Exp $
 */

#include "jxta_apr.h"
#include "jxta_types.h"
#include "jxta_peer.h"
#include "jxta_service.h"
#include "jxta_peer_private.h"

static _jxta_peer_entry *peer_entry_new(void);
static void peer_entry_delete(Jxta_object * addr);

const Jxta_Peer_entry_methods JXTA_PEER_ENTRY_METHODS = {
    "Jxta_Peer_entry_methods",
    jxta_peer_lock,
    jxta_peer_unlock,
    jxta_peer_get_peerid,
    jxta_peer_set_peerid,
    jxta_peer_get_address,
    jxta_peer_set_address,
    jxta_peer_get_adv,
    jxta_peer_set_adv,
    jxta_peer_get_expires,
    jxta_peer_set_expires
};


static _jxta_peer_entry *peer_entry_new(void)
{
    _jxta_peer_entry *self = (_jxta_peer_entry *) calloc(1, sizeof(_jxta_peer_entry));

    /* Initialize the object */
    JXTA_OBJECT_INIT(self, peer_entry_delete, 0);
    self->methods = &JXTA_PEER_ENTRY_METHODS;

    return peer_entry_construct(self);
}

static void peer_entry_delete(Jxta_object * addr)
{
    _jxta_peer_entry *self = (_jxta_peer_entry *) addr;

    peer_entry_destruct(self);

    free(self);
}

/**
* For calling by sub-classes
**/
_jxta_peer_entry *JXTA_STDCALL peer_entry_construct(_jxta_peer_entry * self)
{
    apr_status_t res;

    self->thisType = "_jxta_peer_entry";
    apr_pool_create(&self->pool, NULL);

    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,    /* nested */
                                  self->pool);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(self->pool);
        free(self);
        return NULL;
    }

    self->address = NULL;
    self->adv = NULL;
    self->peerid = NULL;
    self->expires = 0;

    return self;
}

void JXTA_STDCALL peer_entry_destruct(_jxta_peer_entry * self)
{

    if (self->address) {
        JXTA_OBJECT_RELEASE(self->address);
    }
    if (self->adv) {
        JXTA_OBJECT_RELEASE(self->adv);
    }

    if (self->peerid) {
        JXTA_OBJECT_RELEASE(self->peerid);
    }

    apr_pool_destroy(self->pool);

    self->thisType = NULL;
}

JXTA_DECLARE(Jxta_peer *) jxta_peer_new()
{
    return (Jxta_peer *) peer_entry_new();
}

JXTA_DECLARE(Jxta_status) jxta_peer_get_adv(Jxta_peer * p, Jxta_PA ** ret)
{
    _jxta_peer_entry *peer = PTValid(p, _jxta_peer_entry);

    if (ret == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(peer->mutex);
    *ret = peer->adv;
    if (*ret != NULL) {
        JXTA_OBJECT_SHARE(*ret);
    }
    apr_thread_mutex_unlock(peer->mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peer_get_peerid(Jxta_peer * p, Jxta_id ** ret)
{
    _jxta_peer_entry *peer = PTValid(p, _jxta_peer_entry);

    if (ret == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(peer->mutex);
    *ret = peer->peerid;
    if (*ret != NULL) {
        JXTA_OBJECT_SHARE(*ret);
    }
    apr_thread_mutex_unlock(peer->mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peer_get_address(Jxta_peer * p, Jxta_endpoint_address ** ret)
{
    _jxta_peer_entry *peer = PTValid(p, _jxta_peer_entry);

    if (ret == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(peer->mutex);
    *ret = peer->address;
    if (*ret != NULL) {
        JXTA_OBJECT_SHARE(*ret);
    }
    apr_thread_mutex_unlock(peer->mutex);
    return JXTA_SUCCESS;
}


JXTA_DECLARE(Jxta_status) jxta_peer_set_adv(Jxta_peer * p, Jxta_PA * val)
{
    _jxta_peer_entry *peer = PTValid(p, _jxta_peer_entry);

    if (val == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(peer->mutex);

    JXTA_OBJECT_SHARE(val);
    if (peer->adv != NULL) {
        JXTA_OBJECT_RELEASE(peer->adv);
    }
    peer->adv = val;
    apr_thread_mutex_unlock(peer->mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peer_set_peerid(Jxta_peer * p, Jxta_id * val)
{
    _jxta_peer_entry *peer = PTValid(p, _jxta_peer_entry);

    if (val == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(peer->mutex);

    JXTA_OBJECT_SHARE(val);

    if (peer->peerid != NULL) {
        JXTA_OBJECT_RELEASE(peer->peerid);
    }
    peer->peerid = val;
    apr_thread_mutex_unlock(peer->mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peer_set_address(Jxta_peer * p, Jxta_endpoint_address * val)
{
    _jxta_peer_entry *peer = PTValid(p, _jxta_peer_entry);

    if (val == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(peer->mutex);

    JXTA_OBJECT_SHARE(val);

    if (peer->address != NULL) {
        JXTA_OBJECT_RELEASE(peer->address);
    }
    peer->address = val;
    apr_thread_mutex_unlock(peer->mutex);
    return JXTA_SUCCESS;
}

Jxta_status JXTA_STDCALL jxta_peer_lock(Jxta_peer * p)
{
    _jxta_peer_entry *peer = PTValid(p, _jxta_peer_entry);

    apr_thread_mutex_lock(peer->mutex);

    return JXTA_SUCCESS;
}

Jxta_status JXTA_STDCALL jxta_peer_unlock(Jxta_peer * p)
{
    _jxta_peer_entry *peer = PTValid(p, _jxta_peer_entry);

    apr_thread_mutex_unlock(peer->mutex);

    return JXTA_SUCCESS;
}

Jxta_endpoint_address *JXTA_STDCALL jxta_peer_get_address_priv(Jxta_peer * p)
{
    _jxta_peer_entry *peer = (_jxta_peer_entry *) p;

    return peer->address;
}

Jxta_id *JXTA_STDCALL jxta_peer_get_peerid_priv(Jxta_peer * p)
{
    _jxta_peer_entry *peer = (_jxta_peer_entry *) p;

    return peer->peerid;
}

Jxta_PA *JXTA_STDCALL jxta_peer_get_adv_priv(Jxta_peer * p)
{
    _jxta_peer_entry *peer = (_jxta_peer_entry *) p;

    return peer->adv;
}

Jxta_time JXTA_STDCALL jxta_peer_get_expires(Jxta_peer * p)
{
    _jxta_peer_entry *peer = PTValid(p, _jxta_peer_entry);

    return peer->expires;
}

Jxta_status JXTA_STDCALL jxta_peer_set_expires(Jxta_peer * p, Jxta_time expires)
{
    _jxta_peer_entry *peer = PTValid(p, _jxta_peer_entry);

    peer->expires = expires;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_time) jxta_rdv_service_peer_get_expires(Jxta_service * rdv, Jxta_peer * p)
{
    return jxta_peer_get_expires(p);
}

Jxta_status jxta_rdv_service_peer_set_expires(Jxta_service * rdv, Jxta_peer * p, Jxta_time expires)
{
    return jxta_peer_set_expires(p, expires);
}
