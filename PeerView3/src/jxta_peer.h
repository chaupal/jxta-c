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


#ifndef __JXTA_PEER_H__
#define __JXTA_PEER_H__

#include "jxta_errno.h"
#include "jxta_id.h"
#include "jxta_pa.h"
#include "jxta_endpoint_address.h"
#include "jxta_service.h"

/**
 ** This file defines the JXTA Jxta_peer object.
 **/

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_peer_entry Jxta_peer;

/*******
 * Create a Jxta_peer object. This object is mutable.
 * A Jxta_peer object is a Jxta_object: the new object is
 * already initialized by this method, and needs to be released
 * when it is no longer used.
 *
 ********/
JXTA_DECLARE(Jxta_peer *) jxta_peer_new(void);

/**
 *   Determine if the two peer objects refer to the same peer. The
 *   determination is made by comparing the peer id values or the
 *   endpoint addresses if both objects have no peerid set.
 *
 *   @param a pointer to the Jxta_peer object
 *   @param b pointer to the Jxta_peer object
 *   @returns TRUE if peers refer to the same entity otherwise FALSE.
 **/
JXTA_DECLARE(Jxta_boolean) jxta_peer_equals( Jxta_peer* a, Jxta_peer* b );

/*******
 * Get the PeerId of the peer
 *
 * @param peer a pointer to the Jxta_peer object
 * @param  id a pointer to a pointer to Jxta_id containing resulting id. This object
 * needs to be released when not used anymore.
 * @returns JXTA_SUCCESS when succesfull, or JXTA_INVALID_ARGUMENT when no
 * peer id is associated to the given Jxta_peer.
 ********/
JXTA_DECLARE(Jxta_status) jxta_peer_get_peerid(Jxta_peer * peer, Jxta_id ** id);
JXTA_DECLARE(Jxta_id*) jxta_peer_peerid(Jxta_peer * me);

/*******
 * Get the Peer Advertisement of the peer
 *
 * @param peer a pointer to the Jxta_peer object
 * @param pa a pointer to a pointer to Jxta_PA containing resulting advertisement. This object
 * needs to be released when not used anymore.
 * Note that the return object has to be released
 * @returns JXTA_SUCCESS when succesfull, or JXTA_INVALID_ARGUMENT when no
 * Peer Advertisement is associated to the given Jxta_peer.
 ********/
JXTA_DECLARE(Jxta_status) jxta_peer_get_adv(Jxta_peer * peer, Jxta_PA ** pa);
JXTA_DECLARE(Jxta_PA*) jxta_peer_adv(Jxta_peer * me);

/*******
 * Get the Endpoint Address of the peer
 *
 * @param peer a pointer to the Jxta_peer object
 * @param addr a pointer to a pointer to Jxta_endpoint_addresscontaining resulting
 * Endpoint Address. This object needs to be released when not used anymore.
 * Note that the return object has to be released
 * @returns JXTA_SUCCESS when succesfull, or JXTA_INVALID_ARGUMENT when no
 * Jxta_endpoint_address is associated to the given Jxta_peer.
 ********/
JXTA_DECLARE(Jxta_status) jxta_peer_get_address(Jxta_peer * peer, Jxta_endpoint_address ** addr);
JXTA_DECLARE(Jxta_endpoint_address*) jxta_peer_address(Jxta_peer * me);

/*******
 * Set the PeerId of the peer
 *
 * @param peer a pointer to the Jxta_peer object
 * @param peerId a pointer to the Jxta_id. Note that the object
 * is automatically shared by this call.
 * @return a status
 ********/
JXTA_DECLARE(Jxta_status) jxta_peer_set_peerid(Jxta_peer * peer, Jxta_id * peerId);

/*******
 * Set the Peer Advertisement of the peer
 *
 * @param peer a pointer to the Jxta_peer object
 * @param adv a pointer to the Jxta_id. Note that the object
 * is automatically shared by this call.
 * @return a status
 ********/
JXTA_DECLARE(Jxta_status) jxta_peer_set_adv(Jxta_peer * peer, Jxta_PA * adv);

/*******
 * Set the Endpoint Address of the peer
 *
 * @param peer a pointer to the Jxta_peer object
 * @param addr a pointer to the Jxta_endpoint_address. Note that the object
 * is automatically shared by this call.
 * @return a status
 ********/
JXTA_DECLARE(Jxta_status) jxta_peer_set_address(Jxta_peer * peer, Jxta_endpoint_address * addr);


/*******
 * Get the expiration time of this peer record. 
 *
 * @param peer a pointer to the Jxta_peer object
 * @return The absolute time in milliseconds at which this peer will expire.
 ********/
JXTA_DECLARE(Jxta_time) jxta_peer_get_expires(Jxta_peer * p);

/*******
 * Set the expiration time of this peer record. 
 *
 * @param peer a pointer to the Jxta_peer object
 * @param expires The absolute time in milliseconds at which this peer will expire.
 * @return a status
 ********/
JXTA_DECLARE(Jxta_status) jxta_peer_set_expires(Jxta_peer * p, Jxta_time expires);

/**
 * @todo Add documentation.
 */
JXTA_DECLARE(Jxta_time) jxta_rdv_service_peer_get_expires(Jxta_service * rdv, Jxta_peer * p);

/**
 * Set the options for this peer
 *
 * @param peer a pointer of the peer entry
 * @param options a pointer to an options vector
 * @return a status
**/
JXTA_DECLARE(Jxta_status) jxta_peer_set_options(Jxta_peer * p, Jxta_vector *options);

/**
 * Gets the options for this peer
 *
 * @param peer a pointer of the peer entry
 * @param options a pointer to an options vector pointer
 * @return a status
**/
JXTA_DECLARE(Jxta_status) jxta_peer_get_options(Jxta_peer * p, Jxta_vector **options);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_PEER_H__ */

/* vi: set ts=4 sw=4 tw=130 et: */
