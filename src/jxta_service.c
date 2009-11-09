/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights
 * reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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

static const char *__log_cat = "SERVICE";

#include "jxta_service_private.h"
#include "jxta_apr.h"
#include "jxta_peergroup.h"
#include "jxta_log.h"

Jxta_status service_on_option_set(Jxta_service * svc, const char *ns, const char *key, const void *old_val, const void *new_val)
{
    return JXTA_SUCCESS;
}

Jxta_service *jxta_service_construct(Jxta_service * svc, Jxta_service_methods const *methods)
{
    Jxta_service *self = NULL;

    Jxta_service_methods* service_methods = PTValid(methods, Jxta_service_methods);
    self = (Jxta_service *) jxta_module_construct((_jxta_module *) svc, (Jxta_module_methods const *) service_methods);

    if (NULL != self) {
        self->thisType = "Jxta_service";
        self->group = NULL;
        self->assigned_id = NULL;
        self->impl_adv = NULL;
        self->event_cbs = NULL;
        self->mutex = NULL;
    }

    return self;
}

void jxta_service_destruct(Jxta_service * svc)
{
    Jxta_service *self = (Jxta_service *) svc;
    JString *id_j=NULL;

    self->group = NULL;

    if (NULL != self->assigned_id) {
        jxta_id_to_jstring(self->assigned_id, &id_j);
        JXTA_OBJECT_RELEASE(self->assigned_id);
    }

    if (NULL != self->impl_adv) {
        JXTA_OBJECT_RELEASE(self->impl_adv);
    }

    self->thisType = NULL;

#if APR_HAS_THREADS
    if (NULL != self->mutex) {
        apr_thread_mutex_destroy(self->mutex);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Jxta_service_init is not called id:%s \n"
                            , NULL != id_j ? jstring_get_string(id_j) : "no id");
    }
    if (NULL != id_j)
        JXTA_OBJECT_RELEASE(id_j);
#endif

    jxta_module_destruct((Jxta_module *) svc);
}

/**
 * Easy access to the vtbl.
 */
#define VTBL ((Jxta_service_methods*) JXTA_MODULE_VTBL(self))

JXTA_DECLARE(void) jxta_service_get_interface(Jxta_service * svc, Jxta_service ** service)
{
    Jxta_service *self = PTValid(svc, Jxta_service);

    (VTBL->get_interface) (self, service);
}

void jxta_service_get_peergroup(Jxta_service * svc, Jxta_PG ** pg)
{
    Jxta_service *self = PTValid(svc, Jxta_service);

    jxta_service_get_peergroup_impl(self, pg);
}

void jxta_service_get_assigned_ID(Jxta_service * svc, Jxta_id ** assignedID)
{
    Jxta_service *self = PTValid(svc, Jxta_service);

    jxta_service_get_assigned_ID_impl(self, assignedID);
}

JXTA_DECLARE(void) jxta_service_get_MIA(Jxta_service * svc, Jxta_advertisement ** mia)
{
    Jxta_service *self = PTValid(svc, Jxta_service);

    (VTBL->get_MIA) (self, mia);
}

Jxta_status jxta_service_init(Jxta_service * svc, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_service *self = PTValid(svc, Jxta_service);
    apr_pool_t *pool;

    self->group = group;
    JXTA_OBJECT_SHARE(assigned_id);
    self->assigned_id = assigned_id;

    if (NULL != impl_adv) {
        JXTA_OBJECT_SHARE(impl_adv);
        self->impl_adv = impl_adv;
    }

    pool = jxta_PG_pool_get(group);
#if APR_HAS_THREADS
    if (APR_SUCCESS != apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, pool)) {
        return JXTA_FAILED;
    }
#endif
    self->opt_tables = apr_hash_make(pool);
    return JXTA_SUCCESS;
}

/*
 * NB the functions above constitute the interface. There are no default implementations
 * of the interface methods, therefore there is no methods table for the class, and there
 * is no new_instance function either.
 * No one is supposed to create a object of this exact class.
 */

void jxta_service_get_interface_impl(Jxta_service * svc, Jxta_service ** intf)
{
    Jxta_service *self = PTValid(svc, Jxta_service);

    *intf = JXTA_OBJECT_SHARE(self);
}

void jxta_service_get_peergroup_impl(Jxta_service * svc, Jxta_PG ** pg)
{
    Jxta_service *self = PTValid(svc, Jxta_service);

    *pg = JXTA_OBJECT_SHARE(self->group);
}

void jxta_service_get_assigned_ID_impl(Jxta_service * svc, Jxta_id ** assignedID)
{
    Jxta_service *self = PTValid(svc, Jxta_service);

    *assignedID = JXTA_OBJECT_SHARE(self->assigned_id);
}

void jxta_service_get_MIA_impl(Jxta_service * svc, Jxta_advertisement ** mia)
{
    Jxta_service *self = PTValid(svc, Jxta_service);

    if (NULL != self->impl_adv) {
        JXTA_OBJECT_SHARE(self->impl_adv);
    }

    *mia = self->impl_adv;
}

Jxta_PG *jxta_service_get_peergroup_priv(Jxta_service * self)
{
    return self->group;
}

Jxta_id *jxta_service_get_assigned_ID_priv(Jxta_service * self)
{
    return self->assigned_id;
}

Jxta_advertisement *jxta_service_get_MIA_priv(Jxta_service * self)
{
    return self->impl_adv;
}

JXTA_DECLARE(Jxta_status) jxta_service_lock(Jxta_service * me)
{
#if APR_HAS_THREADS
    return apr_thread_mutex_lock(me->mutex);
#else
    return JXTA_SUCCESS;
#endif
}

JXTA_DECLARE(Jxta_status) jxta_service_unlock(Jxta_service * me)
{
#if APR_HAS_THREADS
    return apr_thread_mutex_unlock(me->mutex);
#else
    return JXTA_SUCCESS;
#endif
}

JXTA_DECLARE(Jxta_status) jxta_service_events_connect(Jxta_service * me, Jxta_callback_fn f, void *arg)
{
    Svc_cb *cur;
    Svc_cb *last;
    Svc_cb *cb;

    cb = calloc(1, sizeof(*cb));
    if (!cb) {
        return JXTA_NOMEM;
    }

    cb->f = f;
    cb->arg = arg;

    jxta_service_lock(me);
    if (!me->event_cbs) {
        me->event_cbs = cb;
        jxta_service_unlock(me);
        return JXTA_SUCCESS;
    }

    for (cur = me->event_cbs; cur; cur = cur->next) {
        if (f == cur->f && arg == cur->arg) {
            free(cb);
            jxta_service_unlock(me);
            return JXTA_BUSY;
        }
        last = cur;
    }

    last->next = cb;
    jxta_service_unlock(me);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_service_events_disconnect(Jxta_service * me, Jxta_callback_fn f, void *arg)
{
    Svc_cb *prev;
    Svc_cb *cb;

    jxta_service_lock(me);
    prev = me->event_cbs;
    for (cb = me->event_cbs; cb; cb = cb->next) {
        if (f == cb->f && arg == cb->arg) {
            prev->next = cb->next;
            free(cb);
            jxta_service_unlock(me);
            return JXTA_SUCCESS;
        }
        prev = cb;
    }
    jxta_service_unlock(me);
    return JXTA_ITEM_NOTFOUND;
}

JXTA_DECLARE(Jxta_status) jxta_service_event_emit(Jxta_service * me, void *event)
{
    Svc_cb *cb;

    /* FIXME: bad big lock, what if the callback doing something cause dead lock? */
    /* Case Point: callback should never be blocking */
    jxta_service_lock(me);
    for (cb = me->event_cbs; cb; cb = cb->next) {
        cb->f(event, cb->arg);
    }
    jxta_service_unlock(me);
    return JXTA_SUCCESS;
}

static Jxta_status option_set(Jxta_service * me, apr_hash_t * ht, const char *ns, const char *key, const void *val,
                              Jxta_boolean notify)
{
    void *old_val = NULL;
    Jxta_status rv;

    if (!ht) {
        ht = apr_hash_make(jxta_PG_pool_get(me->group));
    } else {
        old_val = apr_hash_get(ht, key, APR_HASH_KEY_STRING);
    }

    if (old_val && old_val == val) {
        return JXTA_SUCCESS;
    }

    rv = notify ? (JXTA_SERVICE_VTBL(me)->on_option_set) (me, ns, key, old_val, val) : JXTA_SUCCESS;

    if (JXTA_SUCCESS == rv) {
        apr_hash_set(ht, key, APR_HASH_KEY_STRING, val);
        return JXTA_SUCCESS;
    }

    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_service_options_set(Jxta_service * me, const char *ns, apr_hash_t * options)
{
    apr_hash_t *current;
    apr_hash_index_t *hi;
    const void *k;
    apr_ssize_t kl;
    void *new_val;
    Jxta_status rv;

    jxta_service_lock(me);
    current = apr_hash_get(me->opt_tables, ns, strlen(ns));
    if (!current) {
        apr_hash_set(me->opt_tables, ns, strlen(ns), options);
        jxta_service_unlock(me);
        return JXTA_SUCCESS;
    }

    for (hi = apr_hash_first(NULL, options); hi; apr_hash_next(hi)) {
        apr_hash_this(hi, &k, &kl, &new_val);
        rv = option_set(me, current, ns, k, new_val, JXTA_TRUE);
    }

    jxta_service_unlock(me);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_service_option_set(Jxta_service * me, const char *ns, const char *key, const void *val)
{
    apr_hash_t *current;
    Jxta_status rv;

    jxta_service_lock(me);
    current = apr_hash_get(me->opt_tables, ns, APR_HASH_KEY_STRING);
    rv = option_set(me, current, ns, key, val, JXTA_TRUE);
    jxta_service_unlock(me);
    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_service_option_get(Jxta_service * me, const char *ns, const char *key, void **val)
{
    apr_hash_t *current;

    jxta_service_lock(me);
    current = apr_hash_get(me->opt_tables, ns, APR_HASH_KEY_STRING);
    if (!current) {
        *val = NULL;
        jxta_service_unlock(me);
        return JXTA_ITEM_NOTFOUND;
    }

    *val = apr_hash_get(current, key, APR_HASH_KEY_STRING);
    jxta_service_unlock(me);
    return (*val) ? JXTA_SUCCESS : JXTA_ITEM_NOTFOUND;
}

JXTA_DECLARE(Jxta_status) jxta_service_set_default_qos(Jxta_service * me, Jxta_id * id, const Jxta_qos * qos)
{
    Jxta_status rv;
    char *id_pstr;
    apr_hash_t *ht;

    if (!id) {
        jxta_service_lock(me);
        me->qos_for_all = qos;
        jxta_service_unlock(me);
        return JXTA_SUCCESS;
    }

    rv = jxta_id_to_cstr(id, &id_pstr, jxta_PG_pool_get(me->group));
    if (JXTA_SUCCESS != rv) {
        return rv;
    }

    jxta_service_lock(me);
    ht = apr_hash_get(me->opt_tables, "QoS", APR_HASH_KEY_STRING);
    rv = option_set(me, ht, "QoS", id_pstr, qos, JXTA_FALSE);
    jxta_service_unlock(me);
    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_service_default_qos(Jxta_service * me, Jxta_id * id, const Jxta_qos ** qos)
{
    Jxta_status rv;
    JString *id_jstr;

    if (!id) {
        jxta_service_lock(me);
        *qos = me->qos_for_all;
        jxta_service_unlock(me);
        return JXTA_SUCCESS;
    }

    rv = jxta_id_to_jstring(id, &id_jstr);
    if (JXTA_SUCCESS != rv) {
        *qos = NULL;
        return rv;
    }

    rv = jxta_service_option_get(me, "QoS", jstring_get_string(id_jstr), (void **) qos);
    JXTA_OBJECT_RELEASE(id_jstr);
    return rv;
}

/* vim: set ts=4 sw=4 tw=130 et: */
