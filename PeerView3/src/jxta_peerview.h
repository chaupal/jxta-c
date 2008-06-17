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
    JXTA_PEERVIEW_REMOVE = 2,
    JXTA_PEERVIEW_DEMOTE = 3
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
 * Callback to provide a vector of candidate peers from the list of potential peers.
 *
 * @param peers Peer entries available as candidates.
 * @param candidates Candidates to use.
 * @return JXTA_SUCCESS If the candidates vector is valid.
 **/
typedef Jxta_status (JXTA_STDCALL * Jxta_peerview_candidate_list_func) (Jxta_vector const *peers, Jxta_vector **candidates);

/**
 * Callback to provide a target hash or cluster number.
 *
 * @param peer Peer requesting the address assignment
 * @param cluster Location to store the cluster number to assign.
 * @param target_hash Specific target hash to assign.
 * @return JXTA_SUCCESS If the cluster or target hash have been set.
 *          JXTA_NOT_CONFIGURED If neither the cluster or target_hash have been set.
 **/
typedef Jxta_status (JXTA_STDCALL * Jxta_peerview_address_req_func) (Jxta_peer *peer, unsigned int *cluster, BIGNUM **target_hash);

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
*   Sets the peerview auto cycle on/off.
*
*   @param pv The peerview.
*   @param ttime Time for auto cycle - > 0 -check peerview state
*                                       -1 -stop checking state.
**/
JXTA_DECLARE(void) jxta_peerview_set_auto_cycle(Jxta_peerview * pv, Jxta_time_diff ttime );

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
*   Returns a vector containing Jxta_peer_entry objects for each of the current
*   peerview members within the specified cluster.
*
*   @param pv The peerview.
*   @param cluster Cluster number requested.
*   @param view the address of the destination vector for the entries.
*   @return JXTA_SUCCESS if entries could be retrieved otherwise false.
*/
JXTA_DECLARE(Jxta_status) jxta_peerview_get_cluster_view(Jxta_peerview * pv, unsigned int cluster, Jxta_vector ** view);

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

/**
 * Set the callback function for ranking peers in the peerview. The callback is called when the maintenance cycle 
 * is evaluating whether to remain in the peerview.
 *
 *  @param pv The peerview.
 *  @param cb The callback function that compares peers passed by the jxta_vector_qsort function.
 *  @return JXTA_SUCCESS The callback has been registered.
*/
JXTA_DECLARE(Jxta_status) jxta_peerview_set_ranking_order_cb(Jxta_peerview * pv, Jxta_object_compare_func cb);

/**
 * Set the callback function for sorting the candidate list. This callback is called before a candidate is selected
 * to recruit for the peerview.
 * 
 *  @param pv The peerview.
 *  @param cb The callback function that compares peers passed by the jxta_vector_qsort function.
 *  @return JXTA_SUCCESS The callback has been registered.
*/
JXTA_DECLARE(Jxta_status) jxta_peerview_set_candidate_order_cb(Jxta_peerview * pv, Jxta_object_compare_func cb);

/**
 * Set the callback for providing a candidate list. This function is called when a candidate list is required. This callback
 * is used to retrieve a candidate list.
 * 
 *  @param pv The peerview.
 *  @param cb The callback function that returns the candidates to use.
 *  @return JXTA_SUCCESS The callback has been registered.
*/
JXTA_DECLARE(Jxta_status) jxta_peerview_set_candidate_list_cb(Jxta_peerview * pv,  Jxta_peerview_candidate_list_func cb);

/**
 * Set the callback for providing a target_hash or cluster number when a peer requests an address assignment.
 * 
 *  @param pv The peerview.
 *  @param cb The callback function that returns the cluster number or target hash the peer should be assigned.
 *  @return JXTA_SUCCESS The callback has been registered.
*/
JXTA_DECLARE(Jxta_status) jxta_peerview_set_address_cb(Jxta_peerview * pv,  Jxta_peerview_address_req_func cb);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
