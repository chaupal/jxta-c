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
 * $Id: jxta_peer.h,v 1.6 2005/01/18 20:26:11 bondolo Exp $
 */


#ifndef __JXTA_PEER_H__
#define __JXTA_PEER_H__

#include "jxta_errno.h"
#include "jxta_id.h"
#include "jxta_pa.h"
#include "jxta_endpoint_address.h"

/**
 ** This file defines the JXTA jxta_peer_t object.
 **/


#ifdef __cplusplus
extern "C" {
#if 0
}
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
Jxta_peer *jxta_peer_new(void);

 /*******
   * Get the PeerId of the peer
   *
   * @param peer a pointer to the Jxta_peer object
   * @param  id a pointer to a pointer to Jxta_id containing resulting id. This object
   * needs to be released when not used anymore.
   * @returns JXTA_SUCCESS when succesfull, or JXTA_INVALID_ARGUMENT when no
   * peer id is associated to the given Jxta_peer.
   ********/
Jxta_status jxta_peer_get_peerid(Jxta_peer * peer, Jxta_id ** id);

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
Jxta_status jxta_peer_get_adv(Jxta_peer * peer, Jxta_PA ** pa);

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
Jxta_status jxta_peer_get_address(Jxta_peer * peer, Jxta_endpoint_address ** addr);

  /*******
   * Set the PeerId of the peer
   *
   * @param peer a pointer to the Jxta_peer object
   * @param peerId a pointer to the Jxta_id. Note that the object
   * is automatically shared by this call.
   * @return a status
   ********/
Jxta_status jxta_peer_set_peerid(Jxta_peer * peer, Jxta_id * peerId);

  /*******
   * Set the Peer Advertisement of the peer
   *
   * @param peer a pointer to the Jxta_peer object
   * @param adv a pointer to the Jxta_id. Note that the object
   * is automatically shared by this call.
   * @return a status
   ********/
Jxta_status jxta_peer_set_adv(Jxta_peer * peer, Jxta_PA * adv);

  /*******
   * Set the Endpoint Address of the peer
   *
   * @param peer a pointer to the Jxta_peer object
   * @param addr a pointer to the Jxta_endpoint_address. Note that the object
   * is automatically shared by this call.
   * @return a status
   ********/
Jxta_status jxta_peer_set_address(Jxta_peer * peer, Jxta_endpoint_address * addr);

#ifdef __cplusplus
}
#endif


#endif /* __JXTA_PEER_H__ */
