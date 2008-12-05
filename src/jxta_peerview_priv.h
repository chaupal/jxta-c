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


#ifndef JXTA_PEERVIEW_PRIV_H
#define JXTA_PEERVIEW_PRIV_H

#include "jxta_apr.h"

#include "jxta_peerview.h"
#include "jxta_id.h"
#include "jxta_peergroup.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

    /**
 *  Name of the service (as being used in forming the Endpoint Address).
     **/
extern const char JXTA_PEERVIEW_NAME[];
extern const char JXTA_PEERVIEW_PA_ELEMENT_NAME[];
extern const char JXTA_PEERVIEW_FAILURE_ELEMENT_NAME[];

/**
*   Construct a new peerview instance.
*
*   @param parent_pool The apr_pool in which the peerview sub-pool will be allocated.
*   @return The newly constructed peerview or NULL if the peerview could not be created.
*/
extern Jxta_peerview * jxta_peerview_new(apr_pool_t* parent_pool);

/**
*   Initializes the Peerview. The Peerview is usable if init succeeds.
*
*   @param pv    The peerview.
*   @param group The group in which the peerview is operating.
*   @param assigned_id  The module class id associated with this peerview instance.
**/
extern Jxta_status peerview_init(Jxta_peerview * pv, Jxta_PG * group, Jxta_id * assigned_id);

/**
*   When started this peer actively participates in the peerview protocols until
*   <tt>stop()</tt> is called.
*
*   @param pv The peerview.
**/
extern Jxta_status  peerview_start(Jxta_peerview * pv );

/**
*   When stopped the peer will cease active participation in the peerview 
*   protocols. <tt>start()</tt> may be called again to resume operation.
*
*   @param pv The peerview.
*/
extern Jxta_status  peerview_stop(Jxta_peerview * peerview);

extern Jxta_status  jxta_peerview_get_self_peer(Jxta_peerview * pv, Jxta_peer ** peer);

extern char * peerview_get_assigned_id_str(Jxta_peerview * pv);

extern Jxta_status jxta_peerview_set_pa_refresh(Jxta_peerview * pv, Jxta_time_diff interval);

extern Jxta_time_diff jxta_peerview_get_pa_refresh(Jxta_peerview * pv);

extern Jxta_status peerview_get_peer(Jxta_peerview * pv, Jxta_id * peer_id, Jxta_peer **peer);

extern Jxta_status peerview_get_peer_address(Jxta_peer *peer, BIGNUM **address);

extern Jxta_status peerview_send_disconnect(Jxta_peerview * pv, Jxta_peer * dest);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
