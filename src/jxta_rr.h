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


#ifndef __ResolverResponse_H__
#define __ResolverResponse_H__

#include "jstring.h"
#include "jxta_id.h"
#include "jxta_qos.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _ResolverResponse ResolverResponse;
typedef struct _ResolverResponse Jxta_resolver_response;

/**
 * create a new ResolverResponse object
 * @return pointer to the resolver response object
 */
JXTA_DECLARE(ResolverResponse *) jxta_resolver_response_new(void);

/**
 * create a new ResolverResponse object with inital given values
 * @param handlername associated with the query
 * @param response string containing response
 * @param qid  the originator's query id
 * @return pointer to the resolver query object
 */
JXTA_DECLARE(ResolverResponse *) jxta_resolver_response_new_1(JString * handlername, JString * response, long qid);

/**
 * Create a new ResolverResponse object with inital given values
 * @param handlername associated with the query
 * @param response string containing response
 * @param qid  the originator's query id
 * @param res_id the peer id of the responder peer
 * @return pointer to the resolver query object
 */
JXTA_DECLARE(ResolverResponse *) jxta_resolver_response_new_2(JString * handlername, JString * response, long qid,
                                                              Jxta_id * res_pid);

/**
 * @param ResolverResponse the resolver response object
 * return a JString represntation of the advertisement
 * it is the responsiblity of the caller to release the JString object
 * @param adv a pointer to the advertisement.
 * @return Jxta_status
 */
JXTA_DECLARE(Jxta_status) jxta_resolver_response_get_xml(ResolverResponse * ad, JString ** document);

/**
 * @param ResolverResponse the resolver response object
 * parse the char buffer 
 * @param pointer to the buffer to parse
 * @param len length of the buffer
 * @deprecated Use jxta_advertisement_parse_charbuffer instead.
 */
JXTA_DECLARE(Jxta_status) jxta_resolver_response_parse_charbuffer(ResolverResponse * ad, const char *buf, int len);

/**
 * @param ResolverResponse the resolver response object
 * parse a file into a ResolverResponse object
 * @param pointer to the FILE to parse
 * @param len length of the buffer
 * @deprecated Use jxta_advertisement_parse_file instead.
 */
JXTA_DECLARE(Jxta_status) jxta_resolver_response_parse_file(ResolverResponse * ad, FILE * stream);

/**
 * get the message credential
 * @param ResolverQuery the resolver response object
 * @return Jstring the credential
 */
JXTA_DECLARE(JString *) jxta_resolver_response_get_credential(ResolverResponse * ad);

/**
 * set the message credential
 * @param ResolverResponse the resolver response object
 * @param Jstring the credential
 */
JXTA_DECLARE(void) jxta_resolver_response_set_credential(ResolverResponse * ad, JString * credential);

/**
 * Get the peer id of the peer that send this resolver response
 * @param ResolverResponse the resolver response object
 * @param Jxta_id the peer's id
 */
JXTA_DECLARE(Jxta_id *) jxta_resolver_response_get_res_peer_id(ResolverResponse * ad);

/**
 * Set the peer id of the peer that send this resolver response 
 * @param Resolver the resolver response object
 * @param Jxta_id the peer's id
 */
JXTA_DECLARE(void) jxta_resolver_response_set_res_peer_id(ResolverResponse * ad, Jxta_id * id);

/**
 * get the message handlername
 * @param ResolverResponse the resolver response object
 * @param JString pointer to JString to retrun the handlername pointer
 * @return Jxta_id the peer's id
 */
JXTA_DECLARE(void) jxta_resolver_response_get_handlername(ResolverResponse * ad, JString ** str);

/**
 * set the message HandlerName
 * @param ResolverResponse the resolver response object
 * @param Jstring the HandlerName
 */
JXTA_DECLARE(void) jxta_resolver_response_set_handlername(ResolverResponse * ad, JString * handlerName);

/**
 * get the message QueryID
 * @param ResolverResponse the resolver response object
 * @return Jstring the QueryID
 */
JXTA_DECLARE(long) jxta_resolver_response_get_queryid(ResolverResponse * ad);

/**
 * get the message QueryID
 * @param ResolverResponse the resolver response object
 * @param Jstring the QueryID
 */
JXTA_DECLARE(void) jxta_resolver_response_set_queryid(ResolverResponse * ad, long queryID);


/**
 * get the message Query
 * @param ResolverResponse the resolver response object
 * @param JString pointer to JString to retrun the response pointer
 */
JXTA_DECLARE(void) jxta_resolver_response_get_response(ResolverResponse * ad, JString ** str);

/**
 * get the message response
 * @param ResolverResponse the resolver response object
 * @param Jstring the Response
 */
JXTA_DECLARE(void) jxta_resolver_response_set_response(ResolverResponse * ad, JString * response);

JXTA_DECLARE(Jxta_status) jxta_resolver_response_attach_qos(Jxta_resolver_response * me, const Jxta_qos * qos);
JXTA_DECLARE(const Jxta_qos *) jxta_resolver_response_qos(Jxta_resolver_response * me);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __ResolverResponse_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
