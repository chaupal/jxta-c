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
 * $Id: jxta_rsrdi.h,v 1.6 2005/09/21 21:16:50 slowhog Exp $
 */


#ifndef __ResolverSrdi_H__
#define __ResolverSrdi_H__

#include "jxta_id.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _ResolverSrdi ResolverSrdi;

/**
 * create a new ResolverSrdi object
 * @return pointer to the resolver srdi object
 */
JXTA_DECLARE(ResolverSrdi *) jxta_resolver_srdi_new(void);

/**
 * create a new ResolverSrdi object with inital given values
 * @param handlername associated with the message
 * @param message
 * @return pointer to the resolver message object
 */
JXTA_DECLARE(ResolverSrdi *) jxta_resolver_srdi_new_1(JString *, JString *, Jxta_id *);

/**
 * frees the ResolverSrdi object
 * @param ResolverSrdi the resolver message object to free
 */
void jxta_resolver_srdi_free(ResolverSrdi *);

/**
 * @param ResolverSrdi the resolver message object
 * return a JString represntation of the message
 * it is the responsiblity of the caller to release the JString object
 * @param adv a pointer to the message.
 * @return JString representaion of the message
 */
Jxta_status jxta_resolver_srdi_get_xml(ResolverSrdi *, JString **);

/**
 * @param ResolverSrdi the resolver message object
 * parse the char buffer 
 * @param pointer to the buffer to parse
 * @param len length of the buffer
 */
JXTA_DECLARE(void) jxta_resolver_srdi_parse_charbuffer(ResolverSrdi *, const char *, int len);

/**
 * @param ResolverSrdi the resolver message object
 * parse a file into a ResolverSrdi object
 * @param pointer to the FILE to parse
 * @param len length of the buffer
 */
JXTA_DECLARE(void) jxta_resolver_srdi_parse_file(ResolverSrdi *, FILE * stream);


/**
 * get the message credential
 * @param ResolverSrdi the resolver message object
 * @return Jstring the credential
 */
JXTA_DECLARE(JString *) jxta_resolver_srdi_get_credential(ResolverSrdi *);

/**
 * set the message credential
 * @param ResolverSrdi the resolver message object
 * @param Jstring the credential
 */
JXTA_DECLARE(void) jxta_resolver_srdi_set_credential(ResolverSrdi *, JString *);

/**
 * get the message handlername
 * @param ResolverSrdi the resolver message object
 * @param JString pointer to JString to retrun the handlername pointer
 */

JXTA_DECLARE(void) jxta_resolver_srdi_get_handlername(ResolverSrdi *, JString ** str);

/**
 * set the message HandlerName
 * @param ResolverSrdi the resolver message object
 * @param Jstring the HandlerName
 */

JXTA_DECLARE(void) jxta_resolver_srdi_set_handlername(ResolverSrdi *, JString *);

/**
 * get the message payload
 * @param ResolverSrdi the resolver message object
 * @return Jstring the Payload
 */
JXTA_DECLARE(void) jxta_resolver_srdi_get_payload(ResolverSrdi *, JString **);

/**
 * get the message payload
 * @param ResolverSrdi the resolver message object
 * @param Jstring the Payload
 */
JXTA_DECLARE(void) jxta_resolver_srdi_set_payload(ResolverSrdi *, JString *);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __ResolverSrdi_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
