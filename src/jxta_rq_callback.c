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

#include "jxta_rq_callback.h"
#include "jxta_object_type.h"

/** This is the representation fo the actual 
 * callback object in code.  It should stay 
 * opaque to the programmer, and be
 * accessed through the get/set API.
 */
struct _ResolverQueryCallback {
    JXTA_OBJECT_HANDLE;
    ResolverQuery *rq;
    Jxta_boolean walk;
    Jxta_boolean propagate;
};

/**
 * frees the ResolverQueryCallback
 */
static void resolver_query_callback_delete(Jxta_object * me) {
    ResolverQueryCallback * myself = (ResolverQueryCallback *) me;

    if (myself->rq != NULL) {
        JXTA_OBJECT_RELEASE(myself->rq);
    }

    free(myself);
}

JXTA_DECLARE(ResolverQueryCallback *) jxta_resolver_query_callback_new(void) {
    ResolverQueryCallback *myself = (ResolverQueryCallback *) calloc(1, sizeof(ResolverQueryCallback));
    JXTA_OBJECT_INIT(myself, resolver_query_callback_delete, 0);

    myself->rq = NULL;
    myself->walk = FALSE;
    myself->propagate = TRUE;

    return myself;
}

JXTA_DECLARE(ResolverQueryCallback *) jxta_resolver_query_callback_new_1(
                                                ResolverQuery * rq,
                                                Jxta_boolean walk,
                                                Jxta_boolean propagate) {
    ResolverQueryCallback *myself = jxta_resolver_query_callback_new();
    if (rq != NULL)
        myself->rq = JXTA_OBJECT_SHARE(rq);
    myself->walk = walk;
    myself->propagate = propagate;

    return myself;
}

JXTA_DECLARE(ResolverQuery *) jxta_resolver_query_callback_get_rq(ResolverQueryCallback * obj) {
    if (NULL != obj && NULL != obj->rq)
        return JXTA_OBJECT_SHARE(obj->rq);
    else
        return NULL;
}

JXTA_DECLARE(void) jxta_resolver_query_callback_set_rq(ResolverQueryCallback * obj, ResolverQuery * ad) {
    if (NULL != obj) {
        if (NULL != obj->rq) {
            JXTA_OBJECT_RELEASE(obj->rq);
        }
        obj->rq = ad;
    }
}

JXTA_DECLARE(Jxta_boolean) jxta_resolver_query_callback_walk(ResolverQueryCallback * obj) {
    if (NULL != obj)
        return obj->walk;
    else
        return FALSE;
}

JXTA_DECLARE(void) jxta_resolver_query_callback_set_walk(ResolverQueryCallback * obj, Jxta_boolean walk) {
    if (NULL != obj)
        obj->walk = walk;
}

JXTA_DECLARE(Jxta_boolean) jxta_resolver_query_callback_propagate(ResolverQueryCallback * obj) {
    if (NULL != obj)
        return obj->propagate;
    else
        return FALSE;
}

JXTA_DECLARE(void) jxta_resolver_query_callback_set_propagate(ResolverQueryCallback * obj, Jxta_boolean propagate) {
    if (NULL != obj)
        obj->propagate = propagate;
}

