/* 
 * Copyright (c) 2006 Sun Microsystems, Inc.  All rights reserved.
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


#ifndef __LEASE_RESPONSE_H__
#define __LEASE_RESPONSE_H__

#include "jxta_advertisement.h"
#include "jstring.h"
#include "jxta_vector.h"
#include "jxta_pa.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

JXTA_DECLARE_DATA const char JXTA_LEASE_RESPONSE_ELEMENT_NAME[];

typedef struct _Jxta_lease_response_msg Jxta_lease_response_msg;

typedef struct _Jxta_lease_adv_info Jxta_lease_adv_info;

JXTA_DECLARE(Jxta_lease_response_msg *) jxta_lease_response_msg_new(void);
JXTA_DECLARE(Jxta_status) jxta_lease_response_msg_get_xml(Jxta_lease_response_msg *, JString ** xml);
JXTA_DECLARE(Jxta_status) jxta_lease_response_msg_parse_charbuffer(Jxta_lease_response_msg *, const char *, int len);
JXTA_DECLARE(Jxta_status) jxta_lease_response_msg_parse_file(Jxta_lease_response_msg *, FILE * stream);


JXTA_DECLARE(Jxta_id *) jxta_lease_response_msg_get_server_id(Jxta_lease_response_msg * myself);
JXTA_DECLARE(void) jxta_lease_response_msg_set_server_id(Jxta_lease_response_msg * myself, Jxta_id *);

JXTA_DECLARE(apr_uuid_t *) jxta_lease_response_msg_get_pv_id_gen(Jxta_lease_response_msg * myself);
JXTA_DECLARE(void) jxta_lease_response_msg_set_pv_id_gen(Jxta_lease_response_msg * myself, apr_uuid_t *);

JXTA_DECLARE(Jxta_time_diff) jxta_lease_response_msg_get_offered_lease(Jxta_lease_response_msg * myself);
JXTA_DECLARE(void) jxta_lease_response_msg_set_offered_lease(Jxta_lease_response_msg * myself, Jxta_time_diff);

JXTA_DECLARE(Jxta_lease_adv_info *) jxta_lease_response_msg_get_server_adv_info(Jxta_lease_response_msg * myself);
JXTA_DECLARE(void) jxta_lease_response_msg_set_server_adv_info(Jxta_lease_response_msg * myself, Jxta_PA * adv,
                                                                   apr_uuid_t const * gen, Jxta_time_diff exp);

JXTA_DECLARE(Jxta_vector *) jxta_lease_response_msg_get_referral_advs(Jxta_lease_response_msg * myself);
JXTA_DECLARE(void) jxta_lease_response_msg_set_referral_advs(Jxta_lease_response_msg * myself, Jxta_vector * referrals);
JXTA_DECLARE(void) jxta_lease_response_msg_add_referral_adv(Jxta_lease_response_msg * myself,
                                                               Jxta_PA * adv, Jxta_time_diff exp );

JXTA_DECLARE(Jxta_PA *) jxta_lease_adv_info_get_adv(Jxta_lease_adv_info * myself);
JXTA_DECLARE(apr_uuid_t *) jxta_lease_adv_info_get_adv_gen(Jxta_lease_adv_info * myself);
JXTA_DECLARE(Jxta_time_diff) jxta_lease_adv_info_get_adv_exp(Jxta_lease_adv_info * myself);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __LEASE_RESPONSE_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
