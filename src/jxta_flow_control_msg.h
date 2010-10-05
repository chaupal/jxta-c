/* 
 * Copyright (c) 2010 Sun Microsystems, Inc.  All rights reserved.
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


#ifndef __FLOW_CONTROL_MSG_H__
#define __FLOW_CONTROL_MSG_H__

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
typedef struct _jxta_ep_flow_control_msg Jxta_ep_flow_control_msg;

/**
* Message Element Name we use for address assignment messages.
*/
JXTA_DECLARE_DATA const char JXTA_FLOW_CONTROL_ELEMENT_NAME[];

JXTA_DECLARE(Jxta_ep_flow_control_msg *) jxta_ep_flow_control_msg_new(void);
JXTA_DECLARE(Jxta_ep_flow_control_msg *) jxta_ep_flow_control_msg_new_1(int fc_time
                            , apr_int64_t fc_size, int fc_interval, int fc_frame
                            , int fc_look_ahead, int fc_reserve, Ts_max_option max_option);
JXTA_DECLARE(Jxta_status) jxta_ep_flow_control_msg_get_xml(Jxta_ep_flow_control_msg *, JString ** xml);

JXTA_DECLARE(Jxta_status) jxta_ep_flow_control_msg_set_peerid(Jxta_ep_flow_control_msg *, Jxta_id *);
JXTA_DECLARE(Jxta_status) jxta_ep_flow_control_msg_get_peerid(Jxta_ep_flow_control_msg *, Jxta_id **);

JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_size(Jxta_ep_flow_control_msg *, apr_int64_t);
JXTA_DECLARE(apr_int64_t) jxta_ep_flow_control_msg_get_size(Jxta_ep_flow_control_msg *);

JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_time(Jxta_ep_flow_control_msg *, Jxta_time);
JXTA_DECLARE(Jxta_time) jxta_ep_flow_control_msg_get_time(Jxta_ep_flow_control_msg *);

JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_interval(Jxta_ep_flow_control_msg *, int);
JXTA_DECLARE(int) jxta_ep_flow_control_msg_get_interval(Jxta_ep_flow_control_msg *);

JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_frame(Jxta_ep_flow_control_msg *, int);
JXTA_DECLARE(int) jxta_ep_flow_control_msg_get_frame(Jxta_ep_flow_control_msg *);

JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_look_ahead(Jxta_ep_flow_control_msg *, int);
JXTA_DECLARE(int) jxta_ep_flow_control_msg_get_look_ahead(Jxta_ep_flow_control_msg *);

JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_max_option(Jxta_ep_flow_control_msg *, Ts_max_option);
JXTA_DECLARE(Ts_max_option) jxta_ep_flow_control_msg_get_max_option(Jxta_ep_flow_control_msg *);

JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_reserve(Jxta_ep_flow_control_msg *, int reserve);
JXTA_DECLARE(int) jxta_ep_flow_control_msg_get_reserve(Jxta_ep_flow_control_msg *);

JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_override(Jxta_ep_flow_control_msg *, apr_int64_t size);
JXTA_DECLARE(apr_int64_t) jxta_ep_flow_control_msg_override(Jxta_ep_flow_control_msg *);

JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_required_length(Jxta_ep_flow_control_msg *, apr_int64_t size);
JXTA_DECLARE(apr_int64_t) jxta_ep_flow_control_msg_required_length(Jxta_ep_flow_control_msg *);

JXTA_DECLARE(Jxta_status) jxta_ep_flow_control_msg_parse_charbuffer(Jxta_ep_flow_control_msg *
                            , const char *, int len);
JXTA_DECLARE(Jxta_status) jxta_ep_flow_control_msg_parse_file(Jxta_ep_flow_control_msg *, FILE * stream);





#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __PEERVIEW_ADDR_ASSIGN_MSG_H__  */
