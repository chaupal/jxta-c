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
 * $Id: jxta_cm.c,v 1.110 2005/12/15 23:26:59 slowhog Exp $
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <apu.h>
#include <apr_pools.h>
#include <apr_dbd.h>

#ifdef WIN32
#include <direct.h> /* for _mkdir */
#else
#include "config.h"
#endif

/* #include <apr_dbm.h> */
#include <sqlite3.h>
#include "jxta_apr.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_object.h"
#include "jxta_hashtable.h"
#include "jxta_advertisement.h"
#include "jxta_discovery_service.h"
#include "jstring.h"
#include "jxta_id.h"
#include "jxta_cm.h"
#include "jxta_srdi.h"
#include "jxta_log.h"
#include "jxta_sql.h"
#include "jxta_proffer.h"
#include "jxta_range.h"

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
    Dummy_object *res = (Dummy_object *) calloc(1, sizeof(Dummy_object));

    JXTA_OBJECT_INIT(res, dummy_object_free, 0);
    return res;
}

typedef struct {
    JXTA_OBJECT_HANDLE;
    Jxta_hashtable *secondary_keys;
    char *name;
} Folder;

/* 
 * This is a folder definition that is required for the SQL backend
 * using the apr_dbd.h generic SQL database API.
*/
typedef struct {
    JXTA_OBJECT_HANDLE;
    const char *name;
    const char *id;
    Jxta_hashtable *secondary_keys;
    apr_pool_t *pool;
    apr_dbd_t *sql;
    const apr_dbd_driver_t *driver;
    int rv;
} GroupDB;


struct _jxta_cm {
    JXTA_OBJECT_HANDLE;
    char *root;
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
    Jxta_hashtable *folders;
    Dummy_object *dummy_object;
    Jxta_hashtable *srdi_delta;
    Jxta_boolean record_delta;
    Jxta_PID *localPeerId;
    GroupDB *groupDB;
};

/*
 * a sql column value
*/

typedef struct {
    JXTA_OBJECT_HANDLE;
    char *value;
    char *name;
} Jxta_sql_col;

/*
 * result set of a sql query
*/

typedef struct {
    JXTA_OBJECT_HANDLE;
    Jxta_sql_col **columns;
} Jxta_sql_row;

static JString *jPeerNs;
static JString *jGroupNs;
static JString *jInt64_for_format;
static JString *fmtTime;

static Jxta_boolean dbd_initialized = FALSE;
static apr_pool_t *apr_dbd_init_pool = NULL;

/*
 * table used for secondary indexing of advertisements
*/
static const char *jxta_elemTable = CM_TBL_ELEM_ATTRIBUTES;
static const char *jxta_elemTable_fields[][2] = {
    {CM_COL_NameSpace, SQL_VARCHAR_128}, 
    {CM_COL_AdvId, SQL_VARCHAR_128},
    {CM_COL_Name, SQL_VARCHAR_64},
    {CM_COL_Value, SQL_VARCHAR_4K},
    {CM_COL_NumValue, SQL_REAL},
    {CM_COL_NumRange, SQL_VARCHAR_64},
    {CM_COL_TimeOut, SQL_VARCHAR_64},
    {CM_COL_TimeOutForOthers, SQL_VARCHAR_64},
    {NULL, NULL}
};

/*
 * table used by the SRDI for distributed hash table
*/
static const char *jxta_srdiTable = CM_TBL_SRDI;
static const char *jxta_srdiTable_fields[][2] = {
    {CM_COL_NameSpace, SQL_VARCHAR_128},
    {CM_COL_Handler, SQL_VARCHAR_64},
    {CM_COL_Peerid, SQL_VARCHAR_128},
    {CM_COL_AdvId, SQL_VARCHAR_128},
    {CM_COL_Name, SQL_VARCHAR_64},
    {CM_COL_Value, SQL_VARCHAR_4K},
    {CM_COL_NumValue, SQL_REAL},
    {CM_COL_NumRange, SQL_VARCHAR_64},
    {CM_COL_TimeOut, SQL_VARCHAR_64},
    {CM_COL_TimeOutForOthers, SQL_VARCHAR_64},
    {NULL, NULL}
};

/*
 * table used by the SRDI for replica table entries
*/
static const char *jxta_replicaTable = CM_TBL_REPLICA;
static const char *jxta_replicaTable_fields[][2] = {
    {CM_COL_NameSpace, SQL_VARCHAR_128},
    {CM_COL_Handler, SQL_VARCHAR_64},
    {CM_COL_Peerid, SQL_VARCHAR_128},
    {CM_COL_AdvId, SQL_VARCHAR_128},
    {CM_COL_Name, SQL_VARCHAR_64},
    {CM_COL_Value, SQL_VARCHAR_4K},
    {CM_COL_NumValue, SQL_REAL},
    {CM_COL_NumRange, SQL_VARCHAR_64},
    {CM_COL_TimeOut, SQL_VARCHAR_64},
    {CM_COL_TimeOutForOthers, SQL_VARCHAR_64},
    {NULL, NULL}
};

/*
 * table for all advertisements
*/
static const char *jxta_advTable = CM_TBL_ADVERTISEMENTS;
static const char *jxta_advTable_fields[][2] = {
    {CM_COL_NameSpace, SQL_VARCHAR_128},
    {CM_COL_AdvType, SQL_VARCHAR_128},
    {CM_COL_Advert, SQL_VARCHAR_16K},
    {CM_COL_AdvId, SQL_VARCHAR_128},
    {CM_COL_TimeOut, SQL_VARCHAR_64},
    {CM_COL_TimeOutForOthers, SQL_VARCHAR_64},
    {NULL, NULL}
};

static apr_status_t init_group_dbd(GroupDB * group);

static apr_status_t cm_sql_create_table(GroupDB * groupDB, const char *table, const char *columns[]);

static int cm_sql_create_index(GroupDB * groupDB, const char *table, const char **fields);

static int cm_sql_drop_table(GroupDB * groupDB, char *table);

static Jxta_status
cm_sql_save_advertisement(GroupDB * groupDB, const char *key, Jxta_advertisement * adv, Jxta_expiration_time timeOutForMe,
                          Jxta_expiration_time timeOutForOthers);

static Jxta_status
cm_sql_save_srdi(GroupDB * groupDB, JString * Handler, JString * Peerid, JString * NameSpace, JString * Key, JString * Attribute,
                 JString * Value, JString * Range, Jxta_expiration_time timeOutForMe, Jxta_boolean replica);

static Jxta_status cm_sql_remove_advertisement(GroupDB * groupDB, const char *nameSpace, const char *key);

static Jxta_status
cm_sql_update_item(GroupDB * groupDB, const char *table, JString * jNameSpace, JString * jKey, JString * jElemAttr,
                   JString * jVal, JString * jRange, Jxta_expiration_time timeOutForMe);

static Jxta_status
cm_sql_update_srdi(GroupDB * groupDB, const char *table, JString * jHandler, JString * jPeerid, JString * jKey,
                   JString * jAttribute, JString * jValue, JString * jRange, Jxta_expiration_time timeOutForMe);

static Jxta_status
cm_sql_select(GroupDB * groupDB, apr_pool_t * pool, const char *table, apr_dbd_results_t ** res, JString * columns,
              JString * where);

static Jxta_status cm_sql_select_join(GroupDB * groupDB, apr_pool_t * pool, apr_dbd_results_t ** res, JString * where);

static Jxta_status
cm_sql_insert_item(GroupDB * groupDB, const char *table, const char *nameSpace, const char *key, const char *elemAttr,
                   const char *val, const char *range, Jxta_expiration_time timeOutForMe, Jxta_expiration_time timeOutForOthers);

static Jxta_status cm_sql_delete_item(GroupDB * groupDB, const char *table, const char *advId);

static Jxta_status cm_sql_delete_with_where(GroupDB * groupDB, const char *table, JString * where);

static Jxta_status cm_sql_remove_expired_records(GroupDB * groupDB, const char *folder_name);

static Jxta_status
cm_sql_found(GroupDB * groupDB, const char *table, const char *nameSpace, const char *advId, const char *elemAttr,
             Jxta_expiration_time * exp);

static Jxta_status cm_sql_table_found(GroupDB * groupDB, const char *table);

static Jxta_status cm_sql_index_found(GroupDB * groupDB, const char *index);

static char **cm_sql_get_primary_keys(Jxta_cm * self, char *folder_name, const char *tableName, JString * jWhere,
                                      JString * jGroup, int row_max);

static Jxta_status
cm_sql_srdi_found(GroupDB * groupDB, const char *table, JString * NameSpace, JString * Handler, JString * Peerid, JString * Key,
                  JString * Attribute);

static void cm_sql_correct_space(const char *folder_name, JString * where);

static void cm_sql_append_timeout_check_greater(JString * cmd);

static void cm_sql_append_timeout_check_less(JString * cmd);

static void cm_sql_pragma(GroupDB * groupDB, const char *pragma);

JXTA_DECLARE(Jxta_boolean) jxta_sql_escape_and_wc_value(JString * jStr, Jxta_boolean replace);

/* Destroy the cm and free all allocated memory */
static void cm_free(Jxta_object * cm)
{
    Jxta_cm *self = (Jxta_cm *) cm;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "CM free\n");

    apr_thread_mutex_lock(self->mutex);
    if (self->folders) {
        JXTA_OBJECT_RELEASE(self->folders);
    }
    JXTA_OBJECT_RELEASE(self->srdi_delta);
    JXTA_OBJECT_RELEASE(self->dummy_object);
    free(self->root);

    if (self->groupDB != NULL) {
        JXTA_OBJECT_RELEASE(self->groupDB);
    }
    apr_thread_mutex_unlock(self->mutex);
    apr_thread_mutex_destroy(self->mutex);
    apr_pool_destroy(self->pool);

    /* Free the object itself */
    free(self);
}

static void groupDB_free(Jxta_object * obj)
{
    GroupDB *gdb = (GroupDB *) obj;

    if (gdb->secondary_keys != NULL)
        JXTA_OBJECT_RELEASE(gdb->secondary_keys);
    apr_dbd_close(gdb->driver, gdb->sql);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s is closed\n", gdb->name);
    
    if (gdb->name != NULL)
        free((void *) gdb->name);
    if (gdb->id != NULL)
        free((void *) gdb->id);
    if (gdb->secondary_keys != NULL)
        JXTA_OBJECT_RELEASE(gdb->secondary_keys);
    apr_pool_destroy(gdb->pool);
    free(gdb);
}

/* Create a new Cm Structure */
JXTA_DECLARE(Jxta_cm *) jxta_cm_new(const char *home_directory, Jxta_id * group_id, Jxta_PID * localPeerId)
{
    GroupDB *groupDB;
    const char *dbExt = ".sqdb";
    apr_status_t res;
    JString *group_id_s = NULL;
    Jxta_cm *self ;
    int root_len;
    int name_len;

    self = (Jxta_cm *) calloc(1, sizeof(Jxta_cm));

    if (jPeerNs == NULL) {
        jPeerNs = jstring_new_2("jxta:PA");
        jGroupNs = jstring_new_2("jxta:PGA");

        /* this is for formatted strings */
        jInt64_for_format = jstring_new_2("%");
        fmtTime = jstring_new_2("exp Time ");
        jstring_append_2(jInt64_for_format, APR_INT64_T_FMT);
        jstring_append_1(fmtTime, jInt64_for_format);
        jstring_append_2(fmtTime, "\n");
    }

    JXTA_OBJECT_INIT(self, cm_free, 0);
    jxta_id_get_uniqueportion(group_id, &group_id_s);

    /* If there is an apr api for this, maybe we should use it */
#ifdef WIN32
    _mkdir(home_directory);
#else
    mkdir(home_directory, 0755);
#endif

    root_len = strlen(home_directory) + jstring_length(group_id_s) + 2;
    self->root = calloc(1, root_len);

#ifdef WIN32
    apr_snprintf(self->root, root_len, "%s\\%s", home_directory, (char *) jstring_get_string(group_id_s));
#else
    apr_snprintf(self->root, root_len, "%s/%s", home_directory, (char *) jstring_get_string(group_id_s));
#endif

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "CM home_directory: %s\n", self->root);

    self->folders = jxta_hashtable_new_0(3, FALSE);
    self->srdi_delta = jxta_hashtable_new_0(3, TRUE);
    self->record_delta = TRUE;
    self->localPeerId = localPeerId;
    res = apr_pool_create(&self->pool, NULL);

    if (res != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create apr_pool: %d\n", res);
        /* Free the memory that has been already allocated */
        free(self->root);
        free(self);
        return NULL;
    }

    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,    /* nested */
                                  self->pool);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(self->pool);
        free(self->root);
        free(self);
        return NULL;
    }

    self->dummy_object = dummy_object_new();

    /* create a db structure for the group */
    groupDB = calloc(1, sizeof(GroupDB));

    JXTA_OBJECT_INIT(groupDB, groupDB_free, 0);

    name_len = strlen(self->root) + strlen(dbExt) + 2;
    groupDB->name = calloc(1, name_len);
    apr_snprintf((void *) groupDB->name, name_len, "%s%s", self->root, dbExt);
    groupDB->id = calloc(1, jstring_length(group_id_s) + 1);
    apr_snprintf((void *) groupDB->id, jstring_length(group_id_s) + 1, "%s", jstring_get_string(group_id_s));
    res = apr_pool_create(&groupDB->pool, NULL);

    if (res != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create apr_pool: %d\n", res);
        apr_pool_destroy((void *) self->pool);
        free((void *) groupDB->name);
        free(self->root);
        free(self);
        return NULL;
    }

    /* each group has it's own database */
    res = init_group_dbd(groupDB);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s is initialized\n", groupDB->name);

    if (res != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to open dbd database: %s with error %ld\n", groupDB->name, res);
        JXTA_OBJECT_RELEASE(self);
        self = NULL;
    } else {
        self->groupDB = groupDB;
    }
    JXTA_OBJECT_RELEASE(group_id_s);
    return self;
}

/*
 * initialize a SQL database and create the tables and indexes
*/
static apr_status_t init_group_dbd(GroupDB * group)
{
    apr_status_t rv;
    Jxta_status status = JXTA_SUCCESS;
    const char *driver = "sqlite3";
    const char *nsIndex[] = { CM_COL_NameSpace, CM_COL_AdvId, NULL };

    if (!dbd_initialized) {
        rv = apr_pool_create(&apr_dbd_init_pool, NULL);

        if (rv != APR_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, 
                            "Unable to create apr_pool initializing the dbd interface: %d\n", rv);
            return rv;
        }
        apr_dbd_init(apr_dbd_init_pool);
        dbd_initialized = TRUE;
    }
    rv = apr_dbd_get_driver(group->pool, driver, &group->driver);
    status = rv;

    switch (rv) {
    case APR_SUCCESS:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Loaded %s driver OK.\n", driver);
        break;
    case APR_EDSOOPEN:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to load driver file apr_dbd_%s.so\n", driver);
        goto finish;
    case APR_ESYMNOTFOUND:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to load driver apr_dbd_%s_driver.\n", driver);
        goto finish;
    case APR_ENOTIMPL:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "No driver available for %s.\n", driver);
        goto finish;
    default:   /* it's a bug if none of the above happen */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Internal error loading %s.\n", driver);
        goto finish;
    }
    rv = apr_dbd_open(group->driver, group->pool, group->name, &group->sql);
/*    rv = apr_dbd_open(group->driver, group->pool, ":memory:",  &group->sql); */

    switch (rv) {
    case APR_SUCCESS:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Opened %s[%s] OK\n", driver, group->name);
        break;
    case APR_EGENERAL:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to open %s[%s]\n", driver, group->name);
        goto finish;
    default:   /* it's a bug if none of the above happen */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Internal error opening %s[%s]\n", driver, group->name);
        goto finish;
    }

    /** create tables for the advertisements, indexes and srdi 
    * 
    **/
    rv = cm_sql_create_table(group, jxta_elemTable, jxta_elemTable_fields[0]);
    if (rv != APR_SUCCESS)
        goto finish;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Element table created \n", group->name);

    rv = cm_sql_create_table(group, jxta_advTable, jxta_advTable_fields[0]);
    if (rv != APR_SUCCESS)
        goto finish;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Advertisement table created\n", group->name);
    
    rv = cm_sql_create_table(group, jxta_srdiTable, jxta_srdiTable_fields[0]);
    if (rv != APR_SUCCESS)
        goto finish;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- SRDI table created\n", group->name);
    
    cm_sql_delete_with_where(group, jxta_srdiTable, NULL);
    
    rv = cm_sql_create_table(group, jxta_replicaTable, jxta_replicaTable_fields[0]);
    if (rv != APR_SUCCESS)
        goto finish;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Replica table created\n", group->name);

    cm_sql_delete_with_where(group, jxta_replicaTable, NULL);

    cm_sql_pragma(group, " synchronous = NORMAL ");
    /* create indexes */

    rv = cm_sql_create_index(group, jxta_elemTable, &nsIndex[0]);
    if (rv != APR_SUCCESS)
        return rv;

  finish:
    if (rv != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- error creating tables db return %i\n", group->name, rv);
    }
    return rv;
}

static void folder_free(Jxta_object * f)
{
    Folder *folder = (Folder *) f;

    JXTA_OBJECT_RELEASE(folder->secondary_keys);
    free(folder->name);
    free(folder);
}

static Jxta_status folder_remove_expired_records(Jxta_cm * self, Folder * folder)
{
    Jxta_status status;
    GroupDB *groupDB = self->groupDB;

    status = cm_sql_remove_expired_records(groupDB, folder->name);

    return status;
}

/* remove an advertisement given the group_id, type and advertisement name */
JXTA_DECLARE(Jxta_status) jxta_cm_remove_advertisement(Jxta_cm * self, const char *folder_name, char *primary_key)
{
    Folder *folder;
    Jxta_status status;
    GroupDB *groupDB = self->groupDB;
    JString *where = jstring_new_0();
    JString *jKey = jstring_new_2(primary_key);

    apr_thread_mutex_lock(self->mutex);

    status = jxta_hashtable_get(self->folders, folder_name, (size_t) strlen(folder_name), JXTA_OBJECT_PPTR(&folder));

    if (status != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        JXTA_OBJECT_RELEASE(where);
        JXTA_OBJECT_RELEASE(jKey);
        return status;
    }

    apr_thread_mutex_unlock(self->mutex);
    jstring_append_2(where, SQL_WHERE);
    jstring_append_2(where, CM_COL_AdvId);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jKey);

    jstring_append_2(where, SQL_AND);
    cm_sql_correct_space(folder_name, where);

    /* remove the advertisement and all it's indexes */

    cm_sql_delete_with_where(groupDB, CM_TBL_ADVERTISEMENTS, where);

    cm_sql_delete_with_where(groupDB, CM_TBL_ELEM_ATTRIBUTES, where);

    cm_sql_delete_with_where(groupDB, CM_TBL_SRDI, where);

    JXTA_OBJECT_RELEASE(folder);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jKey);
    return JXTA_SUCCESS;
}

/* return a pointer to all advertisements in folder folder_name */
/* a NULL pointer means the end of the list */
JXTA_DECLARE(char **) jxta_cm_get_primary_keys(Jxta_cm * self, char *folder_name)
{
    return cm_sql_get_primary_keys(self, folder_name, CM_TBL_ADVERTISEMENTS, NULL, NULL, -1);
}

static char **cm_sql_get_primary_keys(Jxta_cm * self, char *folder_name, const char *tableName, JString * jWhere,
                                      JString * jGroup, int row_max)
{
    size_t nb_keys;
    char **result = NULL;
    GroupDB *groupDB = self->groupDB;
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row;
    const char *entry;
    int rv = 0;
    int n, i;

    JString *where = jstring_new_0();
    JString *columns = jstring_new_0();

    jstring_append_2(columns, CM_COL_AdvId);

    /**
    *  not checking the Peers and Groups is legacy 
    *  Since the current implementation groups queries 
    *  by groups, peers and advertisements the newer convention
    *  of creating name spaces for each type of advertisement 
    *  isn't implemented yet in the older publishing and queries
    **/
    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create apr_pool: %d\n", aprs);
        goto finish;
    }
    cm_sql_correct_space(folder_name, where);

    if (jWhere != NULL) {
        jstring_append_2(where, SQL_AND);
        jstring_append_1(where, jWhere);
    }

    jstring_append_2(where, SQL_AND);
    cm_sql_append_timeout_check_greater(where);

    if (NULL != jGroup) {
        jstring_append_1(where, jGroup);
    }
    rv = cm_sql_select(groupDB, pool, tableName, &res, columns, where);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s jxta_cm_get_primary_keys Select failed: %s %i \n %s   %s\n",
                        groupDB->id, apr_dbd_error(groupDB->driver, groupDB->sql, rv), rv, jstring_get_string(columns)
                        , jstring_get_string(where));
        goto finish;
    }
    nb_keys = apr_dbd_num_tuples((apr_dbd_driver_t *) groupDB->driver, res);
    if (nb_keys == 0)
        goto finish;
    result = (char **) malloc(sizeof(char *) * (nb_keys + 1));
    i = 0;
    for (rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1);
         rv == 0 && (i < row_max || row_max == -1); i++, rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1)) {
        int column_count = apr_dbd_num_cols(groupDB->driver, res);
        for (n = 0; n < column_count; ++n) {
            entry = apr_dbd_get_entry(groupDB->driver, row, n);
            if (entry == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- (null) in entry from get_primary_keys", groupDB->id);
            } else {
                char *tmp;
                int len = strlen(entry);
                tmp = malloc(len + 1);
                memcpy(tmp, entry, len + 1);
                result[i] = tmp;
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s -- key: %s \n", groupDB->id, result[i]);
            }
        }
    }

    result[i] = NULL;

  finish:
    if (NULL != pool)
        apr_pool_destroy(pool);

    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(columns);
    return result;
}

static void
secondary_indexing(Jxta_cm * self, Folder * folder, Jxta_advertisement * adv, const char *key, Jxta_expiration_time exp,
                   Jxta_expiration_time exp_others)
{
    unsigned int i = 0;
    Jxta_status status = JXTA_SUCCESS;
    JString *jns = jstring_new_0();
    JString *jval = NULL;
    JString *jskey = NULL;
    JString *jrange = NULL;
    JString *jPkey = jstring_new_2(key);
    GroupDB *groupDB = self->groupDB;
    char *full_index_name;
    const char *documentName ;
    Jxta_SRDIEntryElement *entry = NULL;
    Jxta_vector *delta_entries = NULL;
    Jxta_vector *sv = NULL;

    documentName = jxta_advertisement_get_document_name(adv);
    jstring_append_2(jns, documentName);

    sv = jxta_advertisement_get_indexes(adv);

    for (i = 0; sv != NULL && i < jxta_vector_size(sv); i++) {
        char *val = NULL;
        const char *range = NULL;
        Jxta_index *ji = NULL;
        jrange = NULL;

        status = jxta_vector_get_object_at(sv, JXTA_OBJECT_PPTR(&ji), i);
        if (status == JXTA_SUCCESS) {
            char *element = NULL;
            char *attrib = NULL;
            int len = 0;
            Jxta_range *rge = NULL;

            element = (char *) jstring_get_string(ji->element);
            if (ji->attribute != NULL) {
                attrib = (char *) jstring_get_string(ji->attribute);
                len = strlen(attrib) + strlen(element) + 2;
                full_index_name = calloc(1, len);
                apr_snprintf(full_index_name, len, "%s %s", element, attrib);
            } else {
                len = strlen(element) + 1;
                full_index_name = calloc(1, len);
                apr_snprintf(full_index_name, len, "%s", element);
            }

            if (ji->range) {
                char *rgeString;

                rge = (Jxta_range *) ji->range;
                rgeString = jxta_range_get_range_string(rge);
                if (NULL != rgeString) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "cm ------- Index --- %s   range: %s \n", 
                                    full_index_name, rgeString);
                    jrange = jstring_new_2(rgeString);
                    range = jstring_get_string(jrange);
                    free(rgeString);
                }
            }
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "secondary_indexing return from get vector %i\n", (int) status);
            continue;
        }

        val = jxta_advertisement_get_string(adv, ji);
        if (val == NULL) {
            if (jrange)
                JXTA_OBJECT_RELEASE(jrange);
            JXTA_OBJECT_RELEASE(ji);
            free(full_index_name);
            continue;
        }

        jval = jstring_new_2(val);
        jskey = jstring_new_2(full_index_name);
        JXTA_OBJECT_RELEASE(ji);

        cm_sql_insert_item(groupDB, CM_TBL_ELEM_ATTRIBUTES, documentName, key, full_index_name, val, range, exp, exp_others);

        free(full_index_name);

        /* Publish SRDI delta */
        if (exp > 0 && self->record_delta) {
            entry = jxta_srdi_new_element_2(jskey, jval, jns, jPkey, jrange, exp);
            if (entry != NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "add SRDI delta entry: SKey:%s Val:%s\n",
                                jstring_get_string(jskey), jstring_get_string(jval));
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, jstring_get_string(fmtTime), exp);

                apr_thread_mutex_lock(self->mutex);

                status = jxta_hashtable_get(self->srdi_delta, folder->name, (size_t) strlen(folder->name),
                                            JXTA_OBJECT_PPTR(&delta_entries));
                if ( status != JXTA_SUCCESS) {
                    /* first time we see that value, add a list for it */
                    delta_entries = jxta_vector_new(0);
                    jxta_hashtable_put(self->srdi_delta, folder->name, strlen(folder->name), (Jxta_object *) delta_entries);
                }

                status = jxta_vector_add_object_last(delta_entries, (Jxta_object *) entry);
                apr_thread_mutex_unlock(self->mutex);
                JXTA_OBJECT_RELEASE(delta_entries);
                JXTA_OBJECT_RELEASE(entry);
            }
        }

        if (jrange)
            JXTA_OBJECT_RELEASE(jrange);
        JXTA_OBJECT_RELEASE(jval);
        JXTA_OBJECT_RELEASE(jskey);
        free(val);
    }

    if (sv != NULL) {
        JXTA_OBJECT_RELEASE(sv);
    }

    JXTA_OBJECT_RELEASE(jns);
    JXTA_OBJECT_RELEASE(jPkey);
}

static void folder_reindex(Jxta_cm * self, Folder * folder)
{
    char **keys = NULL;
    JString *jAdv = NULL;
    char *sAdv = NULL;
    int i;
    Jxta_expiration_time exp;

    cm_sql_delete_with_where(self->groupDB, CM_TBL_SRDI, NULL);
    cm_sql_delete_with_where(self->groupDB, CM_TBL_ELEM_ATTRIBUTES, NULL);

    keys = jxta_cm_get_primary_keys(self, (char *) folder->name);

    for (i = 0; keys != NULL && keys[i] != NULL; i++) {
        Jxta_vector *content;
        Jxta_advertisement *adv = jxta_advertisement_new();

        jxta_cm_restore_bytes(self, (char *) folder->name, keys[i], &jAdv);

        sAdv = (char *) jstring_get_string(jAdv);
        jxta_advertisement_parse_charbuffer(adv, sAdv, strlen(sAdv));
        jxta_advertisement_get_advs(adv, &content);
        jxta_cm_get_expiration_time(self, (char *) folder->name, keys[i], &exp);
        JXTA_OBJECT_RELEASE(jAdv);

        if (NULL == content) {
            JXTA_OBJECT_RELEASE(adv);
            free(keys[i]);
            continue;
        }
        if (jxta_vector_size(content) == 0) {
            JXTA_OBJECT_RELEASE(adv);
            free(keys[i]);
            continue;
        }

        /* Reuse variable adv for the single contained advertisement */
        jxta_vector_get_object_at(content, JXTA_OBJECT_PPTR(&adv), (unsigned int) 0);
        JXTA_OBJECT_RELEASE(content);

        secondary_indexing(self, folder, adv, keys[i], exp, exp);

        free(keys[i]);

        JXTA_OBJECT_RELEASE(adv);
    }
    
    if (keys != NULL) {
        free(keys);
    }
}

/* save advertisement */
JXTA_DECLARE(Jxta_status)
    jxta_cm_save(Jxta_cm * self, const char *folder_name, char *primary_key,
             Jxta_advertisement * adv, Jxta_expiration_time timeOutForMe, Jxta_expiration_time timeOutForOthers)
{
    Folder *folder;
    Jxta_status status;
    const char * name;

    name = jxta_advertisement_get_document_name(adv);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s primary key: %s\n", name, primary_key);

    apr_thread_mutex_lock(self->mutex);

    status = jxta_hashtable_get(self->folders, folder_name, (size_t) strlen(folder_name), JXTA_OBJECT_PPTR(&folder));
    if (status != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Not saved 2\n");
        return status;
    }

    apr_thread_mutex_unlock(self->mutex);

    status = cm_sql_save_advertisement(self->groupDB, primary_key, adv, timeOutForMe, timeOutForOthers);
    /*
     * Do secondary indexing now.
     */
    secondary_indexing(self, folder, adv, primary_key, timeOutForMe, timeOutForOthers);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Saved\n");
    JXTA_OBJECT_RELEASE(folder);
    return JXTA_SUCCESS;
}

/* save srdi entry */
JXTA_DECLARE(Jxta_status) jxta_cm_save_srdi(Jxta_cm * self, JString * handler, JString * peerid, JString * primaryKey,
                                            Jxta_SRDIEntryElement * entry)
{
    Jxta_status status;

    if (primaryKey) {
    }

    status = cm_sql_save_srdi(self->groupDB, handler, peerid, entry->nameSpace, entry->advId, 
                              entry->key, entry->value, entry->range, entry->expiration, FALSE);
    return status;
}

/* save srdi entry in the replica table */
JXTA_DECLARE(Jxta_status) jxta_cm_save_replica(Jxta_cm * self, JString * handler, JString * peerid, JString * primaryKey,
                                               Jxta_SRDIEntryElement * entry)
{
    Jxta_status status;

    if (primaryKey) {
    }

    status = cm_sql_save_srdi(self->groupDB, handler, peerid, entry->nameSpace, entry->advId, 
                              entry->key, entry->value, entry->range, entry->expiration, TRUE);
    return status;
}

/* search for advertisements with specific (attribute,value) pairs */
/* a NULL pointer means the end of the list */

JXTA_DECLARE(char **) jxta_cm_search(Jxta_cm * self, char *folder_name, const char *attribute, const char *value, int n_adv)
{
    char **all_primaries;

    JString *where = jstring_new_0();
    JString *jAttr = jstring_new_2(attribute);
    JString *jVal = jstring_new_2(value);
    JString *jGroup = jstring_new_0();

    jstring_append_2(where, CM_COL_Name);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jAttr);

    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, CM_COL_Value);
    if (jxta_sql_escape_and_wc_value(jVal, TRUE)) {
        jstring_append_2(where, SQL_LIKE);
    } else {
        jstring_append_2(where, SQL_EQUAL);
    }
    SQL_VALUE(where, jVal);
    jstring_append_2(jGroup, SQL_GROUP);
    jstring_append_2(jGroup, CM_COL_AdvId);

    all_primaries = cm_sql_get_primary_keys(self, folder_name, CM_TBL_ELEM_ATTRIBUTES, where, jGroup, n_adv);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "got a query for attr - %s  val - %s \n", attribute, value);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jAttr);
    JXTA_OBJECT_RELEASE(jVal);
    JXTA_OBJECT_RELEASE(jGroup);

    return all_primaries;
}

/* search for advertisements with specific (attribute,value) pairs */
/* a NULL pointer means the end of the list */

JXTA_DECLARE(char **) jxta_cm_search_srdi(Jxta_cm * self, char *folder_name, const char *attribute, const char *value, int n_adv)
{
    char **all_primaries;

    JString *where = jstring_new_0();
    JString *jAttr = jstring_new_2(attribute);
    JString *jVal = jstring_new_2(value);
    JString *jGroup = jstring_new_0();

    jstring_append_2(where, CM_COL_Name);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jAttr);

    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, CM_COL_Value);
    if (jxta_sql_escape_and_wc_value(jVal, TRUE)) {
        jstring_append_2(where, SQL_LIKE);
    } else {
        jstring_append_2(where, SQL_EQUAL);
    }
    SQL_VALUE(where, jVal);
    jstring_append_2(jGroup, SQL_GROUP);
    jstring_append_2(jGroup, CM_COL_AdvId);

    all_primaries = cm_sql_get_primary_keys(self, folder_name, CM_TBL_SRDI, where, jGroup, n_adv);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "got a query for attr - %s  val - %s \n", attribute, value);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jAttr);
    JXTA_OBJECT_RELEASE(jVal);
    JXTA_OBJECT_RELEASE(jGroup);

    return all_primaries;
}

JXTA_DECLARE(Jxta_advertisement **) jxta_cm_sql_query(Jxta_cm * self, const char *folder_name, JString * where)
{
    Jxta_advertisement **results = NULL;
    Jxta_status status = JXTA_SUCCESS;
    GroupDB *groupDB = self->groupDB;
    Jxta_advertisement *newAdv = NULL;
    Jxta_vector *res_vect;
    Jxta_object *res_adv;
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row = NULL;
    const char *entry;
    int nTuples, i, rv;
    JString *columns = jstring_new_0();

    if (folder_name) {
    }

    jstring_append_2(columns, CM_COL_Advert);

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create apr_pool: %d\n", aprs);
        goto finish;
    }

    rv = cm_sql_select_join(groupDB, pool, &res, where);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- jxta_cm_sql_query Select failed: %s %i\n",
                        groupDB->id, apr_dbd_error(groupDB->driver, groupDB->sql, rv), rv);
        goto finish;
    }
    nTuples = apr_dbd_num_tuples(groupDB->driver, res);
    if (nTuples == 0)
        goto finish;
    results = (Jxta_advertisement **) malloc(sizeof(Jxta_advertisement *) * (nTuples + 1));
    i = 0;
    for (rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1);
         rv == 0; rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1)) {
        const char *advId;

        advId = apr_dbd_get_entry(groupDB->driver, row, 0);
        entry = apr_dbd_get_entry(groupDB->driver, row, 1);
        if (entry == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s -- (null) in entry from jxta_cm_sql_query", groupDB->id);
            continue;
        } else {
            char *tmp;
            int len;
            res_vect = NULL;
            res_adv = NULL;

            len = strlen(entry);
            tmp = malloc(len + 1);
            memcpy(tmp, entry, len + 1);
            newAdv = jxta_advertisement_new();
            status = jxta_advertisement_parse_charbuffer(newAdv, tmp, len);
            jxta_advertisement_get_advs(newAdv, &res_vect);
            free(tmp);
            JXTA_OBJECT_RELEASE(newAdv);

            if (res_vect == NULL) {
                continue;
            }
            if (jxta_vector_size(res_vect) != 0) {
                Jxta_advertisement *res_adv;

                jxta_vector_get_object_at(res_vect, JXTA_OBJECT_PPTR(&res_adv), (unsigned int) 0);
                jxta_advertisement_set_local_id_string(res_adv, advId);
                results[i++] = res_adv;
            }
            JXTA_OBJECT_RELEASE(res_vect);
        }

    }
    if (results != NULL) {
        results[i] = NULL;
    }

  finish:
    JXTA_OBJECT_RELEASE(columns);
    apr_pool_destroy(pool);

    return results;
}

static Jxta_object **cm_build_advertisements(GroupDB * groupDB, apr_pool_t * pool, apr_dbd_results_t * res)
{
    const char *name;
    const char *value;
    const char *peerid;
    const char *nameSpace;
    const char *advid;
    const char *tmpid;
    const char *tmpPid;
    int nTuples, i, rv;
    Jxta_object **results;
    Jxta_object **ret;
    apr_dbd_row_t *row = NULL;
    Jxta_ProfferAdvertisement *profferAdv = NULL;

    /**
    *    Column Order
    *    ------------
    *    Peerid
    *    AdvId
    *    Name
    *    Value
    *    NameSpace
    **/
    
    nTuples = apr_dbd_num_tuples(groupDB->driver, res);
    if (nTuples == 0)
        return NULL;
    ret = calloc((nTuples + 1), sizeof(JString *) + sizeof(Jxta_ProfferAdvertisement *));
    results = ret;
    i = 0;
    for (rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1);
         rv == 0; rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1)) {
        peerid = apr_dbd_get_entry(groupDB->driver, row, 0);
        if (peerid == NULL) {
            continue;
        }
        advid = apr_dbd_get_entry(groupDB->driver, row, 1);
        if (advid == NULL) {
            continue;
        }
        name = apr_dbd_get_entry(groupDB->driver, row, 2);
        if (name == NULL) {
            continue;
        }
        value = apr_dbd_get_entry(groupDB->driver, row, 3);
        if (value == NULL) {
            continue;
        }
        nameSpace = apr_dbd_get_entry(groupDB->driver, row, 4);
        if (nameSpace == NULL) {
            continue;
        }
        if (profferAdv == NULL) {
            profferAdv = jxta_ProfferAdvertisement_new();
            jxta_proffer_adv_set_nameSpace(profferAdv, nameSpace);
            jxta_proffer_adv_set_advId(profferAdv, advid);
            jxta_proffer_adv_set_peerId(profferAdv, peerid);
        }
        tmpid = jxta_proffer_adv_get_advId(profferAdv);
        tmpPid = jxta_proffer_adv_get_peerId(profferAdv);
        if (!strcmp(tmpPid, peerid) && !strcmp(tmpid, advid)) {
            jxta_proffer_adv_add_item(profferAdv, name, value);
        } else {
            *(results++) = (Jxta_object *) profferAdv;
            *(results++) = (Jxta_object *) jstring_new_2(peerid);
            profferAdv = jxta_ProfferAdvertisement_new();
            jxta_proffer_adv_set_nameSpace(profferAdv, nameSpace);
            jxta_proffer_adv_set_advId(profferAdv, advid);
            jxta_proffer_adv_set_peerId(profferAdv, peerid);
            jxta_proffer_adv_add_item(profferAdv, name, value);
        }

    }
    if (profferAdv != NULL) {
        *(results++) = (Jxta_object *) profferAdv;
        *(results++) = (Jxta_object *) jstring_new_2(peerid);
    }
    *(results) = NULL;
    return ret;
}

JXTA_DECLARE(Jxta_object **) jxta_cm_sql_query_srdi(Jxta_cm * self, const char *folder_name, JString * where)
{
    GroupDB *groupDB = self->groupDB;
    JString *whereClone = jstring_clone(where);
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    apr_dbd_results_t *res = NULL;
    Jxta_object **adds = NULL;
    int rv;
    JString *columns = jstring_new_0();

    if (folder_name) {
    }

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", groupDB->id, aprs);
        goto finish;
    }

    jstring_append_2(columns, CM_COL_Peerid);
    jstring_append_2(columns, SQL_COMMA);
    jstring_append_2(columns, CM_COL_AdvId);
    jstring_append_2(columns, SQL_COMMA);
    jstring_append_2(columns, CM_COL_Name);
    jstring_append_2(columns, SQL_COMMA);
    jstring_append_2(columns, CM_COL_Value);
    jstring_append_2(columns, SQL_COMMA);
    jstring_append_2(columns, CM_COL_NameSpace);

    jstring_append_2(whereClone, SQL_AND);
    cm_sql_append_timeout_check_greater(whereClone);
    jstring_append_2(whereClone, SQL_ORDER);
    jstring_append_2(whereClone, CM_COL_AdvId);
    jstring_append_2(whereClone, SQL_COMMA);
    jstring_append_2(whereClone, CM_COL_Peerid);

    rv = cm_sql_select(groupDB, pool, CM_TBL_SRDI_SRC, &res, columns, whereClone);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- jxta_cm_sql_query_srdi Select failed: %s %i\n",
                        groupDB->id, apr_dbd_error(groupDB->driver, groupDB->sql, rv), rv);
        goto finish;
    }

    adds = cm_build_advertisements(groupDB, pool, res);

  finish:
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(whereClone);
    return adds;
}

JXTA_DECLARE(Jxta_vector *) jxta_cm_query_replica(Jxta_cm * self, JString * nameSpace, Jxta_vector * queries)
{
    Jxta_status status;
    GroupDB *groupDB;
    JString *jWhere;
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    Jxta_boolean bNoSubQueries = FALSE;
    JString *jNsWhere;
    JString *jNs;
    Jxta_hashtable *andHash = NULL;
    Jxta_hashtable *peersHash = NULL;
    apr_dbd_results_t *res = NULL;
    Jxta_object **adds = NULL;
    Jxta_object **saveAdds = NULL;
    Jxta_vector *ret = NULL;
    int rv;
    unsigned int i;
    JString *jColumns;
    const char *peerId;
    const char *elemSQL = NULL;
    JString *jPeerId = NULL;

    jColumns = jstring_new_0();
    jNs = jstring_clone(nameSpace);
    jWhere = jstring_new_0();
    jNsWhere = jstring_new_0();
    groupDB = self->groupDB;

    jstring_append_2(jColumns, CM_COL_Peerid);
    jstring_append_2(jColumns, SQL_COMMA);
    jstring_append_2(jColumns, CM_COL_AdvId);
    jstring_append_2(jColumns, SQL_COMMA);
    jstring_append_2(jColumns, CM_COL_Name);
    jstring_append_2(jColumns, SQL_COMMA);
    jstring_append_2(jColumns, CM_COL_Value);
    jstring_append_2(jColumns, SQL_COMMA);
    jstring_append_2(jColumns, CM_COL_NameSpace);
    jstring_append_2(jNsWhere, CM_COL_SRC);
    jstring_append_2(jNsWhere, SQL_DOT);
    jstring_append_2(jNsWhere, CM_COL_NameSpace);
    if (jxta_sql_escape_and_wc_value(jNs, TRUE)) {
        jstring_append_2(jNsWhere, SQL_LIKE);
    } else {
        jstring_append_2(jNsWhere, SQL_EQUAL);
    }
    SQL_VALUE(jNsWhere, jNs);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- query with %i sub queries\n", groupDB->id, jxta_vector_size(queries));
    
    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", groupDB->id, aprs);
        goto finish;
    }
    if (0 == jxta_vector_size(queries)) {
        bNoSubQueries = TRUE;
    }
    for (i = 0; i < jxta_vector_size(queries) || bNoSubQueries; i++) {
        Jxta_query_element *elem = NULL;
        const char *bool = NULL;

        if (bNoSubQueries) {
            bool = SQL_OR;
        } else {
            status = jxta_vector_get_object_at(queries, JXTA_OBJECT_PPTR(&elem), i);
            bool = jstring_get_string(elem->jBoolean);
            elemSQL = jstring_get_string(elem->jSQL);
            JXTA_OBJECT_RELEASE(elem);
        }
        jstring_reset(jWhere, NULL);
        jstring_append_1(jWhere, jNsWhere);
        if (!bNoSubQueries) {
            jstring_append_2(jWhere, SQL_AND);
            jstring_append_2(jWhere, elemSQL);
        }

        jstring_append_2(jWhere, SQL_AND);
        jstring_append_2(jWhere, CM_COL_SRC);
        jstring_append_2(jWhere, SQL_DOT);
        cm_sql_append_timeout_check_greater(jWhere);

        jstring_append_2(jWhere, SQL_ORDER);
        jstring_append_2(jWhere, CM_COL_AdvId);
        jstring_append_2(jWhere, SQL_COMMA);
        jstring_append_2(jWhere, CM_COL_Peerid);

        rv = cm_sql_select(groupDB, pool, CM_TBL_REPLICA_SRC, &res, jColumns, jWhere);

        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- jxta_cm_sql_query_replica Select failed: %s %i\n",
                            groupDB->id, apr_dbd_error(groupDB->driver, groupDB->sql, rv), rv);
            goto finish;
        }

        adds = cm_build_advertisements(groupDB, pool, res);
        if (NULL == adds && !bNoSubQueries)
            continue;

        if (NULL == peersHash) {
            peersHash = jxta_hashtable_new(1);
        }
        saveAdds = adds;
        if (!strcmp(bool, SQL_OR)) {
            while (NULL != adds && *adds) {
                peerId = jstring_get_string((JString *) * (adds + 1));
                if (jxta_hashtable_get(peersHash, peerId, strlen(peerId), JXTA_OBJECT_PPTR(&jPeerId)) != JXTA_SUCCESS) {
                    jxta_hashtable_put(peersHash, peerId, strlen(peerId),
                                       (Jxta_object *) jstring_clone((JString *) * (adds + 1)));
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Added to the peersHash: %s\n", groupDB->id, peerId);
                } else {
                    JXTA_OBJECT_RELEASE(jPeerId);
                    jPeerId = NULL;
                }
                JXTA_OBJECT_RELEASE(*adds);
                JXTA_OBJECT_RELEASE(*(adds + 1));
                adds += 2;
            }
        } else {
            Jxta_vector *peersV = jxta_vector_new(1);
            if (NULL == andHash) {
                andHash = jxta_hashtable_new(1);
            }
            saveAdds = adds;
            while (NULL != adds && *adds) {
                jxta_vector_add_object_first(peersV, (Jxta_object *) jstring_clone((JString *) * (adds + 1)));
                JXTA_OBJECT_RELEASE(*adds);
                JXTA_OBJECT_RELEASE(*(adds + 1));
                adds += 2;
            }
            jxta_hashtable_put(andHash, elemSQL, strlen(elemSQL), (Jxta_object *) peersV);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Added to the andHash:%i %s\n", groupDB->id,
                            jxta_vector_size(peersV), elemSQL);
            JXTA_OBJECT_RELEASE(peersV);
        }
        free(saveAdds);
        if (bNoSubQueries)
            break;
    }
    if (NULL != andHash) {
        Jxta_vector *results;
        unsigned int j;
        Jxta_vector *finalResult;
        int leastPopular = 0;

        results = jxta_hashtable_values_get(andHash);
        finalResult = jxta_vector_new(1);
        for (j = 0; j < jxta_vector_size(results); j++) {
            Jxta_vector *result = NULL;
            int size = 0;

            status = jxta_vector_get_object_at(results, JXTA_OBJECT_PPTR(&result), j);
            if (status != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s -- Error returning result from the results vector : %i \n",
                                groupDB->id, status);
                if (result != NULL)
                    JXTA_OBJECT_RELEASE(result);
                continue;
            }
            size = jxta_vector_size(result);
            if (leastPopular == 0) {
                leastPopular = size;
                jxta_vector_add_object_first(finalResult, (Jxta_object *) result);
            } else if (size > leastPopular) {
                JXTA_OBJECT_RELEASE(result);
            } else if (size == leastPopular) {
                jxta_vector_add_object_first(finalResult, (Jxta_object *) result);
            } else if (size < leastPopular) {
                jxta_vector_clear(finalResult);
                jxta_vector_add_object_first(finalResult, (Jxta_object *) result);
            }
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Final result:%i items\n", groupDB->id,
                        jxta_vector_size(finalResult));
        for (j = 0; j < jxta_vector_size(finalResult); j++) {
            Jxta_vector *peerEntries;
            unsigned int x;
            
            jxta_vector_get_object_at(finalResult, JXTA_OBJECT_PPTR(&peerEntries), j);
            for (x = 0; x < jxta_vector_size(peerEntries); x++) {
                JString *peer;
                const char *tmpPeer;

                status = jxta_vector_get_object_at(peerEntries, JXTA_OBJECT_PPTR(&peer), x);
                if (JXTA_SUCCESS != status) {
                    if (NULL != peer)
                        JXTA_OBJECT_RELEASE(peer);
                    peer = NULL;
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                                    "%s -- Error returning result from the finalResults vector : %i \n", groupDB->id, status);
                    continue;
                }
                if (peersHash == NULL) {
                    peersHash = jxta_hashtable_new(jxta_vector_size(peerEntries));
                }
                tmpPeer = jstring_get_string(peer);
                if (jxta_hashtable_get(peersHash, tmpPeer, strlen(tmpPeer), JXTA_OBJECT_PPTR(&peer)) != JXTA_SUCCESS) {
                    jxta_hashtable_put(peersHash, tmpPeer, strlen(tmpPeer), (Jxta_object *) peer);
                }
                JXTA_OBJECT_RELEASE(peer);
                peer = NULL;
            }
        }
    }
    if (NULL != peersHash) {
        ret = jxta_hashtable_values_get(peersHash);
    } else {
        ret = NULL;
    }

  finish:
    JXTA_OBJECT_RELEASE(jColumns);
    JXTA_OBJECT_RELEASE(jNs);
    JXTA_OBJECT_RELEASE(jNsWhere);
    JXTA_OBJECT_RELEASE(jWhere);
    if (NULL != andHash) {
        JXTA_OBJECT_RELEASE(andHash);
    }
    if (NULL != peersHash) {
        JXTA_OBJECT_RELEASE(peersHash);
    }
    if (NULL != pool)
        apr_pool_destroy(pool);

    return ret;
}

/* restore the original advertisement from the cache */
JXTA_DECLARE(Jxta_status) jxta_cm_restore_bytes(Jxta_cm * self, char *folder_name, char *primary_key, JString ** bytes)
{
    Jxta_status status = JXTA_ITEM_NOTFOUND;

    GroupDB *groupDB = self->groupDB;
    JString *where = jstring_new_0();
    JString *columns = jstring_new_0();
    JString *jKey = jstring_new_2(primary_key);
    JString *jFolder = jstring_new_2(folder_name);
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row;
    const char *entry;
    int rv = 0;

    jstring_append_2(columns, CM_COL_Advert);
    jstring_append_2(where, CM_COL_AdvId);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jKey);

    if (!strcmp(folder_name, "Peers")) {
        jstring_append_2(where, SQL_AND);
        jstring_append_2(where, CM_COL_NameSpace);
        jstring_append_2(where, SQL_EQUAL);
        SQL_VALUE(where, jPeerNs);
        jstring_append_2(where, SQL_OR);
        jstring_append_2(where, CM_COL_NameSpace);
        jstring_append_2(where, SQL_EQUAL);
        SQL_VALUE(where, jFolder);
    } else if (!strcmp(folder_name, "Groups")) {
        jstring_append_2(where, SQL_AND);
        jstring_append_2(where, CM_COL_NameSpace);
        jstring_append_2(where, SQL_EQUAL);
        SQL_VALUE(where, jGroupNs);
        jstring_append_2(where, SQL_OR);
        jstring_append_2(where, CM_COL_NameSpace);
        jstring_append_2(where, SQL_EQUAL);
        SQL_VALUE(where, jFolder);
    } else {
        jstring_append_2(where, SQL_AND);
        jstring_append_2(where, CM_COL_NameSpace);
        jstring_append_2(where, SQL_NOT_EQUAL);
        SQL_VALUE(where, jGroupNs);
        jstring_append_2(where, SQL_AND);
        jstring_append_2(where, CM_COL_NameSpace);
        jstring_append_2(where, SQL_NOT_EQUAL);
        SQL_VALUE(where, jPeerNs);
    }

    jstring_append_2(where, SQL_AND);
    cm_sql_append_timeout_check_greater(where);

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", groupDB->id, aprs);
        goto finish;
    }

    rv = cm_sql_select(groupDB, pool, CM_TBL_ADVERTISEMENTS, &res, columns, where);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- jxta_cm_restore_bytes Select failed: %s %i\n",
                        groupDB->id, apr_dbd_error(groupDB->driver, groupDB->sql, rv), rv);
        goto finish;
    }
    rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1);

    if (rv == 0) {
        entry = apr_dbd_get_entry(groupDB->driver, row, 0);
        if (entry != NULL) {
            *bytes = jstring_new_0();
            jstring_append_2(*bytes, entry);
            status = JXTA_SUCCESS;
        }
    }

  finish:
    if (NULL != pool)
        apr_pool_destroy(pool);

    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jKey);
    JXTA_OBJECT_RELEASE(jFolder);
    return status;
}

static Jxta_status cm_get_time(Jxta_cm * self, const char *folder_name, const char *primary_key, Jxta_expiration_time * time,
                               const char *time_name)
{
    Folder *folder;
    Jxta_status status;
    Jxta_expiration_time exp[2];
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    int rv;
    GroupDB *groupDB = self->groupDB;
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row;
    const char *entry = NULL;
    JString *columns = jstring_new_0();
    JString *where = jstring_new_0();
    JString *jKey = jstring_new_0();

    jstring_append_2(jKey, primary_key);

    apr_thread_mutex_lock(self->mutex);

    status = jxta_hashtable_get(self->folders, folder_name, (size_t) strlen(folder_name), JXTA_OBJECT_PPTR(&folder));
    if (status != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        return status;
    }

    apr_thread_mutex_unlock(self->mutex);

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", groupDB->id, aprs);
        goto finish;
    }

    jstring_append_2(columns, time_name);
    jstring_append_2(where, CM_COL_AdvId);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jKey);

    rv = cm_sql_select(groupDB, pool, CM_TBL_ADVERTISEMENTS, &res, columns, where);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- jxta_cm_get_expiration_time Select failed: %s %i\n",
                        groupDB->id, apr_dbd_error(groupDB->driver, groupDB->sql, rv), rv);
        goto finish;
    }
    rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1);

    if (rv == 0) {
        entry = apr_dbd_get_entry(groupDB->driver, row, 0);
    }

    JXTA_OBJECT_RELEASE(folder);

    if (entry == NULL)
        status = JXTA_ITEM_NOTFOUND;

    if (status == JXTA_SUCCESS) {

        sscanf(entry, jstring_get_string(jInt64_for_format), &exp[0]);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "expiration time retreived \n");
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, jstring_get_string(fmtTime), exp[0]);
        *time = exp[0];
    }

  finish:
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jKey);

    return status;
}

JXTA_DECLARE(Jxta_status) jxta_cm_get_others_time(Jxta_cm * self, char *folder_name, char *primary_key,
                                                  Jxta_expiration_time * time)
{
    return cm_get_time(self, folder_name, primary_key, time, CM_COL_TimeOutForOthers);
}

/* Get the expiration time of an advertisement */
JXTA_DECLARE(Jxta_status) jxta_cm_get_expiration_time(Jxta_cm * self, char *folder_name, char *primary_key,
                                                      Jxta_expiration_time * time)
{
    return cm_get_time(self, folder_name, primary_key, time, CM_COL_TimeOut);
}

/* delete files where the expiration time is less than curr_expiration */
JXTA_DECLARE(Jxta_status) jxta_cm_remove_expired_records(Jxta_cm * self)
{
    Jxta_vector *all_folders;
    int cnt, i;
    Folder *folder;
    Jxta_status rt, latched_rt = JXTA_SUCCESS;

    apr_thread_mutex_lock(self->mutex);

    all_folders = jxta_hashtable_values_get(self->folders);
    if (NULL == all_folders) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s -- Failed to retrieve folders", self->groupDB->id);
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_NOMEM;
    }
    apr_thread_mutex_unlock(self->mutex);

    cnt = jxta_vector_size(all_folders);
    for (i = 0; i < cnt; i++) {
        jxta_vector_get_object_at(all_folders, JXTA_OBJECT_PPTR(&folder), i);
        rt = folder_remove_expired_records(self, folder);
        if (JXTA_SUCCESS != rt) {
            latched_rt = rt;
            /* return rt */ ;
        }
        JXTA_OBJECT_RELEASE(folder);
    }
    JXTA_OBJECT_RELEASE(all_folders);

    return latched_rt;
}

/* create a folder with type <type> and index keywords <keys> */
JXTA_DECLARE(Jxta_status) jxta_cm_create_folder(Jxta_cm * self, char *folder_name, const char *keys[])
{
/*    const char **p; */


    Folder *folder = NULL;
    Jxta_vector *iv = NULL;

    apr_thread_mutex_lock(self->mutex);

    if (jxta_hashtable_get(self->folders, folder_name, (size_t) strlen(folder_name), JXTA_OBJECT_PPTR(&folder)) == JXTA_SUCCESS) {
        if (folder)
            JXTA_OBJECT_RELEASE(folder);
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_SUCCESS;
    }

    folder = malloc(sizeof(Folder));
    memset(folder, 0, sizeof(*folder));
    JXTA_OBJECT_INIT(folder, folder_free, 0);

    folder->name = strdup(folder_name);

    jxta_hashtable_put(self->folders, folder_name, strlen(folder_name), (Jxta_object *) folder);

    if (keys != NULL) {
        iv = jxta_advertisement_return_indexes(keys);
        jxta_cm_create_adv_indexes(self, folder_name, iv);
        JXTA_OBJECT_RELEASE(iv);
    }

    apr_thread_mutex_unlock(self->mutex);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Removing expired %s Advertisements \n", self->groupDB->id,
                    folder_name);

    folder_remove_expired_records(self, folder);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Removing expired %s Advertisements - Done \n", self->groupDB->id,
                    folder_name);

    JXTA_OBJECT_RELEASE(folder);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_cm_create_adv_indexes(Jxta_cm * self, char *folder_name, Jxta_vector * jv)
{
    unsigned int i = 0;
    Folder *folder = NULL;
    Jxta_status status = 0;

    status = jxta_hashtable_get(self->folders, folder_name, (size_t) strlen(folder_name), JXTA_OBJECT_PPTR(&folder));
    if ( status != JXTA_SUCCESS) {
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
        int len;
        
        status = jxta_vector_get_object_at(jv, JXTA_OBJECT_PPTR(&ji), i);
        if (status == JXTA_SUCCESS) {
            if (ji->element != NULL) {
                char *element = (char *) jstring_get_string(ji->element);
                Jxta_hashtable *attrib = NULL;

                if (ji->attribute != NULL) {
                    attributeName = (char *) jstring_get_string(ji->attribute);
                    len = strlen(attributeName) + strlen(element) + 6;
                    full_index_name = calloc(1, len);
                    apr_snprintf(full_index_name, len, "%s %s", element, attributeName);
                } else {
                    len = strlen(element) + 6;
                    full_index_name = calloc(1, len);
                    apr_snprintf(full_index_name, len, "%s", element);
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
    JXTA_OBJECT_RELEASE(folder);
    return JXTA_SUCCESS;
}

/*
 * hash a document
 */
JXTA_DECLARE(unsigned long) jxta_cm_hash(const char *key, size_t ksz)
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

JXTA_DECLARE(void) jxta_cm_close(Jxta_cm * self)
{
    GroupDB *groupDB = self->groupDB;

    apr_thread_mutex_lock(self->mutex);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "cm_close\n");
    JXTA_OBJECT_RELEASE(groupDB);
    JXTA_OBJECT_RELEASE(self->folders);
    self->folders = NULL;
    apr_thread_mutex_unlock(self->mutex);
}

/*
 * obtain all CM SRDI delta entries for a particular folder
 */
JXTA_DECLARE(Jxta_vector *) jxta_cm_get_srdi_delta_entries(Jxta_cm * self, JString * folder_name)
{
    Jxta_vector *delta_entries = NULL;

    apr_thread_mutex_lock(self->mutex);
    if (jxta_hashtable_del(self->srdi_delta, jstring_get_string(folder_name), (size_t) strlen(jstring_get_string(folder_name))
                           , JXTA_OBJECT_PPTR(&delta_entries)) != JXTA_SUCCESS) {
        apr_thread_mutex_unlock(self->mutex);
        if (NULL != delta_entries)
            JXTA_OBJECT_RELEASE(delta_entries);
        return NULL;
    }
    apr_thread_mutex_unlock(self->mutex);
    return delta_entries;
}

JXTA_DECLARE(void) jxta_cm_set_delta(Jxta_cm * self, Jxta_boolean recordDelta)
{
    self->record_delta = recordDelta;
}

/*
 * obtain all CM SRDI entries for a particular folder
 */
JXTA_DECLARE(Jxta_vector *) jxta_cm_get_srdi_entries(Jxta_cm * self, JString * folder_name)
{
    size_t nb_keys;
    GroupDB *groupDB = self->groupDB;
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row;
    JString *jAdvId;
    JString *jKey;
    JString *jVal;
    JString *jNumVal;
    JString *jRange;
    JString *jNameSpace;
    JString *columns = jstring_new_0();

    Jxta_expiration_time exp = 0;
    Jxta_vector *entries = jxta_vector_new(0);
    JString *jFormatTime = jstring_new_0();
    JString *where = jstring_new_0();
    Jxta_SRDIEntryElement *element = NULL;
    const char *cFolder_name;
    const char *entry;
    int rv = 0;
    int n, i;

    cFolder_name = jstring_get_string(folder_name);

    jstring_append_2(jFormatTime, "Exp Time ");
    jstring_append_1(jFormatTime, jInt64_for_format);
    jstring_append_2(jFormatTime, "\n");

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", groupDB->id, aprs);
        goto finish;
    }

    cm_sql_correct_space(jstring_get_string(folder_name), where);

    jstring_append_2(columns, CM_COL_AdvId);
    jstring_append_2(columns, SQL_COMMA);
    jstring_append_2(columns, CM_COL_NameSpace);
    jstring_append_2(columns, SQL_COMMA);
    jstring_append_2(columns, CM_COL_Name);
    jstring_append_2(columns, SQL_COMMA);
    jstring_append_2(columns, CM_COL_Value);
    jstring_append_2(columns, SQL_COMMA);
    jstring_append_2(columns, CM_COL_NumValue);
    jstring_append_2(columns, SQL_COMMA);
    jstring_append_2(columns, CM_COL_NumRange);
    jstring_append_2(columns, SQL_COMMA);
    jstring_append_2(columns, CM_COL_TimeOutForOthers);

    jstring_append_2(where, SQL_AND);
    cm_sql_append_timeout_check_greater(where);

    rv = cm_sql_select(groupDB, pool, CM_TBL_ELEM_ATTRIBUTES, &res, columns, where);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- jxta_cm_get_srdi_entries Select failed: %s %i \n %s   \n",
                        groupDB->id, apr_dbd_error(groupDB->driver, groupDB->sql, rv), rv, jstring_get_string(columns));
        goto finish;
    }
    nb_keys = apr_dbd_num_tuples((apr_dbd_driver_t *) groupDB->driver, res);
    if (nb_keys == 0)
        goto finish;
    i = 0;
    for (rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1);
         rv == 0; i++, rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1)) {
        int column_count = apr_dbd_num_cols(groupDB->driver, res);

        jKey = jstring_new_0();
        jVal = jstring_new_0();
        jNumVal = jstring_new_0();
        jNameSpace = jstring_new_0();
        jAdvId = jstring_new_0();
        jRange = jstring_new_0();
        
        for (n = 0; n < column_count; ++n) {
            entry = apr_dbd_get_entry(groupDB->driver, row, n);
            switch (n) {
            case 0:    /* advid */
                jstring_append_2(jAdvId, entry);
                break;
            case 1:    /* Name Space */
                jstring_append_2(jNameSpace, entry);
                break;
            case 2:    /* Name */
                jstring_append_2(jKey, entry);
                break;
            case 3:    /* Value */
                jstring_append_2(jVal, entry);
                break;
            case 4:    /* Numeric Value */
                if (NULL == entry) {
                    jstring_append_2(jNumVal, "");
                } else {
                    jstring_append_2(jNumVal, entry);
                }
                break;
            case 5:    /* Numeric Range */
                if (NULL == entry) {
                    jstring_append_2(jRange, "");
                } else {
                    jstring_append_2(jRange, entry);
                }
            case 6:    /* Time out */
                sscanf(entry, jstring_get_string(jInt64_for_format), &exp);
                break;
            }
        }

        element = jxta_srdi_new_element_2(jKey, jVal, jNameSpace, jAdvId, jRange, exp);
        jxta_vector_add_object_last(entries, (Jxta_object *) element);
        JXTA_OBJECT_RELEASE(jAdvId);
        JXTA_OBJECT_RELEASE(element);
        JXTA_OBJECT_RELEASE(jKey);
        JXTA_OBJECT_RELEASE(jVal);
        JXTA_OBJECT_RELEASE(jNumVal);
        JXTA_OBJECT_RELEASE(jRange);
        JXTA_OBJECT_RELEASE(jNameSpace);
    }

  finish:
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jFormatTime);
    
    if (jxta_vector_size(entries) > 0) {
        return entries;
    } else {
        JXTA_OBJECT_RELEASE(entries);
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_cm_remove_srdi_entries(Jxta_cm * self, JString * peerid)
{
    JString *where;

    where = jstring_new_2(SQL_WHERE);
    jstring_append_2(where, CM_COL_Peerid);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, peerid);

    cm_sql_delete_with_where(self->groupDB, CM_TBL_SRDI, where);
}

static apr_status_t cm_sql_create_table(GroupDB * groupDB, const char *table, const char *columns[])
{
    int rv = 0;
    int nrows;
    Jxta_status status;
    JString *cmd;

    status = cm_sql_table_found(groupDB, table);
    if (status == JXTA_SUCCESS)
        return APR_SUCCESS;

    cmd = jstring_new_0();
    jstring_append_2(cmd, SQL_CREATE_TABLE);
    jstring_append_2(cmd, table);
    jstring_append_2(cmd, SQL_LEFT_PAREN);
    while (*columns) {
        jstring_append_2(cmd, *columns++);
        jstring_append_2(cmd, " ");
        jstring_append_2(cmd, *columns++);
        if (*columns == NULL)
            break;
        jstring_append_2(cmd, SQL_COMMA);
    }
    jstring_append_2(cmd, SQL_RIGHT_PAREN);
    jstring_append_2(cmd, SQL_END_SEMI);

    rv = apr_dbd_query(groupDB->driver, groupDB->sql, &nrows, jstring_get_string(cmd));

    if (rv != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Couldn't create %s  rc=%i\n", groupDB->id, table, rv);
    }
    JXTA_OBJECT_RELEASE(cmd);
    return rv;
}

static Jxta_status cm_sql_table_found(GroupDB * groupDB, const char *table)
{
    int rv = 0;
    Jxta_status status = JXTA_ITEM_NOTFOUND;
    JString *where = jstring_new_0();
    JString *columns = jstring_new_0();
    JString *statement = jstring_new_0();
    JString *jTable = jstring_new_2(table);
    JString *jType = jstring_new_2(SQL_COLUMN_Type_table);
    apr_dbd_results_t *res = NULL;
    apr_pool_t *pool = NULL;
    apr_status_t aprs;

    jstring_append_2(columns, SQL_COLUMN_Name);

    jstring_append_2(where, SQL_COLUMN_Name);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jTable);
    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, SQL_COLUMN_Type);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jType);

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", groupDB->id, aprs);
        status = JXTA_FAILED;
        goto finish;
    }

    rv = cm_sql_select(groupDB, pool, SQL_TBL_MASTER, &res, columns, where);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- cm_sql_table_found Select failed: %s %i\n",
                        groupDB->id, apr_dbd_error(groupDB->driver, groupDB->sql, rv), rv);
        status = JXTA_FAILED;
        goto finish;
    }
    if (apr_dbd_num_tuples(groupDB->driver, res) > 0) {
        status = JXTA_SUCCESS;
    }

  finish:
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(statement);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(jTable);
    JXTA_OBJECT_RELEASE(jType);
    JXTA_OBJECT_RELEASE(where);
    return status;

}
static int cm_sql_create_index(GroupDB * groupDB, const char *table, const char **fields)
{
    int nrows;
    int ret;
    Jxta_status status;
    JString *cmd;
    JString *jIdx = jstring_new_0();

    jstring_append_2(jIdx, table);
    jstring_append_2(jIdx, *fields);

    status = cm_sql_index_found(groupDB, jstring_get_string(jIdx));
    JXTA_OBJECT_RELEASE(jIdx);

    if (status == JXTA_SUCCESS) {
        return APR_SUCCESS;
    }
    cmd = jstring_new_0();

    jstring_append_2(cmd, SQL_CREATE_INDEX);
    jstring_append_2(cmd, table);
    jstring_append_2(cmd, *fields);
    jstring_append_2(cmd, SQL_INDEX_ON);
    jstring_append_2(cmd, table);
    jstring_append_2(cmd, SQL_LEFT_PAREN);
    while (*fields) {
        jstring_append_2(cmd, *fields++);
        if (*fields)
            jstring_append_2(cmd, SQL_COMMA);
    }
    jstring_append_2(cmd, SQL_RIGHT_PAREN);
    jstring_append_2(cmd, SQL_END_SEMI);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s -- %s \n", groupDB->id, jstring_get_string(cmd));

    ret = apr_dbd_query(groupDB->driver, groupDB->sql, &nrows, jstring_get_string(cmd));

    JXTA_OBJECT_RELEASE(cmd);
    return ret;
}

static Jxta_status cm_sql_index_found(GroupDB * groupDB, const char *index)
{
    int rv = 0;
    Jxta_status status = JXTA_ITEM_NOTFOUND;
    JString *where = jstring_new_0();
    JString *columns = jstring_new_0();
    JString *statement = jstring_new_0();
    JString *jIndex = jstring_new_2(index);
    JString *jType = jstring_new_2(SQL_COLUMN_Type_index);
    apr_dbd_results_t *res = NULL;
    apr_pool_t *pool = NULL;
    apr_status_t aprs;

    jstring_append_2(columns, SQL_COLUMN_Name);

    jstring_append_2(where, SQL_COLUMN_Name);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jIndex);
    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, SQL_COLUMN_Type);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jType);

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", groupDB->id, aprs);
        status = JXTA_FAILED;
        goto finish;
    }

    rv = cm_sql_select(groupDB, pool, SQL_TBL_MASTER, &res, columns, where);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- cm_sql_table_found Select failed: %s %i\n",
                        groupDB->id, apr_dbd_error(groupDB->driver, groupDB->sql, rv), rv);
        status = JXTA_FAILED;
        goto finish;
    }
    if (apr_dbd_num_tuples(groupDB->driver, res) > 0) {
        status = JXTA_SUCCESS;
    }

  finish:
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(statement);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(jIndex);
    JXTA_OBJECT_RELEASE(jType);
    JXTA_OBJECT_RELEASE(where);
    return status;

}

static int cm_sql_drop_table(GroupDB * groupDB, char *table)
{
    int rv = 0;
    int nrows;
    JString *statement = jstring_new_2(SQL_DROP_TABLE);
    jstring_append_2(statement, table);

    rv = apr_dbd_query(groupDB->driver, groupDB->sql, &nrows, jstring_get_string(statement));

    JXTA_OBJECT_RELEASE(statement);

    return rv;
}

static Jpr_interval_time get_lifetime(Jpr_interval_time base, Jpr_interval_time span)
{
    Jpr_interval_time lifetime;

    if (JPR_INTERVAL_TIME_MAX == span) {
        return JPR_INTERVAL_TIME_MAX;
    }

    if (JPR_INTERVAL_TIME_MIN == span) {
        return JPR_INTERVAL_TIME_MIN;
    }

    lifetime = base + span;
    if (span > 0) {
        if (lifetime < base) {
            lifetime = JPR_INTERVAL_TIME_MAX;
        }
    } else {
        if (lifetime > base) {
            lifetime = 0;
        }
    }

    return lifetime;
}

static Jxta_status
cm_sql_update_advertisement(GroupDB * groupDB, JString * jNameSpace, JString * jKey, JString * jXml,
                            Jxta_expiration_time timeOutForMe, Jxta_expiration_time timeOutForOthers)
{
    int nrows = 0, rv = 0;
    Jxta_status status = JXTA_SUCCESS;
    char aTime[64];
    const char *sqlCmd = NULL;
    JString *update_sql = jstring_new_0();

    jstring_append_2(update_sql, SQL_UPDATE);
    jstring_append_2(update_sql, CM_TBL_ADVERTISEMENTS);
    jstring_append_2(update_sql, SQL_SET);
    jstring_append_2(update_sql, CM_COL_Advert);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jXml);
    jstring_append_2(update_sql, SQL_COMMA);
    jstring_append_2(update_sql, CM_COL_TimeOut);
    jstring_append_2(update_sql, SQL_EQUAL);

    memset(aTime, 0, 64);
    if (apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, get_lifetime(jpr_time_now(), timeOutForMe)) != 0) {
        jstring_append_2(update_sql, aTime);
    } else {
        jstring_append_2(update_sql, "0");
    }
    jstring_append_2(update_sql, SQL_COMMA);
    jstring_append_2(update_sql, CM_COL_TimeOutForOthers);
    jstring_append_2(update_sql, SQL_EQUAL);

    if (apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, timeOutForOthers) != 0) {
        jstring_append_2(update_sql, aTime);
    } else {
        jstring_append_2(update_sql, "0");
    }

    jstring_append_2(update_sql, SQL_WHERE);
    jstring_append_2(update_sql, CM_COL_AdvId);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jKey);
    jstring_append_2(update_sql, SQL_AND);
    jstring_append_2(update_sql, CM_COL_NameSpace);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jNameSpace);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s -- update : %s \n", groupDB->id, jstring_get_string(update_sql));

    sqlCmd = jstring_get_string(update_sql);
    rv = apr_dbd_query(groupDB->driver, groupDB->sql, &nrows, sqlCmd);

    status = (Jxta_status) rv;
    JXTA_OBJECT_RELEASE(update_sql);
    return status;

}
static Jxta_status
cm_sql_save_advertisement(GroupDB * groupDB, const char *key, Jxta_advertisement * adv, Jxta_expiration_time timeOutForMe,
                          Jxta_expiration_time timeOutForOthers)
{
    int nrows;
    int rv = 0;
    Jxta_status status = JXTA_SUCCESS;
    const char *nameSpace = jxta_advertisement_get_document_name(adv);
    const char *documentName = jxta_advertisement_get_document_name(adv);
    JString *jXml;
    JString *sqlval = jstring_new_0();
    JString *insert_sql = jstring_new_0();
    JString *jTime = jstring_new_0();
    JString *jNameSpace = jstring_new_2(nameSpace);
    JString *jAdvType = jstring_new_2(documentName);
    Jxta_expiration_time lifetime = 0;
    Jxta_expiration_time expTime = 0;
    char aTime[64];
    JString *jKey = jstring_new_2(key);

    jstring_append_2(sqlval, SQL_LEFT_PAREN);
    SQL_VALUE(sqlval, jNameSpace);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jAdvType);
    jstring_append_2(sqlval, SQL_COMMA);

    jxta_advertisement_get_xml(adv, &jXml);
    if (jXml == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s -- Advertisement Not saved \n", groupDB->id);
        status = JXTA_FAILED;
        goto finish;
    }

    SQL_VALUE(sqlval, jXml);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jKey);
    jstring_append_2(sqlval, SQL_COMMA);
    memset(aTime, 0, 64);

    if (apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, get_lifetime(jpr_time_now(), timeOutForMe)) != 0) {
        jstring_append_2(sqlval, aTime);
    } else {
        status = JXTA_FAILED;
        goto finish;
    }

    memset(aTime, 0, 64);
    jstring_append_2(sqlval, SQL_COMMA);
    if (apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, timeOutForOthers) != 0) {
        jstring_append_2(sqlval, aTime);
    } else {
        status = JXTA_FAILED;
        goto finish;
    }

    jstring_append_2(sqlval, SQL_RIGHT_PAREN);

    status = cm_sql_found(groupDB, CM_TBL_ADVERTISEMENTS, nameSpace, jstring_get_string(jKey), NULL, &lifetime);

    if (status == JXTA_SUCCESS) {
        if (timeOutForMe > lifetime) {
            lifetime = timeOutForMe;
        }
        expTime = timeOutForOthers;
        if (expTime > lifetime) {
            expTime = lifetime;
        }
        status = cm_sql_update_advertisement(groupDB, jNameSpace, jKey, jXml, lifetime, expTime);
        goto finish;
    } else if (status == JXTA_ITEM_NOTFOUND) {
        jstring_append_2(insert_sql, SQL_INSERT_INTO);
        jstring_append_2(insert_sql, CM_TBL_ADVERTISEMENTS);
        jstring_append_2(insert_sql, SQL_VALUES);
    } else {
        goto finish;
    }

    jstring_append_1(insert_sql, sqlval);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s -- Save Adv: %s \n", groupDB->id, jstring_get_string(insert_sql));

    rv = apr_dbd_query(groupDB->driver, groupDB->sql, &nrows, jstring_get_string(insert_sql));

    status = (Jxta_status) rv;

  finish:

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Couldn't save advertisement %s  rc=%i\n",
                        groupDB->id, jstring_get_string(insert_sql), rv);
    }

    JXTA_OBJECT_RELEASE(jKey);
    JXTA_OBJECT_RELEASE(jXml);
    JXTA_OBJECT_RELEASE(sqlval);
    JXTA_OBJECT_RELEASE(insert_sql);
    JXTA_OBJECT_RELEASE(jTime);
    JXTA_OBJECT_RELEASE(jNameSpace);
    JXTA_OBJECT_RELEASE(jAdvType);
    return status;
}

static Jxta_status
cm_sql_save_srdi(GroupDB * groupDB, JString * Handler, JString * Peerid, JString * NameSpace, JString * Key, JString * Attribute,
                 JString * Value, JString * Range, Jxta_expiration_time timeOutForMe, Jxta_boolean replica)
{
    int nrows;
    int rv = 0;
    const char *tableName;
    JString *jNameSpace = NULL;
    JString *jHandler = NULL;
    JString *jPeerid = NULL;
    JString *jKey = NULL;
    JString *jRange = NULL;
    JString *jAttribute = NULL;
    JString *jValue = NULL;
    Jxta_status status = JXTA_SUCCESS;
    JString *insert_sql;
    JString *sqlval;
    JString *jTime;
    char aTime[64];

    jAttribute = jstring_clone(Attribute);
    jValue = jstring_clone(Value);
    if (NULL != Range) {
        jRange = jstring_clone(Range);
    }
    insert_sql = jstring_new_0();
    sqlval = jstring_new_0();
    jTime = jstring_new_0();

    if (Handler != NULL) {
        jHandler = jstring_clone(Handler);
    } else {
        jHandler = jstring_new_2("NoHandler");
    }

    if (Peerid != NULL) {
        jPeerid = jstring_clone(Peerid);
    } else {
        jPeerid = jstring_new_2("NoPeerid");
    }

    if (NameSpace != NULL) {
        /* assume the best */
        if (!strcmp(jstring_get_string(NameSpace), "Peers")) {
            jNameSpace = jstring_new_2("jxta:PA");
        } else if (!strcmp(jstring_get_string(NameSpace), "Groups")) {
            jNameSpace = jstring_new_2("jxta:PGA");
        } else {
            jNameSpace = jstring_clone(NameSpace);
        }
    } else {
        jNameSpace = jstring_new_2("ADV");
    }

    if (Key != NULL) {
        jKey = jstring_clone(Key);
    } else {
      /**
       * Note: 20051004 mjan 
       * If we ended up with no key that means that it comes
       * from a Java peer. This fix emulates the behavior 
       * of the JXTA-J2SE side.
       */
      /**jKey = jstring_new_2("NoKEY");**/
        jKey = jstring_clone(jValue);
    }

    jstring_append_2(sqlval, SQL_LEFT_PAREN);
    SQL_VALUE(sqlval, jNameSpace);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jHandler);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jPeerid);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jKey);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jAttribute);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jValue);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_NUMERIC_VALUE(sqlval, jValue);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jRange);
    jstring_append_2(sqlval, SQL_COMMA);
    memset(aTime, 0, 64);

    if (apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, get_lifetime(jpr_time_now(), timeOutForMe)) != 0) {
        jstring_append_2(sqlval, aTime);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                        "Unable to format time for me and current time - set to default timeout \n");
        apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, get_lifetime(jpr_time_now(), DEFAULT_EXPIRATION));
        jstring_append_2(sqlval, aTime);
    }

    jstring_append_2(sqlval, SQL_COMMA);
    memset(aTime, 0, 64);
    if (apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, timeOutForMe) != 0) {
        jstring_append_2(sqlval, aTime);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to format time for me - set to default timeout \n");
        apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, DEFAULT_EXPIRATION);
        jstring_append_2(sqlval, aTime);
    }

    jstring_append_2(sqlval, SQL_RIGHT_PAREN);
    if (replica) {
        tableName = CM_TBL_REPLICA;
    } else {
        tableName = CM_TBL_SRDI;
    }
    status = cm_sql_srdi_found(groupDB, tableName, jNameSpace, jHandler, jPeerid, jKey, jAttribute);

    if (status == JXTA_SUCCESS) {
        status = cm_sql_update_srdi(groupDB, tableName, jHandler, jPeerid, jKey, jAttribute, jValue, jRange, timeOutForMe);
        goto finish;
    } else if (status == JXTA_ITEM_NOTFOUND) {
        jstring_append_2(insert_sql, SQL_INSERT_INTO);
        jstring_append_2(insert_sql, tableName);
        jstring_append_2(insert_sql, SQL_VALUES);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Error searching for srdi entry %i \n", groupDB->id, status);
        goto finish;
    }

    jstring_append_1(insert_sql, sqlval);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Save srdi: %s \n", groupDB->id, jstring_get_string(insert_sql));
    rv = apr_dbd_query(groupDB->driver, groupDB->sql, &nrows, jstring_get_string(insert_sql));

    status = (Jxta_status) rv;

  finish:

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to save srdi entry %s  rc=%i\n",
                        groupDB->id, jstring_get_string(insert_sql), rv);
    }
    JXTA_OBJECT_RELEASE(jNameSpace);
    JXTA_OBJECT_RELEASE(jHandler);
    JXTA_OBJECT_RELEASE(jPeerid);
    JXTA_OBJECT_RELEASE(jKey);
    JXTA_OBJECT_RELEASE(jAttribute);
    JXTA_OBJECT_RELEASE(jValue);
    if (jRange)
        JXTA_OBJECT_RELEASE(jRange);
    JXTA_OBJECT_RELEASE(sqlval);
    JXTA_OBJECT_RELEASE(insert_sql);
    JXTA_OBJECT_RELEASE(jTime);
    return status;
}

static Jxta_status cm_sql_remove_advertisement(GroupDB * groupDB, const char *nameSpace, const char *key)
{
    Jxta_status status = JXTA_SUCCESS;
    JString *where = jstring_new_0();
    JString *jKey = jstring_new_0();

    jstring_append_2(jKey, key);
    jstring_append_2(where, SQL_WHERE);

    cm_sql_correct_space(nameSpace, where);

    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, CM_COL_AdvId);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jKey);

    status = cm_sql_delete_with_where(groupDB, CM_TBL_ELEM_ATTRIBUTES, where);
    if (status != JXTA_SUCCESS)
        goto finish;

    status = cm_sql_delete_with_where(groupDB, CM_TBL_ADVERTISEMENTS, where);
    if (status != JXTA_SUCCESS)
        goto finish;
    
    status = cm_sql_delete_with_where(groupDB, CM_TBL_SRDI, where);
    if (status != JXTA_SUCCESS)
        goto finish;

  finish:
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s -- Unable to remove Advertisement rc=%i \n %s \n", groupDB->id,
                        status, jstring_get_string(where));
    }
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jKey);
    return status;

}
static Jxta_status
cm_sql_update_item(GroupDB * groupDB, const char *table, JString * jNameSpace, JString * jKey, JString * jElemAttr,
                   JString * jVal, JString * jRange, Jxta_expiration_time timeOutForMe)
{
    int nrows = 0;
    int rv = 0;
    const char *sqlCmd = NULL;
    Jxta_status status;
    char aTime[64];
    JString *update_sql = jstring_new_0();

    jstring_append_2(update_sql, SQL_UPDATE);
    jstring_append_2(update_sql, table);
    jstring_append_2(update_sql, SQL_SET);
    jstring_append_2(update_sql, CM_COL_Value);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jVal);
    jstring_append_2(update_sql, SQL_COMMA);
    jstring_append_2(update_sql, CM_COL_NumValue);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_NUMERIC_VALUE(update_sql, jVal);
    jstring_append_2(update_sql, SQL_COMMA);
    jstring_append_2(update_sql, CM_COL_NumRange);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jRange);
    jstring_append_2(update_sql, SQL_COMMA);
    jstring_append_2(update_sql, CM_COL_TimeOut);
    jstring_append_2(update_sql, SQL_EQUAL);

    memset(aTime, 0, 64);

    if (apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, get_lifetime(jpr_time_now(), timeOutForMe)) != 0) {
        jstring_append_2(update_sql, aTime);
    } else {
        jstring_append_2(update_sql, "0");
    }

    jstring_append_2(update_sql, SQL_WHERE);
    jstring_append_2(update_sql, CM_COL_AdvId);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jKey);
    jstring_append_2(update_sql, SQL_AND);
    jstring_append_2(update_sql, CM_COL_NameSpace);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jNameSpace);
    jstring_append_2(update_sql, SQL_AND);
    jstring_append_2(update_sql, CM_COL_Name);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jElemAttr);
    jstring_append_2(update_sql, SQL_END_SEMI);
    sqlCmd = jstring_get_string(update_sql);
    rv = apr_dbd_query(groupDB->driver, groupDB->sql, &nrows, sqlCmd);

    status = (Jxta_status) rv;
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Couldn't update rc=%i \n %s \n", groupDB->id, rv,
                        jstring_get_string(update_sql));
    }

    JXTA_OBJECT_RELEASE(update_sql);
    return status;
}

static Jxta_status
cm_sql_update_srdi(GroupDB * groupDB, const char *table, JString * jHandler, JString * jPeerid, JString * jKey,
                   JString * jAttribute, JString * jValue, JString * jRange, Jxta_expiration_time timeOutForMe)
{
    int nrows;
    int rv;
    Jxta_status status;
    JString *update_sql = jstring_new_0();
    char aTime[64];
    
    jstring_append_2(update_sql, SQL_UPDATE);
    jstring_append_2(update_sql, table);
    jstring_append_2(update_sql, SQL_SET);
    jstring_append_2(update_sql, CM_COL_Value);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jValue);
    jstring_append_2(update_sql, SQL_COMMA);
    jstring_append_2(update_sql, CM_COL_NumValue);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_NUMERIC_VALUE(update_sql, jValue);
    jstring_append_2(update_sql, SQL_COMMA);
    jstring_append_2(update_sql, CM_COL_NumRange);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jRange);
    jstring_append_2(update_sql, SQL_COMMA);
    jstring_append_2(update_sql, CM_COL_TimeOut);
    jstring_append_2(update_sql, SQL_EQUAL);
    memset(aTime, 0, 64);
    if (apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, get_lifetime(jpr_time_now(), timeOutForMe)) != 0) {
        jstring_append_2(update_sql, aTime);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to format time for me - set to default timeout \n");
        apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, get_lifetime(jpr_time_now(), DEFAULT_EXPIRATION));
        jstring_append_2(update_sql, aTime);
    }

    jstring_append_2(update_sql, SQL_WHERE);
    jstring_append_2(update_sql, CM_COL_AdvId);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jKey);
    jstring_append_2(update_sql, SQL_AND);
    jstring_append_2(update_sql, CM_COL_Handler);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jHandler);
    jstring_append_2(update_sql, SQL_AND);
    jstring_append_2(update_sql, CM_COL_Name);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jAttribute);
    jstring_append_2(update_sql, SQL_AND);
    jstring_append_2(update_sql, CM_COL_Peerid);
    jstring_append_2(update_sql, SQL_EQUAL);
    SQL_VALUE(update_sql, jPeerid);
    jstring_append_2(update_sql, SQL_END_SEMI);

    rv = apr_dbd_query(groupDB->driver, groupDB->sql, &nrows, jstring_get_string(update_sql));

    status = (Jxta_status) rv;
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Couldn't update rc=%i\n %s \n", groupDB->id, rv,
                        jstring_get_string(update_sql));
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s -- Updated - %s \n", groupDB->id, jstring_get_string(update_sql));
    }

    JXTA_OBJECT_RELEASE(update_sql);
    return rv;
}
static Jxta_status
cm_sql_insert_item(GroupDB * groupDB, const char *table, const char *nameSpace, const char *key, const char *elemAttr,
                   const char *val, const char *range, Jxta_expiration_time timeOutForMe, Jxta_expiration_time timeOutForOthers)
{
    int nrows;
    int rv = 0;
    Jxta_status status = JXTA_FAILED;
    JString *insert_sql = jstring_new_0();
    JString *jKey = jstring_new_2(key);
    JString *jNameSpace = jstring_new_2(nameSpace);
    JString *jElemAttr = jstring_new_2(elemAttr);
    JString *jVal = jstring_new_2(val);
    JString *jRange = jstring_new_2(range);
    JString *jTime = jstring_new_0();
    JString *jTimeOthers = jstring_new_0();
    Jxta_expiration_time expTime = 0;
    char aTime[64];

    status = cm_sql_found(groupDB, table, nameSpace, jstring_get_string(jKey), elemAttr, &expTime);

    if (status == JXTA_SUCCESS) {
        if (timeOutForMe > expTime) {
            expTime = timeOutForMe;
        }
        status = cm_sql_update_item(groupDB, table, jNameSpace, jKey, jElemAttr, jVal, jRange, expTime);
        goto finish;
    } else if (status == JXTA_ITEM_NOTFOUND) {
        jstring_append_2(insert_sql, SQL_INSERT_INTO);
        jstring_append_2(insert_sql, table);
        jstring_append_2(insert_sql, SQL_VALUES);
    } else {
        goto finish;
    }

    jstring_append_2(insert_sql, SQL_LEFT_PAREN);
    SQL_VALUE(insert_sql, jNameSpace);
    jstring_append_2(insert_sql, SQL_COMMA);
    SQL_VALUE(insert_sql, jKey);
    jstring_append_2(insert_sql, SQL_COMMA);
    SQL_VALUE(insert_sql, jElemAttr);
    jstring_append_2(insert_sql, SQL_COMMA);
    SQL_VALUE(insert_sql, jVal);
    jstring_append_2(insert_sql, SQL_COMMA);
    SQL_NUMERIC_VALUE(insert_sql, jVal);
    jstring_append_2(insert_sql, SQL_COMMA);
    SQL_VALUE(insert_sql, jRange);
    jstring_append_2(insert_sql, SQL_COMMA);
    memset(aTime, 0, 64);

    if (apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, get_lifetime(jpr_time_now(), timeOutForMe)) != 0) {
        jstring_append_2(insert_sql, aTime);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to format time for me - set to default timeout \n");
        apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, get_lifetime(jpr_time_now(), DEFAULT_EXPIRATION));
        jstring_append_2(insert_sql, aTime);
    }

    jstring_append_2(insert_sql, SQL_COMMA);
    memset(aTime, 0, 64);

    if (apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, timeOutForOthers) != 0) {
        jstring_append_2(insert_sql, aTime);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to format time for others - set to default timeout \n");
        apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, DEFAULT_EXPIRATION);
        jstring_append_2(insert_sql, aTime);
    }

    jstring_append_2(insert_sql, SQL_RIGHT_PAREN);

    rv = apr_dbd_query(groupDB->driver, groupDB->sql, &nrows, jstring_get_string(insert_sql));

    status = (Jxta_status) rv;

  finish:
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Couldn't insert %s  rc=%i\n", groupDB->id,
                        jstring_get_string(insert_sql), rv);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s -- Saved %s  rc=%i\n", groupDB->id, jstring_get_string(insert_sql),
                        rv);
    }

    JXTA_OBJECT_RELEASE(insert_sql);
    JXTA_OBJECT_RELEASE(jKey);
    JXTA_OBJECT_RELEASE(jNameSpace);
    JXTA_OBJECT_RELEASE(jElemAttr);
    JXTA_OBJECT_RELEASE(jVal);
    if (jRange)
        JXTA_OBJECT_RELEASE(jRange);
    JXTA_OBJECT_RELEASE(jTime);
    JXTA_OBJECT_RELEASE(jTimeOthers);
    return status;
}

static Jxta_status cm_sql_delete_item(GroupDB * groupDB, const char *table, const char *advId)
{
    Jxta_status status;
    int rv = 0;
    int nrows = 0;
    JString *statement = jstring_new_0();
    JString *jAdvId = jstring_new_0();
    
    jstring_append_2(jAdvId, advId);
    jstring_append_2(statement, SQL_DELETE);
    jstring_append_2(statement, table);
    jstring_append_2(statement, SQL_WHERE);
    jstring_append_2(statement, CM_COL_AdvId);
    jstring_append_2(statement, SQL_EQUAL);
    SQL_VALUE(statement, jAdvId);

    rv = apr_dbd_query(groupDB->driver, groupDB->sql, &nrows, jstring_get_string(statement));

    status = (Jxta_status) rv;
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Couldn't delete %s  rc=%i\n", groupDB->id,
                        jstring_get_string(statement), rv);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s -- Deleted %i  %s\n", groupDB->id, nrows,
                        jstring_get_string(statement));
    }
    
    JXTA_OBJECT_RELEASE(statement);
    JXTA_OBJECT_RELEASE(jAdvId);
    return status;
}

static Jxta_status cm_sql_delete_with_where(GroupDB * groupDB, const char *table, JString * where)
{
    int rv = 0;
    int nrows = 0;
    Jxta_status status;
    JString *statement = jstring_new_0();
    
    jstring_append_2(statement, SQL_DELETE);
    jstring_append_2(statement, table);
    if (where != NULL) {
        jstring_append_1(statement, where);
    }

    rv = apr_dbd_query(groupDB->driver, groupDB->sql, &nrows, jstring_get_string(statement));

    status = (Jxta_status) rv;
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Couldn't delete rc=%i\n %s \n", groupDB->id, rv,
                        jstring_get_string(statement));
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s -- Deleted %i rows  %s\n", groupDB->id, nrows,
                        jstring_get_string(statement));
    }
    JXTA_OBJECT_RELEASE(statement);
    return status;

}
static Jxta_status cm_sql_remove_expired_records(GroupDB * groupDB, const char *folder_name)
{
    Jxta_status status = JXTA_SUCCESS;
    JString *jTime = jstring_new_0();
    JString *where = jstring_new_0();

    jstring_append_2(where, SQL_WHERE);

    cm_sql_correct_space(folder_name, where);

    jstring_append_2(where, SQL_AND);
    cm_sql_append_timeout_check_less(where);

    status = cm_sql_delete_with_where(groupDB, CM_TBL_ELEM_ATTRIBUTES, where);
    if (status != JXTA_SUCCESS)
        goto finish;
    status = cm_sql_delete_with_where(groupDB, CM_TBL_SRDI, where);
    if (status != JXTA_SUCCESS)
        goto finish;
    status = cm_sql_delete_with_where(groupDB, CM_TBL_ADVERTISEMENTS, where);
    if (status != JXTA_SUCCESS)
        goto finish;

  finish:
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Couldn't delete rc=%i\n %i \n", groupDB->id, status,
                        jstring_get_string(where));
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Deleted ALL -- %s\n", groupDB->id, jstring_get_string(where));
    }
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jTime);
    return status;
}

static Jxta_status
cm_sql_found(GroupDB * groupDB, const char *table, const char *nameSpace, const char *advId, const char *elemAttr,
             Jxta_expiration_time * exp)
{
    int rv = 0;
    Jxta_status status = JXTA_ITEM_NOTFOUND;
    JString *where = jstring_new_0();
    JString *columns = jstring_new_0();
    JString *jNameSpace = jstring_new_2(nameSpace);
    JString *jAdvId = jstring_new_2(advId);
    JString *jElemAttr = NULL;
    JString *statement = jstring_new_0();
    Jpr_absolute_time expTemp;
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row = NULL;
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    const char *entry;

    jstring_append_2(columns, CM_COL_TimeOut);

    jstring_append_2(where, CM_COL_NameSpace);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jNameSpace);
    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, CM_COL_AdvId);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jAdvId);
    if (elemAttr != NULL) {
        jElemAttr = jstring_new_2(elemAttr);
        jstring_append_2(where, SQL_AND);
        jstring_append_2(where, CM_COL_Name);
        jstring_append_2(where, SQL_EQUAL);
        SQL_VALUE(where, jElemAttr);
    }

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", groupDB->id, aprs);
        status = JXTA_FAILED;
        goto finish;
    }

    rv = cm_sql_select(groupDB, pool, table, &res, columns, where);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- cm_sql_found Select failed: %s %i\n",
                        groupDB->id, apr_dbd_error(groupDB->driver, groupDB->sql, rv), rv);
        status = JXTA_FAILED;
        goto finish;
    }
    for (rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1);
         rv == 0; rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1)) {
        entry = apr_dbd_get_entry(groupDB->driver, row, 0);
        if (entry == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s -- (null) entry in cm_sql_found\n", groupDB->id);
        } else {
            sscanf(entry, jstring_get_string(jInt64_for_format), &expTemp);
            if (expTemp < jpr_time_now()) {
                *exp = 0;
            } else {
                *exp = (expTemp - jpr_time_now());
            }
        }
        status = JXTA_SUCCESS;
    }
  finish:
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(statement);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(jNameSpace);
    JXTA_OBJECT_RELEASE(jAdvId);
    if (jElemAttr != NULL) {
        JXTA_OBJECT_RELEASE(jElemAttr);
    }
    JXTA_OBJECT_RELEASE(where);
    return status;

}
static Jxta_status
cm_sql_srdi_found(GroupDB * groupDB, const char *table, JString * NameSpace, JString * Handler, JString * Peerid, JString * Key,
                  JString * Attribute)
{
    int rv = 0;
    int n;
    Jxta_status status = JXTA_ITEM_NOTFOUND;
    JString *where = jstring_new_0();
    JString *columns = jstring_new_0();
    JString *jNameSpace = jstring_clone(NameSpace);
    JString *jHandler = jstring_clone(Handler);
    JString *jPeerid = jstring_clone(Peerid);
    JString *jAdvId = jstring_clone(Key);
    JString *jElemAttr = jstring_clone(Attribute);
    JString *statement = jstring_new_0();
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row = NULL;
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    const char *entry;

    jstring_append_2(columns, CM_COL_AdvId);

    jstring_append_2(where, CM_COL_NameSpace);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jNameSpace);
    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, CM_COL_Handler);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jHandler);
    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, CM_COL_Peerid);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jPeerid);
    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, CM_COL_AdvId);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jAdvId);
    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, CM_COL_Name);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jElemAttr);

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", groupDB->id, aprs);
        status = JXTA_FAILED;
        goto finish;
    }

    jstring_append_2(where, SQL_AND);
    cm_sql_append_timeout_check_greater(where);

    rv = cm_sql_select(groupDB, pool, table, &res, columns, where);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- cm_sql_srdi_found Select failed: %s %i\n",
                        groupDB->id, apr_dbd_error(groupDB->driver, groupDB->sql, rv), rv);
        status = JXTA_FAILED;
        goto finish;
    }
    for (rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1);
         rv == 0; rv = apr_dbd_get_row(groupDB->driver, pool, res, &row, -1)) {
        int column_count = apr_dbd_num_cols(groupDB->driver, res);
        status = JXTA_SUCCESS;
        for (n = 0; n < column_count; ++n) {
            entry = apr_dbd_get_entry(groupDB->driver, row, n);
            if (entry == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s -- (null) entry in cm_sql_found\n", groupDB->id);
            }
        }
    }
  finish:
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(statement);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(jNameSpace);
    JXTA_OBJECT_RELEASE(jHandler);
    JXTA_OBJECT_RELEASE(jPeerid);
    JXTA_OBJECT_RELEASE(jAdvId);
    JXTA_OBJECT_RELEASE(jElemAttr);
    JXTA_OBJECT_RELEASE(where);
    return status;
}

static Jxta_status
cm_sql_select(GroupDB * groupDB, apr_pool_t * pool, const char *table, apr_dbd_results_t ** res, JString * columns,
              JString * where)
{
    int rv = 0;
    Jxta_status status;
    JString *statement = jstring_new_0();

    jstring_append_2(statement, SQL_SELECT);

    if (columns != NULL) {
        jstring_append_1(statement, columns);
    } else {
        jstring_append_2(statement, " * ");
    }

    jstring_append_2(statement, SQL_FROM);
    jstring_append_2(statement, table);

    if (where != NULL && jstring_length(where) > 0) {
        jstring_append_2(statement, SQL_WHERE);
        jstring_append_1(statement, where);
    }

    rv = apr_dbd_select(groupDB->driver, pool, groupDB->sql, res, jstring_get_string(statement), 0);
    if (rv == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s -- %i tuples %s\n", groupDB->id,
                        apr_dbd_num_tuples(groupDB->driver, *res), jstring_get_string(statement));
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- error cm_sql_select %i   %s\n", groupDB->id, rv,
                        jstring_get_string(statement));
    }

    status = rv;
    JXTA_OBJECT_RELEASE(statement);
    return status;
}

static void cm_sql_pragma(GroupDB * groupDB, const char *pragma)
{
    int ret;
    int nrows;
    JString *cmd;

    cmd = jstring_new_0();

    jstring_append_2(cmd, SQL_PRAGMA);
    jstring_append_2(cmd, pragma);

    ret = apr_dbd_query(groupDB->driver, groupDB->sql, &nrows, jstring_get_string(cmd));

    JXTA_OBJECT_RELEASE(cmd);
    return;
}
static Jxta_status cm_sql_select_join(GroupDB * groupDB, apr_pool_t * pool, apr_dbd_results_t ** res, JString * where)
{
    int rv = 0;
    Jxta_status status;
    JString *statement = jstring_new_0();

    jstring_append_2(statement, SQL_SELECT);
    jstring_append_2(statement, CM_COL_SRC);
    jstring_append_2(statement, SQL_DOT);
    jstring_append_2(statement, CM_COL_AdvId);
    jstring_append_2(statement, SQL_COMMA);
    jstring_append_2(statement, CM_COL_Advert);
    jstring_append_2(statement, SQL_FROM);
    jstring_append_2(statement, CM_TBL_ELEM_ATTRIBUTES_SRC);
    jstring_append_2(statement, SQL_JOIN);
    jstring_append_2(statement, CM_TBL_ADVERTISEMENTS_JOIN);
    jstring_append_2(statement, SQL_ON);
    jstring_append_2(statement, CM_COL_SRC);
    jstring_append_2(statement, SQL_DOT);
    jstring_append_2(statement, CM_COL_AdvId);
    jstring_append_2(statement, SQL_EQUAL);
    jstring_append_2(statement, CM_COL_JOIN);
    jstring_append_2(statement, SQL_DOT);
    jstring_append_2(statement, CM_COL_AdvId);

    jstring_append_2(statement, SQL_WHERE);

    if (where != NULL && jstring_length(where) > 0) {
        jstring_append_1(statement, where);
        jstring_append_2(statement, SQL_AND);
    }

    jstring_append_2(statement, CM_COL_SRC);
    jstring_append_2(statement, SQL_DOT);
    cm_sql_append_timeout_check_greater(statement);

    jstring_append_2(statement, SQL_GROUP);
    jstring_append_2(statement, CM_COL_Advert);
    jstring_append_2(statement, SQL_END_SEMI);

    rv = apr_dbd_select(groupDB->driver, pool, groupDB->sql, res, jstring_get_string(statement), 0);
    if (rv == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s -- %i tuples %s\n", groupDB->id,
                        apr_dbd_num_tuples(groupDB->driver, *res), jstring_get_string(statement));
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- error cm_sql_select %i   %s\n", groupDB->id, rv,
                        jstring_get_string(statement));
    }

    status = rv;
    JXTA_OBJECT_RELEASE(statement);
    return status;
}

static void cm_sql_correct_space(const char *folder_name, JString * where)
{
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "looking for space %s\n", folder_name);

    if (!strcmp(folder_name, "Peers")) {
        jstring_append_2(where, CM_COL_NameSpace);
        jstring_append_2(where, SQL_EQUAL);
        SQL_VALUE(where, jPeerNs);
    } else if (!strcmp(folder_name, "Groups")) {
        jstring_append_2(where, CM_COL_NameSpace);
        jstring_append_2(where, SQL_EQUAL);
        SQL_VALUE(where, jGroupNs);
    } else {
        jstring_append_2(where, CM_COL_NameSpace);
        jstring_append_2(where, SQL_NOT_EQUAL);
        SQL_VALUE(where, jGroupNs);
        jstring_append_2(where, SQL_AND);
        jstring_append_2(where, CM_COL_NameSpace);
        jstring_append_2(where, SQL_NOT_EQUAL);
        SQL_VALUE(where, jPeerNs);
    }
}

static void cm_sql_append_timeout_check_greater(JString * cmd)
{
    char aTime[64];
    Jxta_expiration_time tNow;

    jstring_append_2(cmd, CM_COL_TimeOut);
    jstring_append_2(cmd, SQL_GREATER_THAN);
    memset(aTime, 0, 64);
    tNow = jpr_time_now();

    if (apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, tNow) != 0) {
        jstring_append_2(cmd, aTime);
    } else {
        jstring_append_2(cmd, "0");
    }
}

static void cm_sql_append_timeout_check_less(JString * cmd)
{
    char aTime[64];
    Jxta_expiration_time tNow;

    jstring_append_2(cmd, CM_COL_TimeOut);
    jstring_append_2(cmd, SQL_LESS_THAN);
    memset(aTime, 0, 64);
    tNow = jpr_time_now();

    if (apr_snprintf(aTime, 64, "%"APR_INT64_T_FMT, tNow) != 0) {
        jstring_append_2(cmd, aTime);
    } else {
        jstring_append_2(cmd, "0");
    }
}

JXTA_DECLARE(Jxta_boolean) jxta_sql_escape_and_wc_value(JString * jStr, Jxta_boolean replace)
{
    Jxta_boolean wc = FALSE;
    Jxta_boolean change = FALSE;
    char *bufpt;
    int i, j, n, c;
    char *arg;

    if (NULL == jStr)
        return FALSE;

    if (jstring_length(jStr) == 0)
        return FALSE;

    arg = (char *) jstring_get_string(jStr);
    for (i = n = 0; (c = arg[i]) != 0; i++) {
        if (c == '\'') {
            change = TRUE;
            n++;
        }
        if ((c == '*') || (c == '?'))
            wc = TRUE;
    }

    if (replace == TRUE && wc == TRUE)
        change = TRUE;
    if (change == FALSE)
        return FALSE;
    n += i;
    bufpt = calloc(1, n + 1);
    if (bufpt == 0)
        return -1;
    j = 0;
    for (i = 0; (c = arg[i]) != 0; i++) {
        bufpt[j++] = c;
        if (replace) {
            if (c == '*')
                bufpt[j - 1] = '%';
            if (c == '?')
                bufpt[j - 1] = '_';
        }
        if (c == '\'')
            bufpt[j++] = c;
    }

    bufpt[j] = 0;
    jstring_reset(jStr, NULL);
    jstring_append_2(jStr, bufpt);
    free(bufpt);

    return wc;
}

JXTA_DECLARE(void) jxta_sql_numeric_quote(JString * jDest, JString * jStr, Jxta_boolean isNumeric)
{
    const char *str;
    
    if (NULL == jStr) {
        jstring_append_2(jDest, "''");
        return;
    }

    str = jstring_get_string(jStr);
    if ('#' == *str) {
        if (isNumeric) {
            jstring_append_2(jDest, str + 1);
        } else {
            jstring_append_2(jDest, "'");
            jstring_append_1(jDest, jStr);
            jstring_append_2(jDest, "'");
        }
        return;
    }

    if (!isNumeric) {
        jstring_append_2(jDest, "'");
        jstring_append_1(jDest, jStr);
        jstring_append_2(jDest, "'");
    } else {
        jstring_append_2(jDest, "''");
    }

}

/* vi: set ts=4 sw=4 tw=130 et: */
