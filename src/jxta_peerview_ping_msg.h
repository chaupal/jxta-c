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


#ifndef __PEERVIEW_PING_H__
#define __PEERVIEW_PING_H__

#include "jxta_advertisement.h"
#include "jstring.h"
#include "jxta_endpoint_address.h"
#include "jxta_vector.h"
#include "jxta_cred.h"
#include "jxta_peer.h"
#include "jxta_peerview_option_entry.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

JXTA_DECLARE_DATA const char JXTA_PEERVIEW_PING_ELEMENT_NAME[];

typedef struct _Jxta_peerview_ping_msg Jxta_peerview_ping_msg;
typedef struct _jxta_pv_ping_msg_group_entry Jxta_pv_ping_msg_group_entry;

JXTA_DECLARE(Jxta_peerview_ping_msg *) jxta_peerview_ping_msg_new(void);
JXTA_DECLARE(Jxta_status) jxta_peerview_ping_msg_get_xml(Jxta_peerview_ping_msg *, JString ** xml);
JXTA_DECLARE(Jxta_status) jxta_peerview_ping_msg_parse_charbuffer(Jxta_peerview_ping_msg *, const char *, int len);
JXTA_DECLARE(Jxta_status) jxta_peerview_ping_msg_parse_file(Jxta_peerview_ping_msg *, FILE * stream);

JXTA_DECLARE(Jxta_id *) jxta_peerview_ping_msg_get_src_peer_id(Jxta_peerview_ping_msg * myself);
JXTA_DECLARE(void) jxta_peerview_ping_msg_set_src_peer_id(Jxta_peerview_ping_msg * myself, Jxta_id *);

JXTA_DECLARE(Jxta_id *) jxta_peerview_ping_msg_get_dst_peer_id(Jxta_peerview_ping_msg * myself);
JXTA_DECLARE(void) jxta_peerview_ping_msg_set_dst_peer_id(Jxta_peerview_ping_msg * myself, Jxta_id *);

JXTA_DECLARE(Jxta_endpoint_address *) jxta_peerview_ping_msg_get_dst_peer_ea(Jxta_peerview_ping_msg * myself);
JXTA_DECLARE(void) jxta_peerview_ping_msg_set_dst_peer_ea(Jxta_peerview_ping_msg * myself, Jxta_endpoint_address *);

JXTA_DECLARE(Jxta_boolean) jxta_peerview_ping_msg_is_pv_id_only(Jxta_peerview_ping_msg * myself);
JXTA_DECLARE(void) jxta_peerview_ping_msg_set_pv_id_only(Jxta_peerview_ping_msg * myself, Jxta_boolean id_only);

JXTA_DECLARE(Jxta_boolean) jxta_peerview_ping_msg_is_broadcast(Jxta_peerview_ping_msg *myself);

JXTA_DECLARE(apr_uuid_t*) jxta_peerview_ping_msg_get_dest_peer_adv_gen(Jxta_peerview_ping_msg * me);
JXTA_DECLARE(void) jxta_peerview_ping_msg_set_dest_peer_adv_gen(Jxta_peerview_ping_msg * me, apr_uuid_t*);

JXTA_DECLARE(Jxta_credential*) jxta_peerview_ping_msg_get_credential(Jxta_peerview_ping_msg * me);
JXTA_DECLARE(void) jxta_peerview_ping_msg_set_credential(Jxta_peerview_ping_msg * me, Jxta_credential* credential);

JXTA_DECLARE(Jxta_vector*) jxta_peerview_ping_msg_get_options(Jxta_peerview_ping_msg * me);
JXTA_DECLARE(void) jxta_peerview_ping_msg_set_options(Jxta_peerview_ping_msg * me, Jxta_vector*);
JXTA_DECLARE(void) jxta_peerview_ping_msg_add_option_entry(Jxta_peerview_ping_msg *me, Jxta_peerview_option_entry *option_entry);
JXTA_DECLARE(void) jxta_peerview_ping_msg_get_option_entries(Jxta_peerview_ping_msg *me, Jxta_vector **option_entries);

JXTA_DECLARE(void) jxta_peerview_ping_msg_set_composite(Jxta_peerview_ping_msg *me, Jxta_boolean comp);
JXTA_DECLARE(Jxta_boolean) jxta_peerview_ping_msg_is_composite(Jxta_peerview_ping_msg *me);

JXTA_DECLARE(Jxta_status) jxta_peerview_ping_msg_add_group_entry(Jxta_peerview_ping_msg * myself, Jxta_pv_ping_msg_group_entry *entry);
JXTA_DECLARE(Jxta_status) jxta_peerview_ping_msg_get_group_entries(Jxta_peerview_ping_msg * myself, Jxta_vector **entries);

JXTA_DECLARE(Jxta_pv_ping_msg_group_entry *) jxta_pv_ping_msg_entry_new(Jxta_peer *peer, apr_uuid_t peer_adv_gen, Jxta_boolean peer_adv_gen_set, const char *group, Jxta_boolean pv_id_only);

JXTA_DECLARE(Jxta_peer *) jxta_peerview_ping_msg_entry_get_pve(Jxta_pv_ping_msg_group_entry * myself);

JXTA_DECLARE(void) jxta_peerview_ping_msg_entry_set_pv_id_only(Jxta_pv_ping_msg_group_entry * me, Jxta_boolean pv_id_only);
JXTA_DECLARE(Jxta_boolean) jxta_peerview_ping_msg_entry_is_pv_id_only(Jxta_pv_ping_msg_group_entry * me);
JXTA_DECLARE(JString *) jxta_peerview_ping_msg_entry_group_name(Jxta_pv_ping_msg_group_entry *me);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __PEERVIEW_PING_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
