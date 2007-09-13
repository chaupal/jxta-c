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

#ifndef __JXTA_TRANSPORT_TLS_PRIVATE_H__
#define __JXTA_TRANSPORT_TLS_PRIVATE_H__

#include "apr_queue.h"
#include "jxta_log.h"
#include <openssl/ssl.h>

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

#define TLS_NAMESPACE "jxtatls"
#define TLS_SERVICE_NAME "TlsTransport"

#define TLS_DISCONNECTED 0
#define TLS_HANDSHAKE 1
#define TLS_CONNECTED 2

//Jxta_transport_tls *jxta_transport_tls_new_instance(void);

typedef struct _jxta_transport_tls_connection Jxta_transport_tls_connection;
typedef struct _jxta_transport_tls_connections Jxta_transport_tls_connections;

typedef struct _tls_connection_buffer {
    Extends(Jxta_object);

    unsigned int pos;
    char *bytes;
    unsigned int size;

    unsigned int seq_number;
} _tls_connection_buffer;

_tls_connection_buffer *transport_tls_buffer_new();
_tls_connection_buffer *transport_tls_buffer_new_1(int size);

void tls_connections_remove_connection(Jxta_transport_tls_connections * self, const char *dest_addr);
void tls_connections_add_connection(Jxta_transport_tls_connections * self, Jxta_transport_tls_connection * connection);

Jxta_transport_tls_connection *tls_connection_new(Jxta_PG * group, Jxta_endpoint_service * endpoint,
                                                  const char *dest_address, Jxta_transport_tls_connections * connections,
                                                  SSL_CTX * ctx);

Jxta_transport_tls_connections *tls_connections_new(Jxta_PG * group, Jxta_endpoint_service * endpoint);
Jxta_transport_tls_connection *tls_connections_get_connection(Jxta_transport_tls_connections * self, const char *dest_address);

void tls_connection_initiate_handshake(Jxta_transport_tls_connection * self);

void tls_connection_receive(Jxta_transport_tls_connection * me, Jxta_message * msg);
Jxta_status tls_connection_wait_for_handshake(Jxta_transport_tls_connection * me, apr_interval_time_t timeout);
Jxta_status tls_connection_send(Jxta_transport_tls_connection * me, Jxta_message * message, apr_interval_time_t timeout);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_TRANSPORT_TLS_PRIVATE_H__ */

/* vim: set ts=4 sw=4 tw=130 et: */
