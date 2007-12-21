/*
 * Copyright (c) 2006 Sun Microsystems, Inc.  All rights reserved.
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

#include <assert.h>
#include "jxta_qos.h"
#include "jxta_errno.h"
#include "jstring.h"
#include "jxta_util_priv.h"

struct jxta_qos {
    apr_pool_t *pool;
    apr_hash_t *ht;
#if APR_HAS_THREADS
    apr_thread_mutex_t *mutex;
#endif
    Jxta_boolean immutable;
};

static apr_status_t qos_lock(const Jxta_qos * me)
{
#if APR_HAS_THREADS
    return apr_thread_mutex_lock(me->mutex);
#else
    return APR_SUCCESS;
#endif
}

static apr_status_t qos_unlock(const Jxta_qos * me)
{
#if APR_HAS_THREADS
    return apr_thread_mutex_unlock(me->mutex);
#else
    return APR_SUCCESS;
#endif
}

static apr_status_t qos_cleanup(void *me)
{
    Jxta_qos *_self = me;

#if APR_HAS_THREADS
    apr_thread_mutex_destroy(_self->mutex);
#endif
    return APR_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_qos_create(Jxta_qos ** me, apr_pool_t *p)
{
    *me = apr_pcalloc(p, sizeof(**me));
    if (NULL == *me) {
        return JXTA_NOMEM;
    }
    (*me)->pool = p;

    (*me)->ht = apr_hash_make(p);
    if (NULL == (*me)->ht) {
        return JXTA_NOMEM;
    }

#if APR_HAS_THREADS
    if (APR_SUCCESS != apr_thread_mutex_create(&(*me)->mutex, APR_THREAD_MUTEX_NESTED, p)) {
        return JXTA_FAILED;
    }
#endif

    apr_pool_cleanup_register(p, *me, qos_cleanup, apr_pool_cleanup_null);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_qos_create_1(Jxta_qos **me, const char *xml, apr_pool_t *p)
{
    Jxta_status rv;

    *me = apr_pcalloc(p, sizeof(**me));
    if (NULL == *me) {
        return JXTA_NOMEM;
    }
    (*me)->pool = p;

    rv = xml_to_qos_setting(xml, &(*me)->ht, p);
    if (JXTA_SUCCESS != rv) {
        return rv;
    }

#if APR_HAS_THREADS
    if (APR_SUCCESS != apr_thread_mutex_create(&(*me)->mutex, APR_THREAD_MUTEX_NESTED, p)) {
        return JXTA_FAILED;
    }
#endif

    apr_pool_cleanup_register(p, *me, qos_cleanup, apr_pool_cleanup_null);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_qos_destroy(Jxta_qos * me)
{
    return apr_pool_cleanup_run(me->pool, me, qos_cleanup);
}

JXTA_DECLARE(Jxta_status) jxta_qos_clone(Jxta_qos ** me, const Jxta_qos * src, apr_pool_t *p)
{
    if (!p) {
        p = src->pool;
    }

    *me = apr_pcalloc(p, sizeof(**me));
    if (NULL == *me) {
        return JXTA_NOMEM;
    }
    (*me)->pool = p;

    qos_lock(src);
    (*me)->ht = apr_hash_copy(p, src->ht);
    qos_unlock(src);
    if (NULL == (*me)->ht) {
        return JXTA_NOMEM;
    }

#if APR_HAS_THREADS
    if (APR_SUCCESS != apr_thread_mutex_create(&(*me)->mutex, APR_THREAD_MUTEX_NESTED, p)) {
        return JXTA_FAILED;
    }
#endif

    apr_pool_cleanup_register(p, *me, qos_cleanup, apr_pool_cleanup_null);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_qos_set_immutable(Jxta_qos * me)
{
    me->immutable = JXTA_TRUE;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_boolean) jxta_qos_is_immutable(Jxta_qos * me)
{
    return me->immutable;
}

JXTA_DECLARE(Jxta_status) jxta_qos_set(Jxta_qos * me, const char * name, const char * value)
{
    qos_lock(me);
    if (me->immutable) {
        qos_unlock(me);
        return JXTA_FAILED;
    }

    apr_hash_set(me->ht, name, APR_HASH_KEY_STRING, value);
    qos_unlock(me);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_qos_add(Jxta_qos * me, const char * name, const char * value)
{
    void *old_val = NULL;

    qos_lock(me);
    if (me->immutable) {
        qos_unlock(me);
        return JXTA_FAILED;
    }
    old_val = apr_hash_get(me->ht, name, APR_HASH_KEY_STRING);
    if (old_val) {
        qos_unlock(me);
        return JXTA_BUSY;
    }

    apr_hash_set(me->ht, name, APR_HASH_KEY_STRING, value);
    qos_unlock(me);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_qos_get(const Jxta_qos * me, const char * name, const char ** value)
{
    qos_lock(me);
    *value = apr_hash_get(me->ht, name, APR_HASH_KEY_STRING);
    qos_unlock(me);
    return (*value) ? JXTA_SUCCESS : JXTA_ITEM_NOTFOUND;
}

JXTA_DECLARE(Jxta_status) jxta_qos_set_long(Jxta_qos * me, const char * name, long value)
{
#ifdef UNUSED_VWF
    void * old_val = NULL;
#endif
    qos_lock(me);
    if (me->immutable) {
        qos_unlock(me);
        return JXTA_FAILED;
    }

    apr_hash_set(me->ht, name, APR_HASH_KEY_STRING, apr_ltoa(me->pool, value));
    qos_unlock(me);
    return JXTA_SUCCESS;
}


JXTA_DECLARE(Jxta_status) jxta_qos_add_long(Jxta_qos * me, const char * name, long value)
{
    void * old_val = NULL;

    qos_lock(me);
    if (me->immutable) {
        qos_unlock(me);
        return JXTA_FAILED;
    }

    old_val = apr_hash_get(me->ht, name, APR_HASH_KEY_STRING);
    if (old_val) {
        qos_unlock(me);
        return JXTA_BUSY;
    }

    apr_hash_set(me->ht, name, APR_HASH_KEY_STRING, apr_ltoa(me->pool, value));
    qos_unlock(me);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_qos_get_long(const Jxta_qos * me, const char * name, long * value)
{
    const char * v;
    char * tmp;

    qos_lock(me);
    v = apr_hash_get(me->ht, name, APR_HASH_KEY_STRING);
    qos_unlock(me);
    if (!v) {
        return JXTA_ITEM_NOTFOUND;
    }

    *value = strtol(v, &tmp, 10);
    if (v != tmp && '\0' != *tmp) {
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_qos_setting_to_xml(const Jxta_qos * me, char ** result, apr_pool_t *p)
{
    Jxta_status rv;

    qos_lock(me);
    rv = qos_setting_to_xml(me->ht, result, p ? p : me->pool);
    qos_unlock(me);

    return rv;
}

/*
JXTA_DECLARE(Jxta_status) jxta_qos_support_to_xml(Jxta_qos * me, char ** result, apr_pool_t *p)
{
    apr_hash_index_t *hi = NULL;
    const char * key;
    apr_ssize_t len;
    int cnt;
    JString *buf = NULL;

    cnt = 0;
    qos_lock(me);
    for (hi = apr_hash_first(p, setting); hi; hi = apr_hash_next(hi)) {
        if (cnt++) {
            assert(NULL == buf);
            buf = jstring_new_1(128);
            buf = jstring_append_2("<QoS_Support>");
        }
        apr_hash_this(hi, &key, &len, &val);
        assert(NULL != key);
        assert(NULL != val);
        jstring_append_2(buf, "<QoS>");
        jstring_append_2(buf, key);
        jstring_append_2(buf, "</QoS>");
    }
    qos_unlock(me);
    if (cnt) {
        jstring_append_2(buf, "</QoS_Support>");
        *result = apr_pstrdup(jstring_get_string(buf));
    } else {
        *result = "";
    }

    return JXTA_SUCCESS;
}
*/
/* vim: set ts=4 sw=4 tw=130 et: */
