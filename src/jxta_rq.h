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
 * $Id: jxta_rq.h,v 1.10 2005/09/21 22:56:48 mathieu Exp $
 */


#ifndef __ResolverQuery_H__
#define __ResolverQuery_H__

#include "jxta_routea.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _ResolverQuery ResolverQuery;

/**
 * Defines invalid query id
 */
JXTA_DECLARE_DATA const int JXTA_INVALID_QUERY_ID;

/**
 * create a new ResolverQuery object
 * @return pointer to the resolver query object
 */
JXTA_DECLARE(ResolverQuery *) jxta_resolver_query_new(void);

/**
 * create a new ResolverQuery object with inital given values
 * @param handlername associated with the query
 * @param query
 * @param src_peerid  the originator's peerid
 * @param src route route advertisement of the src peer
 * @return pointer to the resolver query object
 */
JXTA_DECLARE(ResolverQuery *) jxta_resolver_query_new_1(JString * handlername, JString * query, Jxta_id * src_peerid,
                                                        Jxta_RouteAdvertisement * route);

/**
 * frees the ResolverQuery object
 * @param ResolverQuery the resolver query object to free
 */
void jxta_resolver_query_free(ResolverQuery * ad);

/**
 * @param ResolverQuery the resolver query object
 * return a JString represntation of the advertisement
 * it is the responsiblity of the caller to release the JString object
 * @param adv a pointer to the advertisement.
 * @return JString representaion of the advertisement
 */
JXTA_DECLARE(Jxta_status) jxta_resolver_query_get_xml(ResolverQuery * adv, JString ** document);

/**
 * @param ResolverQuery the resolver query object
 * parse the char buffer 
 * @param pointer to the buffer to parse
 * @param len length of the buffer
 */
JXTA_DECLARE(void) jxta_resolver_query_parse_charbuffer(ResolverQuery * ad, const char *buf, int len);

/**
 * @param ResolverQuery the resolver query object
 * parse a file into a ResolverQuery object
 * @param pointer to the FILE to parse
 * @param len length of the buffer
 */
JXTA_DECLARE(void) jxta_resolver_query_parse_file(ResolverQuery * ad, FILE * stream);


/**
 * get the message credential
 * @param ResolverQuery the resolver query object
 * @return Jstring the credential
 */
JXTA_DECLARE(JString *) jxta_resolver_query_get_credential(ResolverQuery * ad);

/**
 * set the message credential
 * @param ResolverQuery the resolver query object
 * @param Jstring the credential
 */
JXTA_DECLARE(void) jxta_resolver_query_set_credential(ResolverQuery * ad, JString * credential);

/**
 * get the message source peer id
 * @param ResolverQuery the resolver query object
 * @param Jxta_id the peer's id
 */
JXTA_DECLARE(Jxta_id *) jxta_resolver_query_get_src_peer_id(ResolverQuery * ad);

/**
 * set the message source peer id
 * @param ResolverQuery the resolver query object
 * @param Jxta_id the peer's id
 */
JXTA_DECLARE(void) jxta_resolver_query_set_src_peer_id(ResolverQuery * ad, Jxta_id * id);

/**
 * get the message source peer route
 * @param ResolverQuery the resolver query object
 * @param Jxta_RouteAdvertisement the peer's route
 */
JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_resolver_query_get_src_peer_route(ResolverQuery * ad);

/**
 * set the message source peer route
 * @param ResolverQuery the resolver query object
 * @param Jxta_RouteAdvertisement the peer's src route
 */
JXTA_DECLARE(void) jxta_resolver_query_set_src_peer_route(ResolverQuery * ad, Jxta_RouteAdvertisement * route);

/**
 * get the message handlername
 * @param ResolverQuery the resolver query object
 * @param JString pointer to JString to retrun the handlername pointer
 */

JXTA_DECLARE(void) jxta_resolver_query_get_handlername(ResolverQuery * ad, JString ** str);

/**
 * set the message HandlerName
 * @param ResolverQuery the resolver query object
 * @param Jstring the HandlerName
 */

JXTA_DECLARE(void) jxta_resolver_query_set_handlername(ResolverQuery * ad, JString * handlerName);

/**
 * get the message QueryID
 * @param ResolverQuery the resolver query object
 * @return long QueryID
 */
JXTA_DECLARE(long) jxta_resolver_query_get_queryid(ResolverQuery * ad);

/**
 * get the message QueryID
 * @param ResolverQuery the resolver query object
 * @param long QueryID
 */
JXTA_DECLARE(void) jxta_resolver_query_set_queryid(ResolverQuery * ad, long qid);

/**
 * get the message Query
 * @param ResolverQuery the resolver query object
 * @return Jstring the Query
 */
JXTA_DECLARE(void) jxta_resolver_query_get_query(ResolverQuery * ad, JString ** query);

/**
 * get the message Query
 * @param ResolverQuery the resolver query object
 * @param Jstring the Query
 */
JXTA_DECLARE(void) jxta_resolver_query_set_query(ResolverQuery * ad, JString * query);

 /**
 *
 */
void jxta_resolver_query_set_hopcount(ResolverQuery * ad, int hopcount);

 /**
 *
 */
long jxta_resolver_query_get_hopcount(ResolverQuery * ad);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __ResolverQuery_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
