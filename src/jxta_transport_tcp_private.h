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
 * $Id: jxta_transport_tcp_private.h,v 1.18 2007/01/16 22:27:05 slowhog Exp $
 */

#ifndef __JXTA_TRANSPORT_TCP_PRIVATE_H__
#define __JXTA_TRANSPORT_TCP_PRIVATE_H__

#include "jxta_apr.h"
#include "jxta_types.h"
#include "jxta_endpoint_address.h"
#include "jxta_endpoint_service.h"
#include "jxta_endpoint_service.h"
#include "jxta_transport_tcp_connection.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

#define SEND_BUFFER_SIZE	(64 * 1024)
#define RECV_BUFFER_SIZE	(64 * 1024)
#define LINGER_DELAY		(2 * 60 * 1000 * 1000)
#define CONNECTION_TIMEOUT	(1 * 1000 * 1000)
#define MAX_ACCEPT_COUNT_BACKLOG	50

typedef enum e_tcp_connection_state {
    CONN_INIT = 0,
    CONN_CONNECTING,
    CONN_CONNECTED,
    CONN_DISCONNECTING,
    CONN_DISCONNECTED,
} Tcp_connection_state;


Jxta_transport_tcp *jxta_transport_tcp_new_instance(void);

Jxta_endpoint_service *jxta_transport_tcp_get_endpoint_service(Jxta_transport_tcp * me);

const char *jxta_transport_tcp_local_ipaddr_cstr(Jxta_transport_tcp * me);

char *jxta_transport_tcp_get_local_ipaddr(Jxta_transport_tcp * me);

apr_port_t jxta_transport_tcp_get_local_port(Jxta_transport_tcp * me);

JString *jxta_transport_tcp_get_peerid(Jxta_transport_tcp * me);

Jxta_endpoint_address *jxta_transport_tcp_get_public_addr(Jxta_transport_tcp * me);

Jxta_boolean jxta_transport_tcp_get_allow_multicast(Jxta_transport_tcp * me);

void tcp_got_inbound_connection(Jxta_transport_tcp * me, Jxta_transport_tcp_connection * conn);

Jxta_time get_tcp_connection_last_time_used(Jxta_transport_tcp_connection * tcp_connection);

Tcp_connection_state tcp_connection_state(Jxta_transport_tcp_connection *me);
Jxta_status tcp_connection_get_messenger(Jxta_transport_tcp_connection *me, apr_interval_time_t timeout, TcpMessenger **msgr);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_TRANSPORT_TCP_PRIVATE_H__ */

/* vi: set ts=4 sw=4 tw=130 et: */
