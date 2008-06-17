/* 
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
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


#ifndef JXTA_TCPTRANSPORTADVERTISEMENT_H__
#define JXTA_TCPTRANSPORTADVERTISEMENT_H__

#include "jxta_advertisement.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_TCPTransportAdvertisement Jxta_TCPTransportAdvertisement;

JXTA_DECLARE(Jxta_TCPTransportAdvertisement *) jxta_TCPTransportAdvertisement_new(void);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_set_handlers(Jxta_TCPTransportAdvertisement *, XML_Parser, void *);
JXTA_DECLARE(Jxta_status) jxta_TCPTransportAdvertisement_get_xml(Jxta_TCPTransportAdvertisement *, JString **);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_parse_charbuffer(Jxta_TCPTransportAdvertisement *, const char *, int len);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_parse_file(Jxta_TCPTransportAdvertisement *, FILE * stream);

JXTA_DECLARE(JString *) jxta_TCPTransportAdvertisement_get_Protocol(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(char *) jxta_TCPTransportAdvertisement_get_Protocol_string(Jxta_advertisement * ad);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_set_Protocol(Jxta_TCPTransportAdvertisement *, JString *);

JXTA_DECLARE(Jxta_port) jxta_TCPTransportAdvertisement_get_Port(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(char *) jxta_TCPTransportAdvertisement_get_Port_string(Jxta_advertisement * ad);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_set_Port(Jxta_TCPTransportAdvertisement *, Jxta_port);

JXTA_DECLARE(Jxta_boolean) jxta_TCPTransportAdvertisement_get_MulticastOff(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_set_MulticastOff(Jxta_TCPTransportAdvertisement *, Jxta_boolean);

JXTA_DECLARE(Jxta_in_addr) jxta_TCPTransportAdvertisement_get_MulticastAddr(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_set_MulticastAddr(Jxta_TCPTransportAdvertisement *, Jxta_in_addr);

JXTA_DECLARE(Jxta_port) jxta_TCPTransportAdvertisement_get_MulticastPort(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_set_MulticastPort(Jxta_TCPTransportAdvertisement *, Jxta_port);

JXTA_DECLARE(int) jxta_TCPTransportAdvertisement_get_MulticastSize(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_set_MulticastSize(Jxta_TCPTransportAdvertisement *, int);

JXTA_DECLARE(JString *) jxta_TCPTransportAdvertisement_get_Server(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(char *) jxta_TCPTransportAdvertisement_get_Server_string(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_set_Server(Jxta_TCPTransportAdvertisement *, JString *);

JXTA_DECLARE(Jxta_in_addr) jxta_TCPTransportAdvertisement_get_InterfaceAddress(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_set_InterfaceAddress(Jxta_TCPTransportAdvertisement *, Jxta_in_addr);
JXTA_DECLARE(const char*) jxta_TCPTransportAdvertisement_InterfaceAddress(Jxta_TCPTransportAdvertisement *);

JXTA_DECLARE(JString *) jxta_TCPTransportAdvertisement_get_ConfigMode(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(char *) jxta_TCPTransportAdvertisement_get_ConfigMode_string(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_set_ConfigMode(Jxta_TCPTransportAdvertisement *, JString *);

JXTA_DECLARE(Jxta_boolean) jxta_TCPTransportAdvertisement_get_ServerOff(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_set_ServerOff(Jxta_TCPTransportAdvertisement *, Jxta_boolean);

JXTA_DECLARE(Jxta_boolean) jxta_TCPTransportAdvertisement_get_ClientOff(Jxta_TCPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_set_ClientOff(Jxta_TCPTransportAdvertisement *, Jxta_boolean);

JXTA_DECLARE(Jxta_vector *) jxta_TCPTransportAdvertisement_get_indexes(Jxta_advertisement *);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_TCPTRANSPORTADVERTISEMENT_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
