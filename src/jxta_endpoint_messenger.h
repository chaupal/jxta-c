/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights
 * reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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



#ifndef JXTA_ENDPOINT_MESSENGER_H
#define JXTA_ENDPOINT_MESSENGER_H

#include "jxta_object.h"
#include "jxta_message.h"
#include "jxta_traffic_shaping_priv.h"
#include "jxta_endpoint_config_adv.h"
#include "jxta_apr.h"
#include "jxta_callback.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif


typedef struct _JxtaEndpointMessenger JxtaEndpointMessenger;
typedef Jxta_status(JXTA_STDCALL * MessengerSendFunc) (JxtaEndpointMessenger *, Jxta_message *);
typedef Jxta_status(JXTA_STDCALL * MessengerDetailsFunc) (JxtaEndpointMessenger *, Jxta_message *, apr_int64_t *size, float *compression);
typedef int(JXTA_STDCALL * MessengerHeaderFunc) (JxtaEndpointMessenger *);

struct _JxtaEndpointMessenger {
    JXTA_OBJECT_HANDLE;
    Jxta_endpoint_address *address;
    int header_size;
    Jxta_status(*jxta_send) (JxtaEndpointMessenger *, Jxta_message *);
    Jxta_status(*jxta_get_msg_details) (JxtaEndpointMessenger *, Jxta_message *, apr_int64_t *size, float *compression);
    int (*jxta_header_size) (JxtaEndpointMessenger *);
    /* Jxta_ep_flow_control *ep_fc; */
    Jxta_traffic_shaping *ts;
    Jxta_vector *active_q;
    Jxta_vector *pending_q;
    int fc_frame_seconds;
    apr_int64_t fc_num_bytes;
    int fc_msgs_sent;
    apr_thread_mutex_t * mutex;
    apr_pool_t *pool;
 };

JXTA_DECLARE(JxtaEndpointMessenger *) jxta_endpoint_messenger_initialize(JxtaEndpointMessenger * me
                                    ,MessengerSendFunc jxta_send, MessengerDetailsFunc jxta_get_msg_details, MessengerHeaderFunc header_size, Jxta_endpoint_address * address);

JXTA_DECLARE(JxtaEndpointMessenger *) jxta_endpoint_messenger_construct(JxtaEndpointMessenger * msgr);
/**
*   to be called from inside destruct functions for sub-classes
*/
JXTA_DECLARE(void) jxta_endpoint_messenger_destruct(JxtaEndpointMessenger * msgr);






#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_ENDPOINT_MESSENGER_H */

/* vi: set ts=4 sw=4 tw=130 et: */
