/*
 * Copyright (c) 2008 Sun Microsystems, Inc.  All rights reserved.
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

/*
    TODO: Send messages - Check
    TODO: Start and stop broadcasts 
*/
static const char *__log_cat = "Monitor";

#include <stddef.h>

#include <openssl/sha.h>
#include <assert.h>

#include "jxta_stdpg_private.h"

#include "jxta_service.h"
#include "jxta_service_private.h"
#include "jxta_pipe_service.h"
#include "jxta_object_type.h"
#include "jxta_log.h"

#include "jxta_rdv_diffusion_msg.h"

#include "jxta_monitor_service.h"
#include "jxta_monitor_config_adv.h"

typedef struct _jxta_monitor_service _jxta_monitor_service;
typedef struct _jxta_monitor_listener _jxta_monitor_listener;
typedef struct _jxta_monitor_callback _jxta_monitor_callback;

static Jxta_monitor_service * jxta_monitor_service_instance = NULL;
static Jxta_status monitor_create_broadcast_adv(Jxta_monitor_service * monitor);
static void *APR_THREAD_FUNC monitor_broadcast_thread(apr_thread_t * thread, void *arg);

/**
*   Publish for a day
**/
static const Jxta_expiration_time MON_BROADCAST_ADVERTISEMENT_LIFETIME = 24L * 60L * 60L * 1000L;

/**
*   Cache for a day.
**/
static const Jxta_expiration_time MON_BROADCAST_ADVERTISEMENT_EXPIRATION = 24L * 60L * 60L * 1000L;

struct _jxta_monitor_service {
    Extends(Jxta_service);

    apr_thread_mutex_t *mutex;
    apr_thread_cond_t * started_cv;


    Jxta_PG *monitor_group;
    Jxta_PG *my_group;
    JString *my_groupid;
    Jxta_advertisement *my_impl_adv;
    Jxta_MonitorConfigAdvertisement *config;

    volatile Jxta_boolean running;
    Jxta_boolean enabled;
    Jxta_boolean mon_group_loaded;
    Jxta_id * group_id;
    Jxta_id * pid;
    Jxta_hashtable *context_ids;
    Jxta_service * service;

    Jxta_hashtable *type_callbacks;

    Jxta_vector *listeners;

    Jxta_boolean broadcast;
    Jxta_pipe_service *pipe_service;
    Jxta_pipe_adv *broadcast_pipe_adv;
    Jxta_pipe *broadcast_pipe;
    Jxta_listener * broadcast_listener;
    Jxta_inputpipe * broadcast_input_pipe;
    Jxta_outputpipe * broadcast_ouput_pipe;

    /* Monitor service gets its own threadpool */
    apr_pool_t *mypool;
    apr_thread_pool_t *thd_pool;

};

struct _jxta_monitor_listener {
    Extends(Jxta_object);
    Jxta_listener *listener;
    char * context;
    char * sub_context;
    char * type;
};

struct _jxta_monitor_callback {
    Extends(Jxta_object);
    Jxta_object *obj;
    Jxta_monitor_callback_func func;
};

static Jxta_status monitor_service_init(Jxta_module * it, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_status status;
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    Jxta_PG *parentgroup=NULL;

    Jxta_monitor_service *self = PTValid(it, Jxta_monitor_service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Initializing ...\n");

    status = jxta_service_init((Jxta_service *) self, group, assigned_id, impl_adv);
    if (JXTA_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR , "Unable to initialize service %d...\n", status);
        return status;
    }

    apr_pool_create(&self->mypool, NULL);
    apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, self->mypool);
    apr_thread_cond_create(&self->started_cv, self->mypool);

    /* advs and groups are jxta_objects that we share */
    if (impl_adv != NULL) {
        JXTA_OBJECT_SHARE(impl_adv);
    }

    self->my_impl_adv = impl_adv;
    self->my_group = group;
    self->group_id = jxta_id_defaultMonPeerGroupID;
    jxta_PG_get_PID(group, &self->pid);
    jxta_id_get_uniqueportion(self->group_id, &(self->my_groupid));

    jxta_PG_get_configadv(group, &conf_adv);

    if (!conf_adv) {
        jxta_PG_get_parentgroup(group, &parentgroup);
        if (parentgroup) {
            jxta_PG_get_configadv(parentgroup, &conf_adv);
            JXTA_OBJECT_RELEASE(parentgroup);
        }
    }

    if (conf_adv != NULL) {
        jxta_PA_get_Svc_with_id(conf_adv, jxta_monitor_classid, &svc);
        if (NULL != svc) {
            self->config = jxta_svc_get_MonitorConfig(svc);
            JXTA_OBJECT_RELEASE(svc);
        }
        JXTA_OBJECT_RELEASE(conf_adv);
    }

    if (NULL == self->config) {
        self->config = jxta_MonitorConfigAdvertisement_new();
        if (self->config == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
            res = JXTA_NOMEM;
            goto FINAL_EXIT;
        }
    }

    self->enabled = jxta_MonitorConfig_is_service_enabled(self->config);

    jxta_advertisement_register_global_handler("jxta:Monitor", (JxtaAdvertisementNewFunc) jxta_monitor_msg_new );

FINAL_EXIT:
    
    if (JXTA_SUCCESS == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Initialized\n");
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Unable to Initialize res:%d\n", res);
    }
    return res;
}

static Jxta_status monitor_service_start(Jxta_module * me, const char *args[])
{
    Jxta_monitor_service * myself = PTValid(me, Jxta_monitor_service);

    Jxta_status res = JXTA_SUCCESS;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Starting ...\n");

    /* Monitor service has own threadpool */

    int init_threads = monitor_config_threads_init(myself->config);
    int max_threads = monitor_config_threads_maximum(myself->config);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Initializing DONE THREADS\n");

    if (NULL == myself->mypool) {
        apr_pool_create(&myself->mypool, NULL);
        if (myself->mypool == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot create pool for threadpools.");
        }
    }

    if (NULL == myself->thd_pool) {
        apr_thread_pool_create(&myself->thd_pool, init_threads, max_threads, myself->mypool);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, FILEANDLINE "-------------------   create threadpool.[%pp]", myself->thd_pool);
        if (myself->thd_pool == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot create threadpool.");
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, FILEANDLINE "-------------------   using threadpool.[%pp]", myself->thd_pool);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Initializing DONE POOL, init_threads=%d, max_threads=%d\n",init_threads, max_threads);

    if (APR_SUCCESS != apr_thread_pool_schedule(myself->thd_pool, monitor_broadcast_thread, myself, 1000, myself));
    {

    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Started\n");
    return res;
}

static void monitor_service_stop(Jxta_module * self)
{
    Jxta_monitor_service *myself = PTValid(self, Jxta_monitor_service);

    myself->running = FALSE;
    myself->broadcast = FALSE;

    jxta_hashtable_clear(myself->type_callbacks);
    jxta_vector_clear(myself->listeners);

    /* stop the thread that processes outgoing messages */
    apr_thread_pool_tasks_cancel(myself->thd_pool, myself);

    if(myself->thd_pool) {
        apr_thread_pool_destroy(myself->thd_pool);
        myself->thd_pool = NULL;
    }
    if (myself->broadcast_listener) {
        jxta_listener_stop(myself->broadcast_listener);
    }
    apr_thread_mutex_lock(myself->mutex);

    if (myself->broadcast_pipe != NULL) {
        JXTA_OBJECT_RELEASE(myself->broadcast_pipe);
        myself->broadcast_pipe = NULL;
    }

    if (myself->broadcast_input_pipe != NULL) {
        JXTA_OBJECT_RELEASE(myself->broadcast_input_pipe);
        myself->broadcast_input_pipe = NULL;
    }

    if (myself->broadcast_ouput_pipe != NULL) {
        JXTA_OBJECT_RELEASE(myself->broadcast_ouput_pipe);
        myself->broadcast_ouput_pipe = NULL;
    }

    if (myself->broadcast_listener != NULL) {
        JXTA_OBJECT_RELEASE(myself->broadcast_listener);
        myself->broadcast_listener = NULL;
    }

    if (myself->monitor_group && myself->mon_group_loaded) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, FILEANDLINE "Stopped.\n");
        myself->mon_group_loaded = FALSE;
        jxta_module_stop((Jxta_module *) myself->monitor_group);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, FILEANDLINE "Stopped.\n");
        JXTA_OBJECT_RELEASE(myself->monitor_group);
        myself->monitor_group = NULL;
    }
    apr_thread_mutex_unlock(myself->mutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
}

/*
 * implementations for service methods
 */
static void monitor_service_get_MIA(Jxta_service * it, Jxta_advertisement ** mia)
{
    Jxta_monitor_service *self = PTValid(it, Jxta_monitor_service);

    *mia = JXTA_OBJECT_SHARE(self->my_impl_adv);
}

static void monitor_service_get_interface(Jxta_service * it, Jxta_service ** serv)
{
    Jxta_monitor_service *self = PTValid(it, Jxta_monitor_service);

    JXTA_OBJECT_SHARE(self);
    *serv = (Jxta_service *) self;
}

/**
 * Monitor service methods
 */
typedef Jxta_service_methods Jxta_monitor_service_methods;

static const Jxta_monitor_service_methods jxta_monitor_service_methods = {
    {
     "Jxta_module_methods",
     monitor_service_init,
     monitor_service_start,
     monitor_service_stop},
    "Jxta_service_methods",
    monitor_service_get_MIA,
    monitor_service_get_interface,
    service_on_option_set
};


static _jxta_monitor_service *monitor_service_construct(_jxta_monitor_service * service ,
                                                       const Jxta_monitor_service_methods * methods)
{
    Jxta_service_methods* service_methods = PTValid(methods, Jxta_service_methods);

    jxta_service_construct((Jxta_service *) service, (const Jxta_service_methods *) service_methods);
    service->thisType = "Jxta_monitor_service";

    service->enabled = FALSE;
    service->mon_group_loaded = FALSE;
    service->running = FALSE;
    service->type_callbacks = jxta_hashtable_new(0);
    service->listeners = jxta_vector_new(0);
    service->my_impl_adv = NULL;
    service->monitor_group = NULL;
    service->my_group = NULL;
    service->my_groupid = NULL;
    service->context_ids = jxta_hashtable_new(0);
    service->thd_pool = NULL;
    service->mypool = NULL;
    service->broadcast_pipe_adv = NULL;

    return service;
}

static void monitor_service_destruct(_jxta_monitor_service * service)
{
    Jxta_monitor_service* me = PTValid(service, Jxta_monitor_service);

    if(me->mutex) {
        apr_thread_mutex_destroy(me->mutex);
    }

    if (me->started_cv) {
        apr_thread_cond_destroy(me->started_cv);
    }

    me->my_group = NULL;
    jxta_monitor_service_instance = NULL;

    if (me->my_groupid) {
        JXTA_OBJECT_RELEASE(me->my_groupid);
        me->my_groupid = NULL;
    }

    if (me->context_ids) {
        JXTA_OBJECT_RELEASE(me->context_ids);
        me->context_ids = NULL;
    }

    if (me->my_impl_adv) {
        JXTA_OBJECT_RELEASE(me->my_impl_adv);
        me->my_impl_adv = NULL;
    }

    if (me->type_callbacks) {
        Jxta_vector * entries;

        entries = jxta_hashtable_values_get(me->type_callbacks);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "type_callbacks size: %d\n", jxta_vector_size(entries));
        JXTA_OBJECT_RELEASE(entries);

        JXTA_OBJECT_RELEASE(me->type_callbacks);
        me->type_callbacks = NULL;
    }

    if (me->config) {
        JXTA_OBJECT_RELEASE(me->config);
        me->config = NULL;
    }

    if (me->listeners) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Listener size: %d\n", jxta_vector_size(me->listeners));

        JXTA_OBJECT_RELEASE(me->listeners);
        me->listeners = NULL;
    }

    if (me->pipe_service != NULL) {
        JXTA_OBJECT_RELEASE(me->pipe_service);
        me->pipe_service = NULL;
    }
    if (me->broadcast_pipe_adv != NULL) {
        JXTA_OBJECT_RELEASE(me->broadcast_pipe_adv);
        me->broadcast_pipe_adv = NULL;
    }

    if (me->broadcast_pipe != NULL) {
        JXTA_OBJECT_RELEASE(me->broadcast_pipe);
        me->broadcast_pipe = NULL;
    }

    if (me->broadcast_input_pipe != NULL) {
        JXTA_OBJECT_RELEASE(me->broadcast_input_pipe);
        me->broadcast_input_pipe = NULL;
    }

    if (me->broadcast_ouput_pipe != NULL) {
        JXTA_OBJECT_RELEASE(me->broadcast_ouput_pipe);
        me->broadcast_ouput_pipe = NULL;
    }

    if (me->broadcast_listener != NULL) {
        JXTA_OBJECT_RELEASE(me->broadcast_listener);
        me->broadcast_listener = NULL;
    }

    if (me->pid) {
        JXTA_OBJECT_RELEASE(me->pid);
    }
    me->thisType = NULL;

    jxta_service_destruct((Jxta_service *) me);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destruction finished\n");
}

static void monitor_service_instance_delete(Jxta_object * addr)
{
    Jxta_monitor_service *myself = (Jxta_monitor_service *) addr;

    monitor_service_destruct(myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Deleting Monitor [%pp]\n", myself);

    memset(myself, 0xdd, sizeof(Jxta_monitor_service));

    free(myself);
}

/**
 * create a monitor 
*/

Jxta_monitor_service *jxta_monitor_service_new_instance(void)
{
    if (NULL != jxta_monitor_service_instance) {
        if (jxta_monitor_service_instance->enabled) {
            return jxta_monitor_service_instance;
        } else {
            return NULL;
        }
    }
    Jxta_monitor_service *service = (Jxta_monitor_service *) calloc(1, sizeof(Jxta_monitor_service));
    if (service == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return NULL;
    }

    JXTA_OBJECT_INIT(service, monitor_service_instance_delete, 0);

    if (NULL == monitor_service_construct(service, &jxta_monitor_service_methods)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(service);
        service = NULL;
    }
    jxta_monitor_service_instance = service;

    return service;
}

Jxta_monitor_service *jxta_monitor_service_get_instance(void) 
{
    Jxta_monitor_service * service = NULL;
    if (NULL != jxta_monitor_service_instance) {
        if (jxta_monitor_service_instance->enabled) {
            return jxta_monitor_service_instance;
        }
    }

    return service;
}

/**
 * set the pipe for broadcast 
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_service_set_broadcast_pipe(Jxta_monitor_service * monitor, Jxta_pipe_adv * pipe_adv)
{
    Jxta_monitor_service *myself = PTValid(monitor, Jxta_monitor_service);

    if (NULL != myself->broadcast_pipe_adv) {
        JXTA_OBJECT_RELEASE(myself->broadcast_pipe_adv);
    }
    myself->broadcast_pipe_adv = JXTA_OBJECT_SHARE(pipe_adv);
    return JXTA_SUCCESS;
}


/**
 * send a monitor message on the broadcast pipe or to an individual peer
 * A peer may be specified for directed messages
 * A listener is specified for messages that are directed to a peer requesting a response
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_service_send_msg(Jxta_monitor_service * monitor, Jxta_monitor_msg * msg, Jxta_id * peer, Jxta_listener * listener)
{
    return JXTA_NOTIMP;
}


/**
 * add a monitor entry. Services use this function to add entries at their own intervals
 * When the service broadcasts at its interval all accrued entries are added.
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_service_add_monitor_entry(Jxta_monitor_service * monitor, const char *context, const char *sub_context, Jxta_monitor_entry * entry, Jxta_boolean flush)
{
    Jxta_monitor_service *myself = PTValid(monitor, Jxta_monitor_service);

    Jxta_status res = JXTA_SUCCESS;
    Jxta_hashtable * sub_context_hash = NULL;
    Jxta_hashtable * type_hash = NULL;
    Jxta_vector *entries_v = NULL;
    Jxta_boolean do_flush = FALSE;

    JXTA_OBJECT_CHECK_VALID(entry);

    apr_thread_mutex_lock(myself->mutex);

    if(myself->context_ids == NULL ) {
        myself->context_ids = jxta_hashtable_new(0);
    }

    if (JXTA_SUCCESS != jxta_hashtable_get(myself->context_ids, context, strlen(context) + 1, JXTA_OBJECT_PPTR(&sub_context_hash))) {

        sub_context_hash = jxta_hashtable_new(0);
        if (NULL == sub_context_hash) {
            res = JXTA_NOMEM;
            goto FINAL_EXIT;
        }
        jxta_hashtable_put(myself->context_ids, context, strlen(context) + 1, (Jxta_object *) sub_context_hash);
    } else if (flush) {
        do_flush = TRUE;
    }
    //if there is no type hashtable for that sub_context
    if (JXTA_SUCCESS != jxta_hashtable_get(sub_context_hash, sub_context, strlen(sub_context) + 1, JXTA_OBJECT_PPTR(&type_hash))) {
        type_hash = jxta_hashtable_new(0);
        jxta_hashtable_put(sub_context_hash, sub_context, strlen(sub_context) +1, (Jxta_object *) type_hash);
    }

    char * type = NULL;
    jxta_monitor_entry_get_type(entry, &type);
    if (JXTA_SUCCESS != jxta_hashtable_get(type_hash, type, strlen(type) +1, JXTA_OBJECT_PPTR(&entries_v))){
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Creating vector that needs to be cleaned\n");
        entries_v = jxta_vector_new(0);
        jxta_hashtable_put(type_hash, type, strlen(type) +1, (Jxta_object *) entries_v);
        do_flush = FALSE;
    }

    if (NULL != entries_v) {
        if (flush && do_flush) {
            jxta_vector_clear(entries_v);
        }
        res = jxta_vector_add_object_last( entries_v, (Jxta_object*) entry );
    }

FINAL_EXIT:

    apr_thread_mutex_unlock(myself->mutex);
    
    if(type_hash)
        JXTA_OBJECT_RELEASE(type_hash);
    if (sub_context_hash)
        JXTA_OBJECT_RELEASE(sub_context_hash);
    if (entries_v)
        JXTA_OBJECT_RELEASE(entries_v);

    return res;
}

static void monitor_listener_delete(Jxta_object * me)
{
    Jxta_monitor_listener *myself = (Jxta_monitor_listener *) me;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Deleting Monitor Listener [%pp]\n", myself);

    if (NULL != myself->listener) {
        jxta_listener_stop(myself->listener);
        JXTA_OBJECT_RELEASE(myself->listener);
    }

    if (NULL != myself->context) {
        free(myself->context);
    }

    if (NULL != myself->sub_context) {
        free(myself->sub_context);
    }

    if (NULL != myself->type) {
        free(myself->type);
    }
    memset(myself, 0xdd, sizeof(Jxta_monitor_listener));

    free(myself);
}

JXTA_DECLARE(Jxta_status) jxta_monitor_service_listener_new(Jxta_listener_func listener_func, const char *context, const char *sub_context, const char * type, Jxta_monitor_listener ** monitor_listener)
{
    Jxta_monitor_listener * new_listener = NULL;

    new_listener = calloc(1, sizeof(struct _jxta_monitor_listener));

    JXTA_OBJECT_INIT(new_listener, monitor_listener_delete, NULL);


    new_listener->listener = jxta_listener_new(listener_func, NULL, 1, 200);
    jxta_listener_start(new_listener->listener);

    if (NULL != context) {
        new_listener->context = strdup(context);
    }

    if (NULL != sub_context) {
        new_listener->sub_context = strdup(sub_context);
    }

    if (NULL != type) {
        new_listener->type = strdup(type);
    }

    *monitor_listener = new_listener;
    return JXTA_SUCCESS;
}

static void monitor_callback_delete(Jxta_object * addr) 
{
    _jxta_monitor_callback *myself = (_jxta_monitor_callback *) addr;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Deleting Monitor callback [%pp]\n", myself);

    if (myself->obj)
        JXTA_OBJECT_RELEASE(myself->obj);

    memset(myself, 0xdd, sizeof(_jxta_monitor_callback));

    free(myself);
}

JXTA_DECLARE(Jxta_status) jxta_monitor_service_register_callback(Jxta_monitor_service * monitor, Jxta_monitor_callback_func func, const char * monitor_type, Jxta_object * obj)
{
    Jxta_monitor_service *myself = PTValid(monitor, Jxta_monitor_service);

    Jxta_status res = JXTA_SUCCESS;
    unsigned int i;
    Jxta_vector * types = NULL;
    Jxta_boolean monitor_it = TRUE;

    jxta_MonitorConfig_get_disabled_types(myself->config, &types);

    for (i=0; NULL != types && monitor_it && i < jxta_vector_size(types); i++) {
        JString *type_j = NULL;

        res = jxta_vector_get_object_at(types, JXTA_OBJECT_PPTR(&type_j), i);
        if (res != JXTA_SUCCESS) {
            goto FINAL_EXIT;
        }
        if (!strncmp(monitor_type, jstring_get_string(type_j), strlen(monitor_type))) {
            monitor_it = FALSE;
        }
        JXTA_OBJECT_RELEASE(type_j);
    }

FINAL_EXIT:

    if (monitor_it && (JXTA_SUCCESS != jxta_hashtable_contains(myself->type_callbacks, monitor_type, strlen(monitor_type) +1))) {
        _jxta_monitor_callback *cb;
        cb = calloc(1, sizeof(_jxta_monitor_callback));
        if (NULL != cb) {
            JXTA_OBJECT_INIT(cb, monitor_callback_delete, 0);
            cb->func = func;
            if (obj) {
                JXTA_OBJECT_CHECK_VALID(obj);
                cb->obj = JXTA_OBJECT_SHARE(obj);
            }
            jxta_hashtable_put(myself->type_callbacks, monitor_type, strlen(monitor_type) + 1, (Jxta_object *) cb);
            JXTA_OBJECT_RELEASE(cb);
        } else {
            res = JXTA_NOMEM;
        }
    } else if (!monitor_it) {
        res = JXTA_NOT_CONFIGURED;
    }
    if (NULL != types)
        JXTA_OBJECT_RELEASE(types);

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_service_remove_callback(Jxta_monitor_service * monitor, const char * monitor_type)
{
    Jxta_monitor_service *myself = PTValid(monitor, Jxta_monitor_service);

    return jxta_hashtable_del(myself->type_callbacks, monitor_type, strlen(monitor_type) + 1, NULL);
}

static void processIncomingMessage(Jxta_monitor_service * me, Jxta_message * msg)
{
    Jxta_status res;
    Jxta_message_element * el = NULL;
    Jxta_bytevector * value;
    JString * string;

    res = jxta_message_get_element_2(msg, "jxta" , JXTA_MONITOR_MSG_ELEMENT_NAME, &el);
    if (JXTA_SUCCESS == res) {
        Jxta_monitor_msg * adv;
        Jxta_vector * entries;

        value = jxta_message_element_get_value(el);
        string = jstring_new_3(value);
        JXTA_OBJECT_RELEASE(value);


        adv = jxta_monitor_msg_new();

        jxta_advertisement_parse_charbuffer((Jxta_advertisement *) adv, jstring_get_string(string), jstring_length(string));

        JXTA_OBJECT_RELEASE(string);
        string = NULL;

        jxta_monitor_msg_get_entries(adv, &entries);

        while (jxta_vector_size(entries) > 0) {
            unsigned int i;
            Jxta_monitor_entry *entry;

            jxta_vector_remove_object_at(entries, JXTA_OBJECT_PPTR(&entry), 0);
            for (i=0; i < jxta_vector_size(me->listeners); i++) {
                Jxta_monitor_listener *mon_listener;
                res =jxta_vector_get_object_at(me->listeners, JXTA_OBJECT_PPTR(&mon_listener), i);
                if (JXTA_SUCCESS != res) {
                    continue;
                }
                JXTA_OBJECT_SHARE(entry);
                jxta_listener_process_object(mon_listener->listener, (Jxta_object *) entry);
                JXTA_OBJECT_RELEASE(mon_listener);
            }
            JXTA_OBJECT_RELEASE(entry);
        }
        JXTA_OBJECT_RELEASE(entries);
        jxta_advertisement_get_xml((Jxta_advertisement *) adv, &string);

        if (NULL != string) {
            Jxta_id * peerid;
            JString * id_j;

            peerid = jxta_monitor_msg_get_peer_id((Jxta_monitor_msg *) adv);
            jxta_id_to_jstring(peerid, &id_j);
            JXTA_OBJECT_RELEASE(peerid);
            JXTA_OBJECT_RELEASE(id_j);
            JXTA_OBJECT_RELEASE(string);
        }

        JXTA_OBJECT_RELEASE(adv);
    }
    if (el)
        JXTA_OBJECT_RELEASE(el);
}

static void JXTA_STDCALL message_listener(Jxta_object * obj, void *arg)
{
    Jxta_monitor_service *me = PTValid(arg, Jxta_monitor_service);
    Jxta_message *msg = (Jxta_message *) obj;

    JXTA_OBJECT_CHECK_VALID(msg);

    processIncomingMessage(me, msg);


}

JXTA_DECLARE(Jxta_status) jxta_monitor_service_add_listener(Jxta_monitor_service * monitor, Jxta_monitor_listener *listener)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_monitor_service *myself = PTValid(monitor, Jxta_monitor_service);

    if (NULL == myself->broadcast_pipe_adv) {

        res = monitor_create_broadcast_adv(myself);
        if (JXTA_SUCCESS != res) {
            goto FINAL_EXIT;
        }
    }
    if (NULL == myself->broadcast_input_pipe) {
        if (NULL == myself->pipe_service) {
            jxta_PG_get_pipe_service(myself->monitor_group, &myself->pipe_service);
            if (NULL == myself->pipe_service) {
                res = JXTA_FAILED;
                goto FINAL_EXIT;
            }
        }
        if (NULL == myself->broadcast_pipe) {
            res = jxta_pipe_service_timed_connect(myself->pipe_service, myself->broadcast_pipe_adv, 30 * 1000 * 1000, NULL, &myself->broadcast_pipe);
            if (JXTA_SUCCESS != res) {
                goto FINAL_EXIT;
            }
        }
        res = jxta_pipe_get_inputpipe(myself->broadcast_pipe, &myself->broadcast_input_pipe);
    }
    if (NULL == myself->broadcast_listener) {

        myself->broadcast_listener = jxta_listener_new((Jxta_listener_func) message_listener, myself, 1, 200);

        if (myself->broadcast_listener == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create broadcast listener\n");
            res = JXTA_FAILED;
            goto FINAL_EXIT;
        }
        jxta_listener_start(myself->broadcast_listener);

        jxta_inputpipe_add_listener(myself->broadcast_input_pipe, myself->broadcast_listener);
    }

    jxta_vector_add_object_last(myself->listeners, (Jxta_object *) listener);

FINAL_EXIT:

    return res;
}


/**
 * Remove a previous listener
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_service_remove_listener(Jxta_monitor_service * monitor, Jxta_monitor_listener *listener)
{
    Jxta_monitor_service *myself = PTValid(monitor, Jxta_monitor_service);
    Jxta_status res = JXTA_SUCCESS;

    JXTA_OBJECT_CHECK_VALID(listener);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Removing listener\n");

    if (jxta_vector_remove_object(myself->listeners, (Jxta_object *) listener) < 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Unable to remove listener\n");
        return JXTA_FAILED;
    }

    if (jxta_vector_size(myself->listeners) < 1) {
        jxta_listener_stop(myself->broadcast_listener);
        res = jxta_inputpipe_remove_listener(myself->broadcast_input_pipe, myself->broadcast_listener);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Unable to remove broadcast listener from the pipe\n");
        }
        JXTA_OBJECT_RELEASE(myself->broadcast_listener);
        myself->broadcast_listener = NULL;
    }
    return res;
}

static Jxta_status monitor_create_broadcast_adv(Jxta_monitor_service * monitor)
{
    Jxta_monitor_service *myself = PTValid(monitor, Jxta_monitor_service);
    Jxta_status res=JXTA_SUCCESS;
    Jxta_id *pid = NULL;
    JString * pipe_id_j = NULL;
    JString * pipe_name_j = NULL;
    Jxta_id * monitor_id = NULL;

    jxta_PG_get_GID(monitor->monitor_group, &monitor_id);
    if (NULL == monitor_id) {
        res = JXTA_FAILED;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Error getting Monitor group id\n");
        goto FINAL_EXIT;
    }

    myself->broadcast_pipe_adv = jxta_pipe_adv_new();

    /* create pipe id in the designated group */
    res = jxta_id_pipeid_new_2(&pid, (Jxta_PGID *) monitor_id, (unsigned char *) MONITOR_BROADCAST_SEED, MONITOR_BROADCAST_SEED_LENGTH);

    if (res != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(myself->broadcast_pipe_adv);
        myself->broadcast_pipe_adv = NULL;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Error creating Monitor broadcast pipe id\n");
        goto FINAL_EXIT;
    }
    jxta_id_to_jstring(pid, &pipe_id_j);

    jxta_pipe_adv_set_Id(myself->broadcast_pipe_adv, jstring_get_string(pipe_id_j));
    jxta_pipe_adv_set_Type(myself->broadcast_pipe_adv, JXTA_PROPAGATE_PIPE);
    jxta_pipe_adv_set_Name(myself->broadcast_pipe_adv, jstring_get_string(pipe_name_j));
    jxta_pipe_adv_set_Desc(myself->broadcast_pipe_adv, "Jxta monitor Pipe");

FINAL_EXIT:

    if (monitor_id)
        JXTA_OBJECT_RELEASE(monitor_id);
    if (pid)
        JXTA_OBJECT_RELEASE(pid);

    if (NULL != pipe_id_j) {
        JXTA_OBJECT_RELEASE(pipe_id_j);
    }
    if (NULL != pipe_name_j) {
        JXTA_OBJECT_RELEASE(pipe_name_j);
    }
    return res;
}

static Jxta_status monitor_create_broadcast_pipe(Jxta_monitor_service * monitor) 
{
    Jxta_status res = JXTA_SUCCESS;

    if (NULL == monitor->broadcast_pipe_adv) {
        res = monitor_create_broadcast_adv(monitor);
        if (JXTA_SUCCESS != res) {
            goto FINAL_EXIT;
        }
    }
    if (NULL == monitor->pipe_service) {
        jxta_PG_get_pipe_service(monitor->monitor_group, &monitor->pipe_service);
    }
    res = jxta_pipe_service_timed_connect(monitor->pipe_service, monitor->broadcast_pipe_adv, 30 * 1000 * 1000, NULL, &monitor->broadcast_pipe);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot get output pipe \n");
        goto FINAL_EXIT;
    }

    res = jxta_pipe_get_outputpipe(monitor->broadcast_pipe, &monitor->broadcast_ouput_pipe);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot get output pipe \n");
        goto FINAL_EXIT;
    }


FINAL_EXIT:


    return res;
}

static Jxta_status monitor_build_broadcast_message(Jxta_monitor_service * monitor, const char *context, const char *sub_context, const char *type, Jxta_monitor_msg ** msg)
{
    Jxta_status res = JXTA_SUCCESS;

    *msg = jxta_monitor_msg_new();
    if (NULL == *msg) {
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }

    jxta_monitor_msg_set_peer_id(*msg, monitor->pid);

    if (NULL == context && NULL == sub_context && NULL == type) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Setting broadcast entries \n");
        res = jxta_monitor_msg_set_entries(*msg, monitor->context_ids);
    } else {
        /* TODO: filter the entries by parms */
    }

FINAL_EXIT:

    return res;
}

static Jxta_status monitor_send_broadcast_message(Jxta_monitor_service * myself, Jxta_monitor_msg * b_msg)
{

    Jxta_status res;
    Jxta_message *msg = NULL;
    Jxta_message_element *el = NULL;
    JString *monitor_xml = NULL;

    res = jxta_monitor_msg_get_xml(b_msg, &monitor_xml);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to generate monitor msg xml. [%pp]\n", myself);
        goto FINAL_EXIT;
    }

    msg = jxta_message_new();

    if (NULL == msg) {
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }
    el = jxta_message_element_new_2("jxta", JXTA_MONITOR_MSG_ELEMENT_NAME, "text/xml",
                                    jstring_get_string(monitor_xml), jstring_length(monitor_xml), NULL);

    if (NULL == el) {
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }

    jxta_message_add_element(msg, el);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sending broadcast message\n");
    res = jxta_outputpipe_send(myself->broadcast_ouput_pipe, msg);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error sending monitor message\n");
    }

FINAL_EXIT:

    if (monitor_xml)
        JXTA_OBJECT_RELEASE(monitor_xml);

    if (el)
        JXTA_OBJECT_RELEASE(el);

    if (msg)
        JXTA_OBJECT_RELEASE(msg);

    return res;
}


static void *APR_THREAD_FUNC monitor_broadcast_thread(apr_thread_t * thread, void *arg)
{
    Jxta_status res = JXTA_SUCCESS;
    apr_interval_time_t interval;
    apr_status_t apr_res = APR_SUCCESS;
    Jxta_rdv_service * rdv = NULL;
    Jxta_monitor_service *myself = PTValid(arg, Jxta_monitor_service);
    Jxta_monitor_msg * msg = NULL;

    if (!myself->mon_group_loaded && NULL == myself->monitor_group) {
        char *config_name = NULL;

        jxta_MonitorConfigAdvertisement_get_config_name(myself->config, &config_name);
        if (NULL != config_name) {
            res = jxta_PG_new_monpg(&myself->monitor_group, NULL);
            free(config_name);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create Monitor PeerGroup res:%d\n", res);
                goto FINAL_EXIT;
            }
            myself->mon_group_loaded = TRUE;
        } else {
            myself->monitor_group = JXTA_OBJECT_SHARE(myself->my_group);
        }
        myself->running = TRUE;
        myself->broadcast = TRUE;
        apr_thread_mutex_lock(myself->mutex);
        apr_thread_cond_signal(myself->started_cv);
        apr_thread_mutex_unlock(myself->mutex);
    }


    if (!myself->running) {
        goto FINAL_EXIT;
    }

    if (!myself->broadcast) {
        goto RESCHEDULE_EXIT;
    }
    interval = jxta_MonitorConfig_broadcast_interval(myself->config);

    if (interval > 0 && myself->broadcast) {
        jxta_PG_get_rendezvous_service(myself->my_group, &rdv);
        if (NULL == rdv) {
            goto RESCHEDULE_EXIT;
        }
        if (jxta_rdv_service_is_rendezvous(rdv)) {
            char ** cb_keys;
            char ** cb_keysSave;

            cb_keys = jxta_hashtable_keys_get(myself->type_callbacks);
            cb_keysSave = cb_keys;

            while (*cb_keys) {
                _jxta_monitor_callback * monitor_type_cb;
                Jxta_monitor_entry *entry = NULL;
                if (JXTA_SUCCESS == jxta_hashtable_get(myself->type_callbacks, *cb_keys, strlen(*cb_keys) + 1, JXTA_OBJECT_PPTR(&monitor_type_cb))) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Calling service for %s type\n", *cb_keys);
                    monitor_type_cb->func(monitor_type_cb->obj, &entry);
                    if (NULL != entry) {
                        char *context;
                        char *sub_context;
                        jxta_monitor_entry_get_context(entry, &context);
                        jxta_monitor_entry_get_sub_context(entry, &sub_context);
                        jxta_monitor_service_add_monitor_entry(myself, context, sub_context, entry, TRUE);
                        free(context);
                        free(sub_context);
                        JXTA_OBJECT_RELEASE(entry);
                    }
                    JXTA_OBJECT_RELEASE(monitor_type_cb);
                }
                free(*cb_keys);
                cb_keys++;
            }
            free(cb_keysSave);
            /* todo: apply filters for creating the broadcast message */
            res = monitor_build_broadcast_message(myself, NULL, NULL, NULL, &msg);

            if (JXTA_SUCCESS == res) {
                if (NULL == myself->broadcast_ouput_pipe) {
                    res = monitor_create_broadcast_pipe(myself);
                    if (JXTA_SUCCESS != res) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "monitor [%pp]: unable to create broadcast pipe.\n", myself);
                        goto RESCHEDULE_EXIT;
                    }
                }
                res = monitor_send_broadcast_message(myself, msg);
            }
            if (JXTA_SUCCESS == res) {
                if (myself->context_ids) {
                    //TODO clean out what has been sent.
                    JXTA_OBJECT_RELEASE(myself->context_ids);
                    myself->context_ids = NULL;
                    //flush the tables to start fresh
                }
            }
        } else if (NULL == myself->broadcast_listener) {

        }

RESCHEDULE_EXIT:
        /* Reschedule another check. */
        apr_res = apr_thread_pool_schedule(myself->thd_pool, monitor_broadcast_thread, myself, jxta_MonitorConfig_broadcast_interval(myself->config), myself);

        if (APR_SUCCESS != apr_res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "MONITOR [%pp]: Could not reschedule activity.\n", myself);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "MONITOR [%pp]: Rescheduled.\n", myself);
        }
    }

FINAL_EXIT:

    if (msg) {
        JXTA_OBJECT_RELEASE(msg);
    }
    if (rdv) {
        JXTA_OBJECT_RELEASE(rdv);
    }
    return NULL;
}

/* vi: set ts=4 sw=4 tw=130 et: */
