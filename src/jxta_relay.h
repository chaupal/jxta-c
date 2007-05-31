/* 
 * Copyright (c) 2002-2004 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_relay.h,v 1.2 2005/01/10 17:19:23 brent Exp $
 */

#ifndef JXTA_RELAY_H__
#define JXTA_RELAY_H__


#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

#define LEASE_REQUEST    "3600000"                             /* 1 hour */
#define REQUEST_TIMEOUT  "120000,-1"
#define LAZY_CLOSE       "keep,true"
#define RELAY_LEASE_ELEMENT "relay:lease"
#define RELAY_LEASE_RENEWAL_DELAY ((Jxta_time) 5 * 60 * 1000 * 1000) /* 5 Minutes */
#define LEASE_REQUEST_TIME    (Jxta_time) 3600000              /* 1 hour */
#define RELAY_ADDRESS "EndpointService:jxta-NetGroup/uuid-DEADBEEFDEAFBABAFEEDBABE0000000F05"
#define RELAY_LEASE_ELEMENT "relay:lease"
#define RELAY_LEASE_REQUEST "relay:request"
#define RELAY_LEASE_RESPONSE "relay:response"
#define RELAY_RESPONSE_ADV "relay:relayAdv"
#define RELAY_CONNECT_REQUEST "connect"
#define RELAY_RESPONSE_CONNECTED "connected"
#define RELAY_RESPONSE_DISCONNECTED "disconnected"


#ifdef __cplusplus
}
#endif

#endif /* JXTA_RELAY_H__  */
