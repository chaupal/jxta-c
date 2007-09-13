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

#ifndef __JXTA_TRANSPORT_TCP_CONNECTION_H__
#define __JXTA_TRANSPORT_TCP_CONNECTION_H__

#include "jxta_apr.h"
#include "jxta_endpoint_address.h"
#include "jxta_message.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_transport_tcp Jxta_transport_tcp;
typedef struct _tcp_messenger TcpMessenger;
typedef struct _jxta_transport_tcp_connection Jxta_transport_tcp_connection;

JXTA_DECLARE(Jxta_transport_tcp_connection *) jxta_transport_tcp_connection_new_1(Jxta_transport_tcp * tp,
                                                                                  Jxta_endpoint_address * dest);

JXTA_DECLARE(Jxta_transport_tcp_connection *) jxta_transport_tcp_connection_new_2(Jxta_transport_tcp * tp,
                                                                                  apr_socket_t * inc_socket);

JXTA_DECLARE(Jxta_status) jxta_transport_tcp_connection_start(Jxta_transport_tcp_connection * self);

JXTA_DECLARE(Jxta_status) jxta_transport_tcp_connection_close(Jxta_transport_tcp_connection * self);

JXTA_DECLARE(Jxta_status) jxta_transport_tcp_connection_send_message(Jxta_transport_tcp_connection * conn, Jxta_message * msg);

JXTA_DECLARE(Jxta_endpoint_address *) jxta_transport_tcp_connection_get_destaddr(Jxta_transport_tcp_connection * conn);

JXTA_DECLARE(Jxta_endpoint_address *) jxta_transport_tcp_connection_get_connaddr(Jxta_transport_tcp_connection * conn);

JXTA_DECLARE(Jxta_id *) jxta_transport_tcp_connection_get_destination_peerid(Jxta_transport_tcp_connection * conn);

JXTA_DECLARE(char *) jxta_transport_tcp_connection_get_ipaddr(Jxta_transport_tcp_connection * conn);
JXTA_DECLARE(const char *) jxta_transport_tcp_connection_ipaddr(Jxta_transport_tcp_connection * conn);

JXTA_DECLARE(apr_port_t) jxta_transport_tcp_connection_get_port(Jxta_transport_tcp_connection * conn);

JXTA_DECLARE(Jxta_boolean) jxta_transport_tcp_connection_is_connected(Jxta_transport_tcp_connection * self);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_TRANSPORT_TCP_CONNECTION_H__ */

/* vi: set ts=4 sw=4 tw=130 et: */
