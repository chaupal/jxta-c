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
 * $Id: $
 */

#ifndef __ResolverQuery_Callback_H_
#define __ResolverQuery_Callback_H_

#include "jxta_rq.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/** callback parameter objectto allow services being called by the resolver
 * to provide additional handling instructions on return
 */

typedef struct _ResolverQueryCallback ResolverQueryCallback;

/**
 * create a new ResolverQueryCallback object
 * @return pointer to the resolver query callback object
 */
JXTA_DECLARE(ResolverQueryCallback *) jxta_resolver_query_callback_new(void);

/**
 * create a new ResolverQueryCallback object with initial given values
 * @param rq the ResolverQuery contained within the callback object
 * @param walk out member instructing the resolver to walk the query
 * @param propagate out member instructing the resolver to propagate the query
 * @return pointer to the ResolverQueryCallback object created
 */
JXTA_DECLARE(ResolverQueryCallback *) jxta_resolver_query_callback_new_1(
                                                ResolverQuery * rq,
                                                Jxta_boolean walk,
                                                Jxta_boolean propagate);

/**
 * get the ResolverQuery
 * @param ResolverQueryCallback the resolver query callback object
 * @return ResolverQuery the resolver query
 */
JXTA_DECLARE(ResolverQuery *) jxta_resolver_query_callback_get_rq(ResolverQueryCallback * obj);

/**
 * set the ResolverQuery
 * @param ResolverQueryCallback the resolver query callback object
 * @param ResolverQuery the resolver query
 */
JXTA_DECLARE(void) jxta_resolver_query_callback_set_rq(ResolverQueryCallback * obj, ResolverQuery * ad);

/**
 * get the walk instruction associated with the resolver query
 * @param ResolverQueryCallback the resolver query callback object
 * @return Jxta_boolean the walk instruction
 */
JXTA_DECLARE(Jxta_boolean) jxta_resolver_query_callback_walk(ResolverQueryCallback * obj);

/**
 * set the walk instruction associated with the resolver query
 * @param ResolverQueryCallback the resolver query callback object
 * @param Jxta_boolean the walk instruction
 */
JXTA_DECLARE(void) jxta_resolver_query_callback_set_walk(ResolverQueryCallback * obj, Jxta_boolean walk);

/**
 * get the propagate instruction associated with the resolver query
 * @param ResolverQueryCallback the resolver query callback object
 * @return Jxta_boolean the propagate instruction
 */
JXTA_DECLARE(Jxta_boolean) jxta_resolver_query_callback_propagate(ResolverQueryCallback * obj);

/**
 * set the propagate instruction associated with the resolver query
 * @param ResolverQueryCallback the resolver query callback object
 * @param Jxta_boolean the propagate instruction
 */
JXTA_DECLARE(void) jxta_resolver_query_callback_set_propagate(ResolverQueryCallback * obj, Jxta_boolean propagate);


#ifdef __cplusplus
#if 0
{
#endif 
}
#endif

#endif /* __ResolverQuery_Callback_H_ */

/* vi: set ts=4 sw=4 tw=130 et: */
