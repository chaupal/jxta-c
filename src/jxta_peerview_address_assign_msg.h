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
 * $Id: jxta_peerview_address_assign_msg.h,v 1.1.4.1 2006/11/16 00:06:31 bondolo Exp $
 */


#ifndef __PEERVIEW_ADDR_ASSIGN_MSG_H__
#define __PEERVIEW_ADDR_ASSIGN_MSG_H__

#include "jstring.h"
#include "jxta_vector.h"
#include "jxta_cred.h"
#include "jxta_pa.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif


/**
*   Opaque pointer to a address_assign message.
*/
typedef struct _Jxta_peerview_address_assign_msg Jxta_peerview_address_assign_msg;

/**
* Message Element Name we use for address assignment messages.
*/
JXTA_DECLARE_DATA const char JXTA_PEERVIEW_ADDRESS_ASSIGN_ELEMENT_NAME[];

JXTA_DECLARE(Jxta_peerview_address_assign_msg *) jxta_peerview_address_assign_msg_new(void);
JXTA_DECLARE(Jxta_status) jxta_peerview_address_assign_msg_get_xml(Jxta_peerview_address_assign_msg *, JString ** xml);
JXTA_DECLARE(Jxta_status) jxta_peerview_address_assign_msg_parse_charbuffer(Jxta_peerview_address_assign_msg *, const char *, int len);
JXTA_DECLARE(Jxta_status) jxta_peerview_address_assign_msg_parse_file(Jxta_peerview_address_assign_msg *, FILE * stream);

JXTA_DECLARE(Jxta_id*) jxta_peerview_address_assign_msg_get_peer_id(Jxta_peerview_address_assign_msg * me);
JXTA_DECLARE(void) jxta_peerview_address_assign_msg_set_peer_id(Jxta_peerview_address_assign_msg * me, Jxta_id* id);

JXTA_DECLARE(Jxta_credential*) jxta_peerview_address_assign_msg_get_credential(Jxta_peerview_address_assign_msg * me);
JXTA_DECLARE(void) jxta_peerview_address_assign_msg_set_credential(Jxta_peerview_address_assign_msg * me, Jxta_credential* credential);

JXTA_DECLARE(const char *) jxta_peerview_address_assign_msg_get_instance_mask(Jxta_peerview_address_assign_msg * me);
JXTA_DECLARE(void) jxta_peerview_address_assign_msg_set_instance_mask(Jxta_peerview_address_assign_msg * me, const char *instance_mask);

JXTA_DECLARE(const char *) jxta_peerview_address_assign_msg_get_target_hash(Jxta_peerview_address_assign_msg * me);
JXTA_DECLARE(void) jxta_peerview_address_assign_msg_set_target_hash(Jxta_peerview_address_assign_msg * me, const char *target_hash);

JXTA_DECLARE(Jxta_vector *) jxta_peerview_address_assign_msg_get_options(Jxta_peerview_address_assign_msg * me);
JXTA_DECLARE(void) jxta_peerview_address_assign_msg_set_options(Jxta_peerview_address_assign_msg * me, Jxta_vector *options);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __PEERVIEW_ADDR_ASSIGN_MSG_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
