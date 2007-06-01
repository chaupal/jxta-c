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
 * $Id: jxta_rdv_diffusion_msg.h,v 1.1.4.1 2006/11/16 00:06:33 bondolo Exp $
 */


#ifndef __RDV_DIFFUSION_MESSAGE_H__
#define __RDV_DIFFUSION_MESSAGE_H__

#include "jxta_advertisement.h"
#include "jstring.h"
#include "jxta_vector.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
*   The policy of the diffusion message.
*/
enum Jxta_rdv_diffusion_policies {
    /** Flooding. Use sparingly with low, low TTL */
    JXTA_RDV_DIFFUSION_POLICY_BROADCAST = 0,

    /** Send to all subscribers. */
    JXTA_RDV_DIFFUSION_POLICY_MULTICAST = 1,

    /** Send to appropriate destination (caller helps) */
    JXTA_RDV_DIFFUSION_POLICY_TRAVERSAL = 2
};

typedef enum Jxta_rdv_diffusion_policies Jxta_rdv_diffusion_policy;

/**
*   The current scope of the diffusion message.
*/
enum Jxta_rdv_diffusion_scopes {
    /** Initial Stage, message is being sent to a "root" for diffusion. */
    JXTA_RDV_DIFFUSION_SCOPE_GLOBAL = 0,

    /** Main Stage, message is being sent to target destinations. */
    JXTA_RDV_DIFFUSION_SCOPE_LOCAL = 1,

    /** Final Stage, message has been forwarded to appropriate limits. */
    JXTA_RDV_DIFFUSION_SCOPE_TERMINAL = 2
};

typedef enum Jxta_rdv_diffusion_scopes Jxta_rdv_diffusion_scope;

/**
*   Opaque pointer to a diffusion header.
*/
typedef struct _Jxta_rdv_diffusion Jxta_rdv_diffusion;

/**
* Message Element Name we use for diffusion messages.
*/
extern const char JXTA_RDV_DIFFUSION_ELEMENT_NAME[];

JXTA_DECLARE(Jxta_rdv_diffusion *) jxta_rdv_diffusion_new(void);
JXTA_DECLARE(Jxta_status) jxta_rdv_diffusion_get_xml(Jxta_rdv_diffusion *, JString ** xml);
JXTA_DECLARE(Jxta_status) jxta_rdv_diffusion_parse_charbuffer(Jxta_rdv_diffusion *, const char *, int len);
JXTA_DECLARE(Jxta_status) jxta_rdv_diffusion_parse_file(Jxta_rdv_diffusion *, FILE * stream);

JXTA_DECLARE(Jxta_id *) jxta_rdv_diffusion_get_src_peer_id(Jxta_rdv_diffusion * me);
JXTA_DECLARE(void) jxta_rdv_diffusion_set_src_peer_id(Jxta_rdv_diffusion * me, Jxta_id * src_peer_id);

JXTA_DECLARE(Jxta_rdv_diffusion_policy) jxta_rdv_diffusion_get_policy(Jxta_rdv_diffusion * me);
JXTA_DECLARE(void) jxta_rdv_diffusion_set_policy(Jxta_rdv_diffusion * me, Jxta_rdv_diffusion_policy policy);

JXTA_DECLARE(Jxta_rdv_diffusion_scope) jxta_rdv_diffusion_get_scope(Jxta_rdv_diffusion * me);
JXTA_DECLARE(void) jxta_rdv_diffusion_set_scope(Jxta_rdv_diffusion * me, Jxta_rdv_diffusion_scope scope);

JXTA_DECLARE(unsigned int) jxta_rdv_diffusion_get_ttl(Jxta_rdv_diffusion * me);
JXTA_DECLARE(void) jxta_rdv_diffusion_set_ttl(Jxta_rdv_diffusion * me, unsigned int ttl);

JXTA_DECLARE(const char *) jxta_rdv_diffusion_get_target_hash(Jxta_rdv_diffusion * me);
JXTA_DECLARE(void) jxta_rdv_diffusion_set_target_hash(Jxta_rdv_diffusion * me, const char *target_hash);

JXTA_DECLARE(const char *) jxta_rdv_diffusion_get_dest_svc_name(Jxta_rdv_diffusion * me);
JXTA_DECLARE(void) jxta_rdv_diffusion_set_dest_svc_name(Jxta_rdv_diffusion * me, const char *svc_name);

JXTA_DECLARE(const char *) jxta_rdv_diffusion_get_dest_svc_param(Jxta_rdv_diffusion * me);
JXTA_DECLARE(void) jxta_rdv_diffusion_set_dest_svc_param(Jxta_rdv_diffusion * me, const char *svc_params);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __RDV_DIFFUSION_MESSAGE_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
