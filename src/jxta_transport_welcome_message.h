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

#ifndef __JXTA_TRANSPORT_WELCOME_MESSAGE_H__
#define __JXTA_TRANSPORT_WELCOME_MESSAGE_H__

#include "jxta_apr.h"
#include "jxta_endpoint_address.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

#define MAX_WELCOME_MSG_SIZE 4096

enum Jxta_welcome_message_versions {
    JXTA_WELCOME_MESSAGE_VERSION_UNKNOWN,
    JXTA_WELCOME_MESSAGE_VERSION_1_1,
    JXTA_WELCOME_MESSAGE_VERSION_3_0
};

typedef enum Jxta_welcome_message_versions Jxta_welcome_message_version;

typedef const struct _welcome_message Jxta_welcome_message;

JXTA_DECLARE(Jxta_welcome_message *) welcome_message_new_1(Jxta_endpoint_address * dest_addr, Jxta_endpoint_address * public_addr,
                                             JString *peerid, Jxta_boolean noprop, int message_version );
JXTA_DECLARE( Jxta_welcome_message *) welcome_message_new_2(JString * welcome_str);

JXTA_DECLARE(Jxta_endpoint_address *) welcome_message_get_publicaddr(Jxta_welcome_message * me);

JXTA_DECLARE(Jxta_endpoint_address *) welcome_message_get_destaddr(Jxta_welcome_message * me);

JXTA_DECLARE(JString *) welcome_message_get_peerid(Jxta_welcome_message * me);

JXTA_DECLARE(int) welcome_message_get_message_version(Jxta_welcome_message * me);

JXTA_DECLARE(JString *) welcome_message_get_welcome(Jxta_welcome_message * me);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_TRANSPORT_TCP_CONNECTION_H__ */

/* vi: set ts=4 sw=4 tw=130 et: */
