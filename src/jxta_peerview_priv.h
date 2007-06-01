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
 * $Id: jxta_peerview_priv.h,v 1.3 2006/02/01 23:37:50 slowhog Exp $
 */


#ifndef JXTA_PEERVIEW_PRIV_H
#define JXTA_PEERVIEW_PRIV_H

#include "jxta_apr.h"

#include "jxta_peerview.h"
#include "jxta_id.h"
#include "jxta_pipe_adv.h"
#include "jxta_peergroup.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

    /**
     ** Name of the service (as being used in forming the Endpoint Address).
     **/
extern const char JXTA_PEERVIEW_NAME[];
extern const char JXTA_PEERVIEW_RDVADV_ELEMENT_NAME[];
extern const char JXTA_PEERVIEW_RDVRESP_ELEMENT_NAME[];
extern const char JXTA_PEERVIEW_FAILURE_ELEMENT_NAME[];
extern const char JXTA_PEERVIEW_CACHED_ELEMENT_NAME[];
extern const char JXTA_PEERVIEW_EDGE_ELEMENT_NAME[];
extern const char JXTA_PEERVIEW_RESPONSE_ELEMENT_NAME[];

JXTA_DECLARE(Jxta_peerview *) jxta_peerview_new(void);

/**
*   Initializes the Peerview. The Peerview is usable if init succeeds.
*
*   @param pv    The peerview.
*   @param group The group in which the peerview is operating.
**/
JXTA_DECLARE(Jxta_status) jxta_peerview_init(Jxta_peerview * pv, Jxta_PG * group, JString * name);

JXTA_DECLARE(Jxta_status) jxta_peerview_start(Jxta_peerview * pv);

JXTA_DECLARE(Jxta_status) jxta_peerview_stop(Jxta_peerview * peerview);

JXTA_DECLARE(Jxta_status) jxta_peerview_send_rdv_probe(Jxta_peerview * self, Jxta_endpoint_address * dest);

JXTA_DECLARE(Jxta_status) jxta_peerview_send_rdv_request(Jxta_peerview * pv, Jxta_boolean announce, Jxta_boolean failure);

JXTA_DECLARE(Jxta_status) jxta_peerview_add_seed(Jxta_peerview * peerview, Jxta_peer * peer);

JXTA_DECLARE(Jxta_status) jxta_peerview_probe_cached_rdvadv(Jxta_peerview * pv, Jxta_RdvAdvertisement * rdva);

JXTA_DECLARE(Jxta_status) jxta_peerview_get_down_peer(Jxta_peerview * pv, Jxta_peer ** peer);
JXTA_DECLARE(Jxta_status) jxta_peerview_get_self_peer(Jxta_peerview * pv, Jxta_peer ** peer);
JXTA_DECLARE(Jxta_status) jxta_peerview_get_up_peer(Jxta_peerview * pv, Jxta_peer ** peer);

JXTA_DECLARE(JString *) jxta_peerview_get_name(Jxta_peerview * pv);

JXTA_DECLARE(Jxta_vector *) jxta_peerview_get_seeds(Jxta_peerview * self);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
