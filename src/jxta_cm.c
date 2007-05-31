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
 * $Id: jxta_cm.c,v 1.60 2005/02/22 00:32:43 bondolo Exp $
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef WIN32
#include <direct.h>     /* for _mkdir */
#else
#include "config.h"
#endif

#include <apr_dbm.h>

#include "jxtaapr.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_object.h"
#include "jxta_hashtable.h"
#include "jxta_advertisement.h"
#include "jstring.h"
#include "jxta_id.h"
#include "jxta_cm.h"
#include "jxta_srdi.h"
#include "jxta_log.h"

#ifdef WIN32
#define DBTYPE "SDBM"   /* defined for other platform in config.h */
#endif

static const char *__log_cat = "CM";

/*
 * Hashtables want a value to be associated with keys.
 * In one instance we do not need any, so we just use one instance
 * of dummy_object for all values.
 */
typedef struct {
    JXTA_OBJECT_HANDLE;
} Dummy_object;

static void dummy_object_free(Jxta_object * o)
{
    free((void *) o);
}

static Dummy_object *dummy_object_new(void)
{
    Dummy_object *res = (Dummy_object *) malloc(sizeof(Dummy_object));
    JXTA_OBJECT_INIT(res, dummy_object_free, 0);
    return res;
}

typedef struct {
    JXTA_OBJECT_HANDLE;
    apr_dbm_t *data;
    apr_dbm_t *exp;
    Jxta_hashtable *secondary_keys;
    char *name;
} Folder;

struct _jxta_cm {
    JXTA_OBJECT_HANDLE;
    char *root;
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
    Jxta_hashtable *folders;
    Dummy_object *dummy_object;
    Jxta_hashtable *srdi_delta;
    Jxta_boolean record_delta;
};

/* Destroy the cm and free all allocated memory */
static void cm_free(Jxta_object * cm)
{
    Jxta_cm *self = (Jxta_cm *) cm;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "CM free\n");
    JXTA_OBJECT_RELEASE(self->folders);
    JXTA_OBJECT_RELEASE(self->srdi_delta);
    JXTA_OBJECT_RELEASE(self->dummy_object);
    free(self->root);
    apr_thread_mutex_destroy(self->mutex);
    apr_pool_destroy(self->pool);

    /* Free the object itself */
    free(self);
}

/* Create a new Cm Structure */
Jxta_cm *jxta_cm_new(const char *home_directory, Jxta_id * group_id)
{
    Jxta_status res;
    JString *group_id_s = NULL;
    Jxta_cm *self = (Jxta_cm *) malloc(sizeof(Jxta_cm));
    memset(self, 0, sizeof(*self));

    JXTA_OBJECT_INIT(self, cm_free, 0);
    jxta_id_get_uniqueportion(group_id, &group_id_s);

    /* If there is an apr api for this, maybe we should use it */
#ifdef WIN32
    printf("CM home_directory: %s\n", home_directory);
    _mkdir(home_directory);
#else
    mkdir(home_directory, 0755);
#endif
    self->root = malloc(strlen(home_directory) + jstring_length(group_id_s)
                        + 2);
#ifdef WIN32
    sprintf(self->root, "%s\\%s", home_directory, (char *) jstring_get_string(group_id_s));
    printf("CM home_directory: %s\n", self->root);
#else
    sprintf(self->root, "%s/%s", home_directory, (char *) jstring_get_string(group_id_s));
#endif

    self->folders = jxta_hashtable_new_0(3, FALSE);
    self->srdi_delta = jxta_hashtable_new_0(3, FALSE);
    self->record_delta = FALSE;
    res = apr_pool_create(&self->pool, NULL);
    if (res != JXTA_SUCCESS) {
        /* Free the memory that has been already allocated */
        free(self->root);
        free(self);
        return NULL;
    }

    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,        /* nested */
                                  self->pool);
    if (res != JXTA_SUCCESS) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(self->pool);
        free(self->root);
        free(self);
        return NULL;
    }

    self->dummy_object = dummy_object_new();

    JXTA_OBJECT_RELEASE(group_id_s);
    return self;
}


static void folder_free(Jxta_object * f)
{
    Folder *folder = (Folder *) f;
    JXTA_OBJECT_RELEASE(folder->secondary_keys);
    apr_dbm_close(folder->data);
    apr_dbm_close(folder->exp);
    free(folder);
}

static Jxta_status folder_remove_advertisement(Folder * folder, char *primary_key, size_t key_size)
{
    apr_datum_t key;
    Jxta_vector *all_attribs;
    size_t sz;
    size_t sz2;
    size_t i;
    size_t j;

    all_attribs = jxta_hashtable_values_get(folder->secondary_keys);
    sz = jxta_vector_size(all_attribs);
    for (i = 0; i < sz; i++) {
        Jxta_hashtable *vals_hashtbl;
        Jxta_vector *all_values;

        jxta_vector_get_object_at(all_attribs, (Jxta_object **) & vals_hashtbl, i);
        all_values = jxta_hashtable_values_get(vals_hashtbl);
        JXTA_OBJECT_RELEASE(vals_hashtbl);
        sz2 = jxta_vector_size(all_values);
        for (j = 0; j < sz2; j++) {
            Jxta_hashtable *all_primaries;
            jxta_vector_get_object_at(all_values, (Jxta_object **) & all_primaries, j);
            jxta_hashtable_del(all_primaries, primary_key, key_size, NULL);
            JXTA_OBJECT_RELEASE(all_primaries);
        }
        JXTA_OBJECT_RELEASE(all_values);
    }
    JXTA_OBJECT_RELEASE(all_attribs);

    key.dptr = primary_key;
    key.dsize = key_size;

    apr_dbm_delete(folder->data, key);
    apr_dbm_delete(folder->exp, key);

    return JXTA_SUCCESS;
}

static Jxta_status folder_remove_expired_records(Folder * folder)
{
    apr_datum_t key;
    apr_datum_t data;
    Jxta_expiration_time exp[2];
    Jxta_status status;

    apr_dbm_firstkey(folder->exp, &key);
    while (key.dptr != NULL) {
        status = apr_dbm_fetch(folder->exp, key, &data);

        if (data.dptr == NULL)
            status = JXTA_ITEM_NOTFOUND;

        if (status == JXTA_SUCCESS) {
#ifndef WIN32
            sscanf(data.dptr, "%lld %lld", exp, exp + 1);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "expiration time retreived %lld %lld\n", exp[0], exp[1]);
#else
            sscanf(data.dptr, "%I64d %I64d", exp, exp + 1);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "expiration time retreived %I64d %I64d\n", exp[0], exp[1]);
#endif
        }

        if (exp[0] < jpr_time_now()) {
            folder_remove_advertisement(folder, key.dptr, key.dsize);
        }

        apr_dbm_nextkey(folder->exp, &key);
    }

    return JXTA_SUCCESS;
}

/* remove an advertisement given the group_id, type and advertisement name */
Jxta_status jxta_cm_remove_advertisement(Jxta_cm * self, char *folder_name, char *primary_key)
{
    Folder *folder;
    Jxta_status status;

    apr_thread_mutex_lock(self->mutex);

    status = jxta_hashtable_get(self->folders, folder_name, strlen(folder_name), (Jxta_object **) & folder);
    if (status != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        return status;
    }

    folder_remove_advertisement(folder, primary_key, strlen(primary_key));

    JXTA_OBJECT_RELEASE(folder);

    apr_thread_mutex_unlock(self->mutex);
    return JXTA_SUCCESS;
}

/* return a pointer to all advertisements in folder folder_name */
/* a NULL pointer means the end of the list */
char **jxta_cm_get_primary_keys(Jxta_cm * self, char *folder_name)
{
    Folder *folder;
    Jxta_status status;
    size_t nb_keys;
    char **result;
    apr_datum_t key;

    apr_thread_mutex_lock(self->mutex);

    status = jxta_hashtable_get(self->folders, folder_name, strlen(folder_name), (Jxta_object **) & folder);
    if (status != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        return NULL;
    }

    nb_keys = 0;
    apr_dbm_firstkey(folder->data, &key);
    while (key.dptr != NULL) {
        apr_dbm_nextkey(folder->data, &key);
        ++nb_keys;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "NB Keys : %d\n", nb_keys);

    result = (char **) malloc(sizeof(char *) * (nb_keys + 1));

    nb_keys = 0;
    apr_dbm_firstkey(folder->data, &key);
    while (key.dptr != NULL) {
        char *tmp;
        tmp = (char *) malloc(key.dsize + 1);
        memcpy(tmp, key.dptr, key.dsize);
        tmp[key.dsize] = '\0';
        result[nb_keys++] = tmp;
        apr_dbm_nextkey(folder->data, &key);
    }
    result[nb_keys] = NULL;

    JXTA_OBJECT_RELEASE(folder);

    apr_thread_mutex_unlock(self->mutex);

    return result;
}

static void
secondary_indexing(Jxta_cm * self, Folder * folder, Jxta_advertisement * adv, apr_datum_t key, Jxta_expiration_time exp)
{
    const char **secondaries;
    const char **sec_k;
    Jxta_vector *sv = NULL;
    JString *jval = NULL;
    JString *jskey = NULL;
    Jxta_SRDIEntryElement *entry = NULL;
    Jxta_vector *delta_entries = NULL;
    sv = jxta_advertisement_get_indexes(adv);
    if (sv != NULL) {
        jxta_cm_create_adv_indexes(self, folder->name, sv);
        JXTA_OBJECT_RELEASE(sv);
    }
    secondaries = jxta_advertisement_get_tagnames(adv);
    sec_k = secondaries;
    while ((secondaries != NULL) && (*sec_k != 0)) {
        char *val;
        Jxta_hashtable *val_tbl;
        Jxta_hashtable *prim_tbl;
        if (jxta_hashtable_get(folder->secondary_keys, *sec_k, strlen(*sec_k), (Jxta_object **) & val_tbl) != JXTA_SUCCESS) {
            ++sec_k;
            continue;
        }
        val = jxta_advertisement_get_string(adv, *sec_k);

        if (val == NULL) {
            JXTA_OBJECT_RELEASE(val_tbl);
            ++sec_k;
            continue;
        }

        if (jxta_hashtable_get(val_tbl, val, strlen(val), (Jxta_object **) & prim_tbl) != JXTA_SUCCESS) {
            /* first time we see that value, add a list for it */
            prim_tbl = jxta_hashtable_new_0(1, FALSE);
            jxta_hashtable_put(val_tbl, val, strlen(val), (Jxta_object *) prim_tbl);
        }

        /* Publish SRDI delta */
        if (exp > 0 && self->record_delta) {
            jval = jstring_new_2(val);
            jskey = jstring_new_2(*sec_k);
            entry = jxta_srdi_new_element_1(jskey, jval, exp - jpr_time_now());
            if (entry != NULL) {
#ifndef WIN32
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "add SRDI delta entry: SKey:%s Val:%s exp:%lld\n",
                                jstring_get_string(jskey), jstring_get_string(jval), (exp - jpr_time_now()));
#else
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "add SRDI delta entry: SKey:%s Val:%s exp:%I64d\n",
                                jstring_get_string(jskey), jstring_get_string(jval), (exp - jpr_time_now()));
#endif
                if (jxta_hashtable_get
                    (self->srdi_delta, folder->name, strlen(folder->name), (Jxta_object **) & delta_entries) != JXTA_SUCCESS) {
                    /* first time we see that value, add a list for it */
                    delta_entries = jxta_vector_new(0);
                    jxta_hashtable_put(self->srdi_delta, folder->name, strlen(folder->name), (Jxta_object *) delta_entries);
                }
                jxta_vector_add_object_last(delta_entries, (Jxta_object *) entry);
                JXTA_OBJECT_RELEASE(entry);
                entry = NULL;
                JXTA_OBJECT_RELEASE(delta_entries);
            }
            JXTA_OBJECT_RELEASE(jval);
            JXTA_OBJECT_RELEASE(jskey);
        }
        ++sec_k;
        free(val);
        JXTA_OBJECT_RELEASE(val_tbl);
        jxta_hashtable_put(prim_tbl, key.dptr, key.dsize, (Jxta_object *) self->dummy_object);
        JXTA_OBJECT_RELEASE(prim_tbl);
    }
    free(secondaries);
    secondaries = NULL;
}

static void folder_reindex(Jxta_cm * self, Folder * folder)
{
    apr_datum_t key;

    apr_dbm_firstkey(folder->data, &key);
    while (key.dptr != NULL) {
        apr_datum_t data;
        Jxta_vector *content;
        Jxta_advertisement *adv = jxta_advertisement_new();
        char *primary_key;

        primary_key = malloc(key.dsize + 1);
        memcpy(primary_key, key.dptr, key.dsize);
        primary_key[key.dsize] = '\0';


        if ((apr_dbm_fetch(folder->data, key, &data) != JXTA_SUCCESS)
            || (data.dptr == NULL)) {
            free(primary_key);
            apr_dbm_nextkey(folder->data, &key);
            continue;
        }

        jxta_advertisement_parse_charbuffer(adv, data.dptr, data.dsize);
        jxta_advertisement_get_advs(adv, &content);
        JXTA_OBJECT_RELEASE(adv);

        if (content == NULL) {
            free(primary_key);
            apr_dbm_nextkey(folder->data, &key);
            continue;
        }
        if (jxta_vector_size(content) == 0) {
            free(primary_key);
            JXTA_OBJECT_RELEASE(content);
            apr_dbm_nextkey(folder->data, &key);
            continue;
        }

        /* Reuse variable adv for the single contained advertisement */
        jxta_vector_get_object_at(content, (Jxta_object **) & adv, 0);
        JXTA_OBJECT_RELEASE(content);

        secondary_indexing(self, folder, adv, key, 0);

        JXTA_OBJECT_RELEASE(adv);
        apr_dbm_nextkey(folder->data, &key);
        free(primary_key);
    }
}


/* save advertisement */
Jxta_status
jxta_cm_save(Jxta_cm * self, char *folder_name, char *primary_key,
             Jxta_advertisement * adv, Jxta_expiration_time timeOutForMe, Jxta_expiration_time timeOutForOthers)
{
    Folder *folder;
    Jxta_status status;
    Jxta_expiration_time exp[2];
    apr_datum_t key;
    apr_datum_t data;
    JString *xml;
    char buff[256];

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Saving primary key: %s\n", primary_key);
    jxta_advertisement_get_xml(adv, &xml);
    if (xml == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Not saved 1\n");
        return JXTA_FAILED;
    }

    apr_thread_mutex_lock(self->mutex);

    status = jxta_hashtable_get(self->folders, folder_name, strlen(folder_name), (Jxta_object **) & folder);
    if (status != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Not saved 2\n");
        return status;
    }

    key.dptr = primary_key;
    key.dsize = strlen(primary_key);

    status = apr_dbm_fetch(folder->exp, key, &data);

    if (data.dptr == NULL)
        status = JXTA_ITEM_NOTFOUND;

    if (status == JXTA_SUCCESS) {

#ifndef WIN32
        sscanf(data.dptr, "%lld %lld", exp, exp + 1);
#else
        sscanf(data.dptr, "%I64d %I64d", exp, exp + 1);
#endif /* WIN32 */

        if (jpr_time_now() + timeOutForMe > exp[0])
            exp[0] = jpr_time_now() + timeOutForMe;
        exp[1] = timeOutForOthers;

        /*
           apr_dbm_delete(folder->exp, key);
         */
    } else {
        exp[0] = jpr_time_now() + timeOutForMe;
        exp[1] = timeOutForOthers;
    }

    /*
       apr_dbm_delete(folder->data, key);
     */
    data.dptr = (char *) jstring_get_string(xml);
    data.dsize = jstring_length(xml);

    status = apr_dbm_store(folder->data, key, data);
    JXTA_OBJECT_RELEASE(xml);

    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(folder);
        apr_thread_mutex_unlock(self->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Not saved 3 (%d)\n", status);
        return status;
    }
#ifndef WIN32
    sprintf(buff, "%lld %lld", exp[0], exp[1]);
#else
    sprintf(buff, "%I64d %I64d", exp[0], exp[1]);
#endif
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "expiration save time %s\n", buff);
    data.dptr = buff;
    data.dsize = strlen(buff) + 1;

    status = apr_dbm_store(folder->exp, key, data);

    if (status != JXTA_SUCCESS) {
        apr_dbm_delete(folder->data, key);
        JXTA_OBJECT_RELEASE(folder);
        apr_thread_mutex_unlock(self->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Not saved 4\n");
        return status;
    }

    /*
     * Do secondary indexing now.
     */
    secondary_indexing(self, folder, adv, key, exp[0]);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Saved\n");
    JXTA_OBJECT_RELEASE(folder);
    apr_thread_mutex_unlock(self->mutex);
    return JXTA_SUCCESS;
}

/* search for advertisements with specific (attribute,value) pairs */
/* a NULL pointer means the end of the list */

char **jxta_cm_search(Jxta_cm * self, char *folder_name, const char *attribute, const char *value, int n_adv)
{
    Folder *folder;
    Jxta_status status;
    Jxta_hashtable *values_hashtbl;
    Jxta_hashtable *primaries_hashtbl;
    char **all_primaries;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Search folder:%s Attr:%s Val:%s\n", folder_name, attribute, value);
    apr_thread_mutex_lock(self->mutex);

    status = jxta_hashtable_get(self->folders, folder_name, strlen(folder_name), (Jxta_object **) & folder);

    if (status != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        return NULL;
    }

    status = jxta_hashtable_get(folder->secondary_keys, attribute, strlen(attribute), (Jxta_object **) & values_hashtbl);

    JXTA_OBJECT_RELEASE(folder);
    if (status != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        return NULL;
    }
    status = jxta_hashtable_get(values_hashtbl, value, strlen(value), (Jxta_object **) & primaries_hashtbl);

    JXTA_OBJECT_RELEASE(values_hashtbl);

    if (status != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        return NULL;
    }
    all_primaries = jxta_hashtable_keys_get(primaries_hashtbl);
    JXTA_OBJECT_RELEASE(primaries_hashtbl);
    apr_thread_mutex_unlock(self->mutex);
    return all_primaries;
}


/* restore the original advertisement from the cache */
Jxta_status jxta_cm_restore_bytes(Jxta_cm * self, char *folder_name, char *primary_key, JString ** bytes)
{
    Folder *folder;
    Jxta_status status;
    apr_datum_t key;
    apr_datum_t data;

    apr_thread_mutex_lock(self->mutex);

    status = jxta_hashtable_get(self->folders, folder_name, strlen(folder_name), (Jxta_object **) & folder);
    if (status != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        return status;
    }

    key.dptr = primary_key;
    key.dsize = strlen(primary_key);
    status = apr_dbm_fetch(folder->data, key, &data);
    JXTA_OBJECT_RELEASE(folder);

    if (data.dptr == NULL)
        status = JXTA_ITEM_NOTFOUND;

    if (status == JXTA_SUCCESS) {
        *bytes = jstring_new_1(data.dsize);
        jstring_append_0(*bytes, data.dptr, data.dsize);
    }

    apr_thread_mutex_unlock(self->mutex);
    return status;
}

/* Get the expiration time of an advertisement */
Jxta_status jxta_cm_get_expiration_time(Jxta_cm * self, char *folder_name, char *primary_key, Jxta_expiration_time * time)
{
    Folder *folder;
    Jxta_status status;
    apr_datum_t data;
    apr_datum_t key;
    Jxta_expiration_time exp[2];

    apr_thread_mutex_lock(self->mutex);

    status = jxta_hashtable_get(self->folders, folder_name, strlen(folder_name), (Jxta_object **) & folder);
    if (status != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        return status;
    }

    key.dptr = primary_key;
    key.dsize = strlen(primary_key);

    /*
     * Note: the db is supposed to own the buffer that it returns. we have
     * to copy it and must not free it.
     */

    status = apr_dbm_fetch(folder->exp, key, &data);

    JXTA_OBJECT_RELEASE(folder);

    apr_thread_mutex_unlock(self->mutex);

    if (data.dptr == NULL)
        status = JXTA_ITEM_NOTFOUND;

    if (status == JXTA_SUCCESS) {

#ifndef WIN32
        sscanf(data.dptr, "%lld %lld", &exp[0], &exp[1]);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "expiration time retreived %lld %lld\n", exp[0], exp[1]);
#else
        sscanf(data.dptr, "%I64d %I64d", &exp[0], &exp[1]);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "expiration time retreived %I64d %I64d\n", exp[0], exp[1]);
#endif
        *time = exp[0];
    }

    return status;
}

/* delete files where the expiration time is less than curr_expiration */
Jxta_status jxta_cm_remove_expired_records(Jxta_cm * self)
{
    Jxta_vector *all_folders;
    int cnt, i;
    Folder *folder;
    Jxta_status rt, latched_rt = JXTA_SUCCESS;

    apr_thread_mutex_lock(self->mutex);

    all_folders = jxta_hashtable_values_get(self->folders);
    if (NULL == all_folders) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to retrieve folders");
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_NOMEM;
    }

    cnt = jxta_vector_size(all_folders);
    for (i = 0; i < cnt; i++) {
        jxta_vector_get_object_at(all_folders, (Jxta_object **) & folder, i);
        rt = folder_remove_expired_records(folder);
        if (JXTA_SUCCESS != rt) {
            latched_rt = rt;
            /* return rt */ ;
        }
        JXTA_OBJECT_RELEASE(folder);
    }
    JXTA_OBJECT_RELEASE(all_folders);

    apr_thread_mutex_unlock(self->mutex);
    return latched_rt;
}

/* create a folder with type <type> and index keywords <keys> */
Jxta_status jxta_cm_create_folder(Jxta_cm * self, char *folder_name, const char *keys[])
{
/*    const char **p; */
    Jxta_status status;
    char *full_name;
    Folder *folder;
    Jxta_vector *iv = NULL;

    apr_thread_mutex_lock(self->mutex);

    if (jxta_hashtable_get(self->folders, folder_name, strlen(folder_name), (Jxta_object **) & folder) == JXTA_SUCCESS) {

        JXTA_OBJECT_RELEASE(folder);
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_SUCCESS;
    }

    folder = malloc(sizeof(Folder));
    memset(folder, 0, sizeof(*folder));
    JXTA_OBJECT_INIT(folder, folder_free, 0);

    full_name = malloc(strlen(self->root) + strlen(folder_name) + 6);

    if (full_name == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Out of memory");
        return JXTA_NOMEM;
    }

    sprintf(full_name, "%s_%s", self->root, folder_name);
    status =
        apr_dbm_open_ex(&(folder->data), DBTYPE, full_name,
                        APR_DBM_RWCREATE, APR_UREAD | APR_UWRITE | APR_GREAD | APR_WREAD, self->pool);

    if (status != JXTA_SUCCESS) {
        status =
            apr_dbm_open_ex(&(folder->data), DBTYPE, full_name,
                            APR_DBM_RWTRUNC, APR_UREAD | APR_UWRITE | APR_GREAD | APR_WREAD, self->pool);
    }

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to open folder: %s with error %ld\n", full_name, status);
        return status;
    }

    strcat(full_name, "_exp");
    status =
        apr_dbm_open_ex(&(folder->exp), DBTYPE, full_name,
                        APR_DBM_RWCREATE, APR_UREAD | APR_UWRITE | APR_GREAD | APR_WREAD, self->pool);
    if (status != JXTA_SUCCESS) {
        status =
            apr_dbm_open_ex(&(folder->exp), DBTYPE, full_name,
                            APR_DBM_RWTRUNC, APR_UREAD | APR_UWRITE | APR_GREAD | APR_WREAD, self->pool);
    }

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to open folder: %s with error %ld\n", full_name, status);
        return status;
    }

    free(full_name);

    folder->name = folder_name;

    jxta_hashtable_put(self->folders, folder_name, strlen(folder_name), (Jxta_object *) folder);

    if (keys != NULL) {
        iv = jxta_advertisement_return_indexes(keys);
        jxta_cm_create_adv_indexes(self, folder_name, iv);
        JXTA_OBJECT_RELEASE(iv);
    }

    folder_reindex(self, folder);

    JXTA_OBJECT_RELEASE(folder);


    apr_thread_mutex_unlock(self->mutex);


    return JXTA_SUCCESS;
}

Jxta_status jxta_cm_create_adv_indexes(Jxta_cm * self, char *folder_name, Jxta_vector * jv)
{
    int i = 0;
    Folder *folder = NULL;
    Jxta_status status = 0;

    if (jxta_hashtable_get(self->folders, folder_name, strlen(folder_name), (Jxta_object **) & folder) != JXTA_SUCCESS) {

        JXTA_OBJECT_RELEASE(folder);
        return !JXTA_SUCCESS;
    }

    if (folder->secondary_keys == NULL) {
        folder->secondary_keys = jxta_hashtable_new_0(4, FALSE);
    }

    for (; i < jxta_vector_size(jv); i++) {
        Jxta_index *ji;
        char *attributeName;
        char *full_index_name;
        status = jxta_vector_get_object_at(jv, (Jxta_object **) & ji, i);
        if (status == JXTA_SUCCESS) {
            if (ji->element != NULL) {
                char *element = (char *) jstring_get_string(ji->element);
                Jxta_hashtable *attrib = NULL;
                if (ji->attribute != NULL) {
                    attributeName = (char *) jstring_get_string(ji->attribute);
                    full_index_name = malloc(strlen(attributeName) + strlen(element) + 6);
                    sprintf(full_index_name, "%s_%s", element, attributeName);
                } else {
                    full_index_name = malloc(strlen(element) + 6);
                    sprintf(full_index_name, "%s", element);
                }
                attrib = jxta_hashtable_new_0(1, FALSE);
                jxta_hashtable_putnoreplace(folder->secondary_keys,
                                            full_index_name, strlen(full_index_name), (Jxta_object *) attrib);
                JXTA_OBJECT_RELEASE(attrib);

                free(full_index_name);
            }
        }
        JXTA_OBJECT_RELEASE((Jxta_object *) ji);
    }
    return JXTA_SUCCESS;
}

/*
 * hash a document
 */
unsigned long jxta_cm_hash(const char *key, size_t ksz)
{
    const unsigned char *s = (const unsigned char *) key;
    unsigned long hash = 0;
    while (ksz--) {
        hash += *s++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    /* hash_key zero is invalid. it remaps to 1 */
    return hash ? hash : 1;
}

/* temporary workaround for closing the database.
 * The cm will be freed automatically when group stoping realy works
 */





void jxta_cm_close(Jxta_cm * self)
{
    apr_thread_mutex_lock(self->mutex);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "cm_close\n");
    JXTA_OBJECT_RELEASE(self->folders);
    self->folders = NULL;
    apr_thread_mutex_unlock(self->mutex);
}

/*
 * obtain all CM SRDI delta entries for a particular folder
 */
Jxta_vector *jxta_cm_get_srdi_delta_entries(Jxta_cm * self, JString * folder_name)
{

    Jxta_vector *delta_entries = NULL;

    apr_thread_mutex_lock(self->mutex);
    if (jxta_hashtable_del
        (self->srdi_delta, jstring_get_string(folder_name),
         strlen(jstring_get_string(folder_name)), (Jxta_object **) & delta_entries) != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        return NULL;
    }

    apr_thread_mutex_unlock(self->mutex);
    return delta_entries;
}

/*
 * obtain all CM SRDI entries for a particular folder
 */
Jxta_vector *jxta_cm_get_srdi_entries(Jxta_cm * self, JString * folder_name)
{

    Jxta_status status;
    Folder *folder;
    char **keys = NULL;
    JString *skey = NULL;
    JString *sval = NULL;
    Jxta_expiration_time exp;
    Jxta_SRDIEntryElement *entry = NULL;
    char **values = NULL;
    char **pkeys = NULL;
    int i = 0;
    int j = 0;
    int k = 0;
    Jxta_hashtable *hash_val;
    Jxta_hashtable *values_hashtbl = NULL;
    Jxta_vector *entries = jxta_vector_new(0);

    apr_thread_mutex_lock(self->mutex);
    self->record_delta = TRUE;
    status = jxta_hashtable_get(self->folders,
                                jstring_get_string(folder_name),
                                strlen(jstring_get_string(folder_name)), (Jxta_object **) & folder);

    if (status != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        JXTA_OBJECT_RELEASE(entries);
        return NULL;
    }

    keys = jxta_hashtable_keys_get(folder->secondary_keys);
    if (keys == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "No CM SRDI items found for type: %s\n", jstring_get_string(folder_name));
        JXTA_OBJECT_RELEASE(folder);
        apr_thread_mutex_unlock(self->mutex);
        JXTA_OBJECT_RELEASE(entries);
        return NULL;
    }

    while (keys[i]) {
        status = jxta_hashtable_get(folder->secondary_keys, keys[i], strlen(keys[i]), (Jxta_object **) & values_hashtbl);

        if (status != JXTA_SUCCESS) {
            JXTA_OBJECT_RELEASE(folder);
            free(keys);
            apr_thread_mutex_unlock(self->mutex);
            JXTA_OBJECT_RELEASE(entries);
            return NULL;
        }

        values = jxta_hashtable_keys_get(values_hashtbl);
        j = 0;
        while (values[j]) {

            /*
             * get the expiration using the unique UUID portion
             */
            jxta_hashtable_get(values_hashtbl, values[j], strlen(values[j]), (Jxta_object **) & hash_val);

            pkeys = jxta_hashtable_keys_get(hash_val);
            JXTA_OBJECT_RELEASE(hash_val);

            /*
             * FIXME:20040614 tra
             * we pick up the first entry for this value
             * we may want to take the MAX or MIN of all the values
             */
            if (pkeys[0] != NULL) {
                status = jxta_cm_get_expiration_time(self, (char *)
                                                     jstring_get_string(folder_name), pkeys[0], &exp);

                if (pkeys != NULL) {
                    k = 0;
                    while (pkeys[k]) {
                        free(pkeys[k]);
                        k++;
                    }
                    free(pkeys);
                    pkeys = NULL;
                }

                if (status != JXTA_SUCCESS) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Could not find expiration for %s\n", values[j]);
                    j++;
                    continue;
                }
            }

            if (exp - jpr_time_now() < 0) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Expired entry\n");
                j++;
                continue;
            }

            skey = jstring_new_2(keys[i]);
            sval = jstring_new_2(values[j]);
            entry = jxta_srdi_new_element_1(skey, sval, exp - jpr_time_now());

            if (entry != NULL) {
#ifndef WIN32
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "add SRDI entry: SKey:%s Val:%s exp:%lld\n",
                                jstring_get_string(skey), jstring_get_string(sval), (exp - jpr_time_now()));
#else
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "add SRDI entry: SKey:%s Val:%s exp:%I64d\n",
                                jstring_get_string(skey), jstring_get_string(sval), (exp - jpr_time_now()));
#endif
                jxta_vector_add_object_last(entries, (Jxta_object *) entry);
                JXTA_OBJECT_RELEASE(entry);
            }
            JXTA_OBJECT_RELEASE(skey);
            JXTA_OBJECT_RELEASE(sval);
            j++;
        }

        k = 0;
        while (values[k]) {
            free(values[k]);
            k++;
        }
        free(values);
        values = NULL;
        JXTA_OBJECT_RELEASE(values_hashtbl);
        i++;
    }

    apr_thread_mutex_unlock(self->mutex);
    k = 0;
    while (keys[k]) {
        free(keys[k]);
        k++;
    }
    free(keys);
    keys = NULL;
    JXTA_OBJECT_RELEASE(folder);

    if (jxta_vector_size(entries) > 0) {
        return entries;
    } else {
        JXTA_OBJECT_RELEASE(entries);
        return NULL;
    }
}


/* vim: set sw=4 ts=4 et tw=130: */
