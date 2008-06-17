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


#ifndef JXTA_HTTPTRANSPORTADVERTISEMENT_H__
#define JXTA_HTTPTRANSPORTADVERTISEMENT_H__

#include "jxta_advertisement.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_HTTPTransportAdvertisement Jxta_HTTPTransportAdvertisement;

JXTA_DECLARE(Jxta_HTTPTransportAdvertisement *) jxta_HTTPTransportAdvertisement_new(void);
JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_set_handlers(Jxta_HTTPTransportAdvertisement *, XML_Parser, void *);
JXTA_DECLARE(Jxta_status) jxta_HTTPTransportAdvertisement_get_xml(Jxta_HTTPTransportAdvertisement *, JString **);
JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_parse_charbuffer(Jxta_HTTPTransportAdvertisement *, const char *, int len);
JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_parse_file(Jxta_HTTPTransportAdvertisement *, FILE * stream);

JXTA_DECLARE(char *) jxta_HTTPTransportAdvertisement_get_Jxta_HTTPTransportAdvertisement(Jxta_HTTPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_set_Jxta_HTTPTransportAdvertisement(Jxta_HTTPTransportAdvertisement *, char *);

JXTA_DECLARE(JString *) jxta_HTTPTransportAdvertisement_get_Protocol(Jxta_HTTPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_set_Protocol(Jxta_HTTPTransportAdvertisement *, JString *);

JXTA_DECLARE(Jxta_in_addr) jxta_HTTPTransportAdvertisement_get_InterfaceAddress(Jxta_HTTPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_set_InterfaceAddress(Jxta_HTTPTransportAdvertisement *, Jxta_in_addr);

JXTA_DECLARE(JString *) jxta_HTTPTransportAdvertisement_get_ConfigMode(Jxta_HTTPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_set_ConfigMode(Jxta_HTTPTransportAdvertisement *, JString *);

JXTA_DECLARE(Jxta_port) jxta_HTTPTransportAdvertisement_get_Port(Jxta_HTTPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_set_Port(Jxta_HTTPTransportAdvertisement *, Jxta_port);

JXTA_DECLARE(JString *) jxta_HTTPTransportAdvertisement_get_Proxy(Jxta_HTTPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_set_Proxy(Jxta_HTTPTransportAdvertisement *, JString *);

JXTA_DECLARE(Jxta_boolean) jxta_HTTPTransportAdvertisement_get_ProxyOff(Jxta_HTTPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_set_ProxyOff(Jxta_HTTPTransportAdvertisement *, Jxta_boolean);

JXTA_DECLARE(JString *) jxta_HTTPTransportAdvertisement_get_Server(Jxta_HTTPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_set_Server(Jxta_HTTPTransportAdvertisement *, JString *);

JXTA_DECLARE(Jxta_boolean) jxta_HTTPTransportAdvertisement_get_ServerOff(Jxta_HTTPTransportAdvertisement *);
JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_set_ServerOff(Jxta_HTTPTransportAdvertisement *, Jxta_boolean);

JXTA_DECLARE(Jxta_vector *) jxta_HTTPTransportAdvertisement_get_indexes(Jxta_advertisement *);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_HTTPTRANSPORTADVERTISEMENT_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
