/* 
 * Copyright (c) 2009 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id:$
 */


#ifndef __JXTAEMMSG_H__
#define __JXTAEMMSG_H__

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

#import "jxta_message.h"

static const char JXTA_ENDPOINT_MSG_JXTA_NS[] = "jxta";
static const char JXTA_ENPOINT_MSG_ELEMENT_NAME[] = "EndpointMessage";
static const char JXTA_FLOWCONTROL_MSG_ELEMENT_NAME[] = "FlowControl";

typedef struct _jxta_endpoint_message Jxta_endpoint_message;
typedef struct _jxta_endpoint_msg_entry_element Jxta_endpoint_msg_entry_element;

JXTA_DECLARE(Jxta_endpoint_message *) jxta_endpoint_msg_new(void);
JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_get_xml(Jxta_endpoint_message * myself
                                    , Jxta_boolean encode, JString ** xml, Jxta_boolean with_entries);
JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_parse_charbuffer(Jxta_endpoint_message *, const char *, int len);
JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_parse_file(Jxta_endpoint_message *, FILE * stream);

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_set_timestamp(Jxta_endpoint_message *myself, Jxta_time timestamp);
JXTA_DECLARE(Jxta_time) jxta_endpoint_msg_timestamp(Jxta_endpoint_message *myself);

JXTA_DECLARE(Jxta_endpoint_msg_entry_element *) jxta_endpoint_message_entry_new();
JXTA_DECLARE(void) jxta_endpoint_msg_set_priority(Jxta_endpoint_message *ep_msg, Msg_priority p);
JXTA_DECLARE(Msg_priority) jxta_endpoint_msg_priority(Jxta_endpoint_message *ep_msg);
JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_entry_add_attribute(Jxta_endpoint_msg_entry_element *entry, const char *attribute, const char *value);
JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_entry_get_attribute(Jxta_endpoint_msg_entry_element *entry, const char *attribute
                                    , JString ** get_att_j);

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_entry_get_attributes(Jxta_endpoint_msg_entry_element *entry, Jxta_hashtable **attributes);
JXTA_DECLARE(void) jxta_endpoint_msg_entry_set_value(Jxta_endpoint_msg_entry_element *entry, Jxta_message_element *msg_elem);
JXTA_DECLARE(void) jxta_endpoint_msg_entry_get_value(Jxta_endpoint_msg_entry_element *entry, Jxta_message_element **msg_elem);
JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_entry_set_ea(Jxta_endpoint_msg_entry_element *entry, Jxta_endpoint_address *ea);
JXTA_DECLARE(void) jxta_endpoint_msg_entry_get_ea(Jxta_endpoint_msg_entry_element *entry, Jxta_endpoint_address **ea);

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_add_entry(Jxta_endpoint_message *msg, Jxta_endpoint_msg_entry_element * entry);
JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_get_entries(Jxta_endpoint_message *msg, Jxta_vector **entries);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTAEMMSG_H__  */
