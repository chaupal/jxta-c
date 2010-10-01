/*
 * Copyright(c) 2005 Sun Microsystems, Inc.  All rights reserved.
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
 * OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
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

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_endpoint_messenger.h"

static const char *__log_cat = "MESSENGER";

JXTA_DECLARE(JxtaEndpointMessenger *) jxta_endpoint_messenger_initialize(JxtaEndpointMessenger * me
                                    ,MessengerSendFunc jxta_send, MessengerDetailsFunc jxta_get_msg_details, MessengerHeaderFunc header_size, Jxta_endpoint_address * address)
{
    me->jxta_send = jxta_send;
    me->jxta_get_msg_details = jxta_get_msg_details;
    me->jxta_header_size = header_size;
    me->address = address != NULL ? JXTA_OBJECT_SHARE(address):NULL;

    return jxta_endpoint_messenger_construct(me);
}

JXTA_DECLARE(JxtaEndpointMessenger *) jxta_endpoint_messenger_construct(JxtaEndpointMessenger * msgr)
{
    apr_status_t res;

    res =  apr_pool_create(&msgr->pool, NULL);
    if (APR_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to create a pool in the messenger:%d\n", res);
    } else {
        res = apr_thread_mutex_create(&msgr->mutex, APR_THREAD_MUTEX_DEFAULT, msgr->pool);
        if (APR_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to create a mutex in the messenger:%d\n", res);
        }
    }
    msgr->active_q = jxta_vector_new(0);
    msgr->pending_q = jxta_vector_new(0);
    return msgr;
}

/**
*   to be called from inside destruct functions for sub-classes
*/
JXTA_DECLARE(void) jxta_endpoint_messenger_destruct(JxtaEndpointMessenger * msgr)
{

    if (msgr->address)
        JXTA_OBJECT_RELEASE(msgr->address);
    if (msgr->active_q)
        JXTA_OBJECT_RELEASE(msgr->active_q);
    if (msgr->pending_q)
        JXTA_OBJECT_RELEASE(msgr->pending_q);
    if (msgr->ts)
        JXTA_OBJECT_RELEASE(msgr->ts);
    if (msgr->mutex)
        apr_thread_mutex_destroy(msgr->mutex);
    if (msgr->pool)
        apr_pool_destroy(msgr->pool);
}

