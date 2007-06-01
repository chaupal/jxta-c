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
 * $Id: jxta_peerview.h,v 1.16.4.1 2006/11/16 00:06:30 bondolo Exp $
 */


#ifndef JXTA_PEERVIEW_H
#define JXTA_PEERVIEW_H

#include <openssl/bn.h>

#include "jxta_apr.h"

#include "jxta_id.h"
#include "jxta_listener.h"
#include "jxta_peer.h"
#include "jxta_vector.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_peerview Jxta_peerview;

/**
 * The Peerview event types
 **/
typedef enum Jxta_Peerview_event_types {
    JXTA_PEERVIEW_ADD = 1,
    JXTA_PEERVIEW_REMOVE = 2
} Jxta_Peerview_event_type;

/**
 *    The event structure
 */
typedef struct _jxta_peerview_event {
    JXTA_OBJECT_HANDLE;
    Jxta_Peerview_event_type event;
    Jxta_id *pid;
} Jxta_peerview_event;

/**
*   Returns TRUE if this peer is an active member of the peerview.
*   @param pv The peerview.
*   @return TRUE if this peer is an active member of the peerview otherwise false.
**/
JXTA_DECLARE(Jxta_boolean) jxta_peerview_is_member(Jxta_peerview * pv );

/**
*   Returns TRUE if this peer is active in trying to join the peerview.
*
*   @param pv The peerview.
*   @return TRUE if this peer is active in trying to join the peerview otherwise FALSE.
**/
JXTA_DECLARE(Jxta_boolean) jxta_peerview_is_active(Jxta_peerview * pv );

/**
*   Sets the peerview to the active state.
*
*   @param pv The peerview.
*   @param active If TRUE this peer will actively try to join the peerview.
**/
JXTA_DECLARE(Jxta_boolean) jxta_peerview_set_active(Jxta_peerview * pv, Jxta_boolean active );

/**
*   Returns a vector containing Jxta_peer_entry objects for each of the current
*   peerview members we are partnered with in other clusters.
*
*   @param pv The peerview.
*   @param view the address of the destination vector for the entries.
*   @return JXTA_SUCCESS if entries could be retrieved otherwise false.
*/
JXTA_DECLARE(Jxta_status) jxta_peerview_get_globalview(Jxta_peerview * pv, Jxta_vector ** view);

/**
*   Returns a count of the number of entries in the global view (the number of clusters).
*
*   @param pv The peerview.
*   @return The number of clusters used by this peerview.
*/
JXTA_DECLARE(unsigned int) jxta_peerview_get_globalview_size(Jxta_peerview * pv);

/**
*   Returns a vector containing Jxta_peer_entry objects for each of the current
*   peerview members.
*
*   @param pv The peerview.
*   @param view the address of the destination vector for the entries.
*   @return JXTA_SUCCESS if entries could be retrieved otherwise false.
*/
JXTA_DECLARE(Jxta_status) jxta_peerview_get_localview(Jxta_peerview * pv, Jxta_vector ** view);

/**
*   Returns a count of the current number of entries in the current local view.
*   Usually more efficient than getting the view just to count the entries.
*
*   @deprecated Since the structure of the peerview has changed this is no longer really useful.
*
*   @param pv The peerview.
*   @return number of entries in the current view.
*/
JXTA_DECLARE(unsigned int) jxta_peerview_get_localview_size(Jxta_peerview * pv);

/**
*
**/
JXTA_DECLARE(Jxta_status) jxta_peerview_gen_hash(Jxta_peerview * me, unsigned char const *value, size_t length, BIGNUM ** hash);
                                                              
/**
*   Return the associate peer for the specified cluster.
*
*   @param pv The peerview.
*   @param cluster The cluster who's associate peer is sought.
*   @param peer The result associate peer.
*   @return JXTA_SUCCESS If a peer could be returned otherwise errors per the fault.
*/
JXTA_DECLARE(Jxta_status) jxta_peerview_get_associate_peer(Jxta_peerview * pv, unsigned int cluster, Jxta_peer ** peer);
                                                              
/**
*   Returns a Jxta_peer object for the specified target hash value.
*
*   @param pv The peerview.
*   @param target_hash The target hash value. The peer returned will be the peer which mostly closely matches the hash.
*   @param peer The peer which will be returned.
*   @return JXTA_SUCCESS If a peer could be returned otherwise errors per the fault.
*/
JXTA_DECLARE(Jxta_status) jxta_peerview_get_peer_for_target_hash(Jxta_peerview * pv, BIGNUM *target_hash, Jxta_peer ** peer);



JXTA_DECLARE(Jxta_status) jxta_peerview_add_event_listener(Jxta_peerview * pv, const char *serviceName,
                                                           const char *serviceParam, Jxta_listener *listener);

JXTA_DECLARE(Jxta_status) jxta_peerview_remove_event_listener(Jxta_peerview * pv, const char *serviceName,
                                                              const char *serviceParam);
#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
