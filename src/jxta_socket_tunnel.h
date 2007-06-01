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
 * $Id: jxta_socket_tunnel.h,v 1.6 2005/09/21 21:16:51 slowhog Exp $
 */

#ifndef __JXTA_SOCKET_TUNNEL_H__
#define __JXTA_SOCKET_TUNNEL_H__

#include "jxta_peergroup.h"
#include "jxta_pipe_adv.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0                           /* to cheat indent */
};
#endif

typedef struct Jxta_socket_tunnel Jxta_socket_tunnel;

/**
 * addr_spec: protocol://addr:port. for example,
 * tcp://192.168.2.100:1234, supported protocols are tcp, udp
 */
JXTA_DECLARE(Jxta_status) jxta_socket_tunnel_create(Jxta_PG * group, const char *addr_spec, Jxta_socket_tunnel ** newobj);

Jxta_status jxta_socket_tunnel_delete(Jxta_socket_tunnel * self);

JXTA_DECLARE(Jxta_status) jxta_socket_tunnel_establish(Jxta_socket_tunnel * self, Jxta_pipe_adv * remote_adv);

JXTA_DECLARE(Jxta_status) jxta_socket_tunnel_accept(Jxta_socket_tunnel * self, Jxta_pipe_adv * local_adv);

JXTA_DECLARE(Jxta_status) jxta_socket_tunnel_teardown(Jxta_socket_tunnel * self);

JXTA_DECLARE(Jxta_boolean) jxta_socket_tunnel_is_established(Jxta_socket_tunnel * self);

JXTA_DECLARE(char *) jxta_socket_tunnel_addr_get(Jxta_socket_tunnel * self);

#if 0   /* to cheat indent */
{
#endif
#ifdef __cplusplus
}   /* extern "C" */
#endif
#endif /* __JXTA_SOCKET_TUNNEL_H__ */
/* vi: set sw=4 ts=4 et tw=130: */
