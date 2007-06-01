/* 
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: EndpointRouter.h,v 1.5 2005/08/03 05:51:14 slowhog Exp $
 */


#ifndef __EndpointRouter_H__
#define __EndpointRouter_H__

#include "Advertisement.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

typedef struct _EndpointRouter EndpointRouter;

JXTA_DECLARE(EndpointRouter *) EndpointRouter_new();
JXTA_DECLARE(EndpointRouter *) EndpointRouter_set_handlers(EndpointRouter *, XML_Parser, void *);
void EndpointRouter_delete(EndpointRouter *);
JXTA_DECLARE(void) EndpointRouter_print_XML(EndpointRouter *, FILE * stream);
JXTA_DECLARE(EndpointRouter *) EndpointRouter_parse_charbuffer(const char *, int len);
JXTA_DECLARE(EndpointRouter *) EndpointRouter_parse_file(FILE * stream);

JXTA_DECLARE(char *) EndpointRouter_get_EndpointRouter(EndpointRouter *);
JXTA_DECLARE(void) EndpointRouter_set_EndpointRouter(EndpointRouter *, char *);

JXTA_DECLARE(char *) EndpointRouter_get_Version(EndpointRouter *);
JXTA_DECLARE(void) EndpointRouter_set_Version(EndpointRouter *, char *);

JXTA_DECLARE(char *) EndpointRouter_get_Type(EndpointRouter *);
JXTA_DECLARE(void) EndpointRouter_set_Type(EndpointRouter *, char *);

JXTA_DECLARE(char *) EndpointRouter_get_DestPeer(EndpointRouter *);
JXTA_DECLARE(void) EndpointRouter_set_DestPeer(EndpointRouter *, char *);

JXTA_DECLARE(char *) EndpointRouter_get_RoutingPeer(EndpointRouter *);
JXTA_DECLARE(void) EndpointRouter_set_RoutingPeer(EndpointRouter *, char *);

JXTA_DECLARE(char *) EndpointRouter_get_RoutingPeerAdv(EndpointRouter *);
JXTA_DECLARE(void) EndpointRouter_set_RoutingPeerAdv(EndpointRouter *, char *);

JXTA_DECLARE(char *) EndpointRouter_get_NbofHops(EndpointRouter *);
JXTA_DECLARE(void) EndpointRouter_set_NbofHops(EndpointRouter *, char *);

JXTA_DECLARE(char *) EndpointRouter_get_GatewayForward(EndpointRouter *);
JXTA_DECLARE(void) EndpointRouter_set_GatewayForward(EndpointRouter *, char *);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __EndpointRouter_H__  */
