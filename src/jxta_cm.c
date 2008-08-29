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

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>

#include "jxta_apr.h"
#include <apu.h>
#include <apr_dbd.h>

#ifdef WIN32
#include <direct.h> /* for _mkdir */
#else
#include "config.h"
#endif

/* #include <apr_dbm.h> */
#include <sqlite3.h>

#include "jxta_cred.h"
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_object.h"
#include "jxta_object_priv.h"
#include "jxta_object_type.h"
#include "jxta_hashtable.h"
#include "jxta_advertisement.h"
#include "jxta_pa.h"
#include "jxta_discovery_service.h"
#include "jstring.h"
#include "jxta_id.h"
#include "jxta_cm.h"
#include "jxta_cm_private.h"
#include "jxta_srdi.h"
#include "jxta_log.h"
#include "jxta_sql.h"
#include "jxta_proffer.h"
#include "jxta_range.h"

#ifdef WIN32
#define DBTYPE "SDBM"   /* defined for other platform in config.h */
#endif

static const char *__log_cat = "CM";
static Jxta_vector *global_cm_registry = NULL;

typedef struct {
    JXTA_OBJECT_HANDLE;
    Jxta_hashtable *secondary_keys;
    char *name;
} Folder;

typedef struct {
    JXTA_OBJECT_HANDLE;
    apr_thread_mutex_t *lock;
    apr_dbd_t *sql;
    const apr_dbd_driver_t *driver;
    apr_dbd_transaction_t *transaction;
    apr_pool_t *pool;
    char *name;
    int log_id;
} DBConnection;

static struct adv_entry {
    JXTA_OBJECT_HANDLE;
    JString *advid;
    Jxta_boolean replica;
    Jxta_expiration_time timeout;
} adv_entry;

/* 
 * This is a folder definition that is required for the SQL backend
 * using the apr_dbd.h generic SQL database API.
*/
typedef struct {
    JXTA_OBJECT_HANDLE;
    DBConnection *conn;
    char *alias;
    char *name;
    char *id;
    JString *jId;
    Jxta_hashtable *secondary_keys;
    apr_thread_pool_t *thread_pool;
    Jxta_addressSpace *jas;
    Jxta_boolean available;
    int xaction_threshold;
} DBSpace;

struct _jxta_cm {
    Extends(Jxta_object);
    char *root;
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
    Jxta_hashtable *folders;
    Jxta_hashtable *srdi_delta;
    Jxta_hashtable *db_connections;
    Jxta_cm *parent;
    Jxta_boolean sharedDB;
    volatile Jxta_boolean record_delta;
    Jxta_PA *config_adv;
    Jxta_PID *localPeerId;
    Jxta_vector *dbSpaces;
    Jxta_hashtable *resolvedAdvs;
    Jxta_CacheConfigAdvertisement *cacheConfig;
    Jxta_vector *addressSpaces;
    Jxta_vector *nameSpaces;
    JString *jGroupID_string;
    DBSpace *defaultDB;
    DBSpace *bestChoiceDB;
    apr_thread_pool_t *thread_pool;
    volatile Jxta_boolean available;
    volatile Jxta_sequence_number delta_seq_number;
    int log_id;
};

typedef struct resolved_adv_type {
    JXTA_OBJECT_HANDLE;
    char *adv;
    DBSpace *dbSpace;
} Jxta_resolved_adv_type;

typedef struct cm_srdi_transaction_task {
    Jxta_cm *self;
    DBSpace *dbSpace;
    Jxta_vector *entriesV;
    JString *handler;
    JString *peerid;
    JString *src_peerid;
    JString *primaryKey;
    Jxta_boolean bReplica;
} Jxta_cm_srdi_task;

static JString *jPeerNs;
static JString *jGroupNs;
static JString *jInt64_for_format;
static JString *fmtTime;
static unsigned int _cm_params_initialized = 0;
static int in_memories = 0;
static int log_id = 0;

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
    {CM_COL_GroupID, SQL_VARCHAR_128},
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
    {CM_COL_OriginalTimeout, SQL_VARCHAR_64},
    {CM_COL_TimeOutForOthers, SQL_VARCHAR_64},
    {CM_COL_GroupID, SQL_VARCHAR_128},
    {CM_COL_DupPeerid, SQL_VARCHAR_128},
    {NULL, NULL}
};

/*
 * table for index into SRDI table
*/
static const char *jxta_srdiIndexTable = CM_TBL_SRDI_INDEX;
static const char *jxta_srdiIndexTable_fields[][2] = {
    {CM_COL_DBAlias, SQL_VARCHAR_64},
    {CM_COL_NameSpace, SQL_VARCHAR_128},
    {CM_COL_Peerid, SQL_VARCHAR_128},
    {CM_COL_GroupID, SQL_VARCHAR_128},
    {CM_COL_AdvId, SQL_VARCHAR_128},
    {CM_COL_Name, SQL_VARCHAR_64},
    {CM_COL_SeqNumber, SQL_VARCHAR_64},
    {CM_COL_TimeOut, SQL_VARCHAR_64},
    {CM_COL_OriginalTimeout, SQL_VARCHAR_64},
    {CM_COL_Replica, SQL_INTEGER},
    {CM_COL_Replicate, SQL_INTEGER},
    {CM_COL_SourcePeerid, SQL_VARCHAR_128},
    {NULL, NULL}
};

/*
 * table used for SRDI deltas
*/
static const char *jxta_srdiDeltaTable = CM_TBL_SRDI_DELTA;
static const char *jxta_srdiDeltaTable_fields[][2] = {
    {CM_COL_NameSpace, SQL_VARCHAR_128},
    {CM_COL_Handler, SQL_VARCHAR_64},
    {CM_COL_Peerid, SQL_VARCHAR_128},
    {CM_COL_AdvId, SQL_VARCHAR_128},
    {CM_COL_Name, SQL_VARCHAR_64},
    {CM_COL_Value, SQL_VARCHAR_4K},
    {CM_COL_NumRange, SQL_VARCHAR_64},
    {CM_COL_TimeOut, SQL_VARCHAR_64},
    {CM_COL_TimeOutForOthers, SQL_VARCHAR_64},
    {CM_COL_GroupID, SQL_VARCHAR_128},
    {CM_COL_SeqNumber, SQL_VARCHAR_64},
    {CM_COL_NextUpdate, SQL_VARCHAR_64},
    {CM_COL_DeltaWindow, SQL_INTEGER},
    {CM_COL_SourcePeerid, SQL_VARCHAR_128},
    {CM_COL_Duplicate, SQL_INTEGER},
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
    {CM_COL_OriginalTimeout, SQL_VARCHAR_64},
    {CM_COL_TimeOutForOthers, SQL_VARCHAR_64},
    {CM_COL_GroupID, SQL_VARCHAR_128},
    {CM_COL_DupPeerid, SQL_VARCHAR_128},
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
    {CM_COL_GroupID, SQL_VARCHAR_128},
    {NULL, NULL}
};

/*=============== AddressSpace/Namespace functions ==============================*/

static Jxta_status cm_address_spaces_process(Jxta_cm * self);

static Jxta_status cm_address_space_conn_get(Jxta_cm * me, Jxta_addressSpace * jas, DBConnection ** conn);

static Jxta_status cm_name_space_register(Jxta_cm * me, DBSpace * dbSpace, const char *ns);

static Jxta_status cm_name_spaces_process(Jxta_cm * self);

static Jxta_status cm_name_spaces_populate(Jxta_cm * self);

static Jxta_status cm_SQLite_parms_process(Jxta_cm * self, DBSpace * dbSpace, Jxta_addressSpace * jas);

static DBSpace *cm_dbSpace_get(Jxta_cm * self, const char *pAddressSpace);

static DBSpace *cm_dbSpace_by_alias_get(Jxta_cm * me, const char *dbAlias);

static DBSpace **cm_dbSpaces_get(Jxta_cm * self, const char *pAdvType, Jxta_credential ** scope, Jxta_boolean bestChoice);

static Jxta_status cm_dbSpaces_find(Jxta_cm * self, JString * jName, Jxta_boolean * nsWildCardMatch,
                                    Jxta_hashtable ** dbsReturned, DBSpace ** wildCardDB);

/*================ SQL Database functions =============================*/

static apr_status_t cm_sql_db_init(DBSpace * dbSpace, Jxta_addressSpace * jas);

static apr_status_t cm_sql_table_create(DBSpace * dbSpace, const char *table, const char *columns[]);

static Jxta_status cm_sql_table_found(DBSpace * dbSpace, const char *table);

#ifdef UNUSED_VWF
static int cm_sql_table_drop(DBSpace * dbSpace, char *table);
#endif

static Jxta_status cm_sql_index_found(DBSpace * dbSpace, const char *iindex);

static int cm_sql_index_create(DBSpace * dbSpace, const char *table, const char **fields);

/*================ SQL Command functions =============================*/

static Jxta_status cm_sql_select(DBSpace * dbSpace, apr_pool_t * pool, const char *table, apr_dbd_results_t ** res,
                                 JString * columns, JString * where, JString * sort, Jxta_boolean bCheckGroup);

static Jxta_status cm_sql_select_join(DBSpace * dbSpace, apr_pool_t * pool, apr_dbd_results_t ** res, JString * where);

static Jxta_status cm_sql_delete_with_where(DBSpace * dbSpace, const char *table, JString * where);

static Jxta_status cm_sql_update_with_where(DBSpace * dbSpace, const char *table, JString * columns, JString * where);

static void cm_sql_correct_space(const char *folder_name, JString * where);

static void cm_sql_append_timeout_check_greater(JString * cmd);

static void cm_sql_append_timeout_check_less(JString * cmd);

static void cm_sql_pragma(DBSpace * dbSpace, const char *pragma);

static Jxta_status cm_sql_delta_entry_update(DBSpace * dbSpace, JString * jPeerid, Jxta_SRDIEntryElement * entry,
                                         JString * where, Jpr_interval_time next_update);

/*==================== CM entry functions ===========================*/

static Jxta_cm *cm_new_priv(Jxta_cm * cm, const char *home_directory, Jxta_id * group_id,
                            Jxta_CacheConfigAdvertisement * conf_adv, Jxta_PID * localPeerId, apr_thread_pool_t * thread_pool);

static Jxta_status cm_advertisement_save(DBSpace * dbSpace, const char *key, Jxta_advertisement * adv,
                                         Jxta_expiration_time timeOutForMe, Jxta_expiration_time timeOutForOthers,
                                         Jxta_boolean update);

static Jxta_status cm_advertisement_update(DBSpace * dbSpace, JString * jNameSpace, JString * jKey, JString * jXml,
                                           Jxta_expiration_time timeOutForMe, Jxta_expiration_time timeOutForOthers);
#ifdef UNUSED_VWF
static Jxta_status cm_advertisement_remove(DBSpace * dbSpace, const char *nameSpace, const char *key);
#endif
static Jxta_cache_entry **cm_proffer_advertisements_build(DBSpace * dbSpace, apr_pool_t * pool, apr_dbd_results_t * res,
                                                          Jxta_boolean adv_present);

static Jxta_cache_entry **cm_advertisements_build(DBSpace * dbSpace, apr_pool_t * pool, apr_dbd_results_t * res);

static void cm_proffer_advertisements_columns_build(JString * columns);

static Jxta_status cm_item_insert(DBSpace * dbSpace, const char *table, const char *nameSpace, const char *key,
                                  const char *elemAttr, const char *val, const char *range, Jxta_expiration_time timeOutForMe,
                                  Jxta_expiration_time timeOutForOthers);

static Jxta_status cm_sql_select_join_generic(DBSpace * dbSpace, apr_pool_t * pool, JString * jJoin, JString * jSort,
                                              apr_dbd_results_t ** res, JString * where, Jxta_boolean bCheckGroup);

static Jxta_status cm_item_update(DBSpace * dbSpace, const char *table, JString * jNameSpace, JString * jKey,
                                  JString * jElemAttr, JString * jVal, JString * jRange, Jxta_expiration_time timeOutForMe,
                                  Jxta_expiration_time timeOutForOthers);

static Jxta_status cm_item_delete(DBSpace * dbSpace, const char *table, const char *advId);


static Jxta_status cm_item_found(DBSpace * dbSpace, const char *table, const char *nameSpace, const char *advId,
                                 const char *elemAttr, Jxta_expiration_time * timeout, Jxta_expiration_time * exp);

static Jxta_status cm_expired_records_remove(DBSpace * dbSpace, const char *folder_name, Jxta_boolean all);

static Jxta_status cm_get_time(Jxta_cm * self, const char *folder_name, const char *primary_key, Jxta_expiration_time * time,
                               const char *time_name);
/*===================== SRDI/Replica functions ==========================*/

static Jxta_status cm_srdi_transaction_save(Jxta_cm_srdi_task * task_parms);

static Jxta_status cm_srdi_element_save(DBSpace * dbSpace, JString * Handler, JString * Peerid, Jxta_SRDIEntryElement * entry,
                                        Jxta_boolean replica);

static Jxta_status cm_srdi_item_found(DBSpace * dbSpace, const char *table, JString * NameSpace, JString * Handler,
                                      JString * Peerid, JString * Key, JString * Attribute);

static Jxta_status cm_srdi_item_update(DBSpace * dbSpace, const char *table, JString * jHandler, JString * jPeerid,
                                       JString * jKey, JString * jAttribute, JString * jValue, JString * jRange,
                                       Jxta_expiration_time timeOutForMe);

static Jxta_status cm_srdi_index_save(Jxta_cm * me, const char *alias, JString * jPeerid, JString *jSrcPeerid, Jxta_SRDIEntryElement * entry,
                                      Jxta_boolean bReplica);

static void cm_delta_entry_create(JString * jGroupID, JString * dbAlias, JString * jPeerid, JString * jSourcePeerid, JString * jHandler, JString * sqlval,
                                  Jxta_SRDIEntryElement * entry, int delta_window);

static void cm_srdi_delta_add(Jxta_hashtable * srdi_delta, Folder * folder, Jxta_SRDIEntryElement * entry);

static Jxta_status cm_srdi_seq_number_update(Jxta_cm * me, JString * jPeerid, Jxta_SRDIEntryElement * entry);

static Jxta_vector *cm_get_srdi_entries_priv(Jxta_cm *self, const char *table_name, JString *folder_name, JString *pAdvid, JString *pPeerId);

/*================= Best Choice functions ==============================*/

static void cm_best_choice_select(Jxta_cm * self);

static apr_status_t cm_best_choice_tables_create(Jxta_cm * me);

/*=============== Miscellaneous functions ================================*/

static Jxta_object **cm_arrays_concatenate(Jxta_object ** destination, Jxta_object ** source);

static Jpr_interval_time lifetime_get(Jpr_interval_time base, Jpr_interval_time span);

static void cm_prefix_split_type_and_check_wc(const char *pAdvType, char prefix[], const char **type, Jxta_boolean * wildcard,
                                              Jxta_boolean * wildCardPrefix);

static JString ** tokenize_string_j(JString *string_j, char separator);

static void initiate_cm_params()
{
    if (_cm_params_initialized++) {
        return;
    }

    jPeerNs = jstring_new_2("jxta:PA");
    jGroupNs = jstring_new_2("jxta:PGA");

    /* this is for formatted strings */
    jInt64_for_format = jstring_new_2("%");
    fmtTime = jstring_new_2("exp Time ");
    jstring_append_2(jInt64_for_format, APR_INT64_T_FMT);
    jstring_append_1(fmtTime, jInt64_for_format);
    jstring_append_2(fmtTime, "\n");
}

static void terminate_cm_params()
{
    if (!_cm_params_initialized) {
        return;
    }

    _cm_params_initialized--;
    if (_cm_params_initialized) {
        return;
    }

    JXTA_OBJECT_RELEASE(jPeerNs);
    JXTA_OBJECT_RELEASE(jGroupNs);
    JXTA_OBJECT_RELEASE(jInt64_for_format);
    JXTA_OBJECT_RELEASE(fmtTime);
}

/* make the CM unavailable and cancel any pending tasks */
void cm_stop(Jxta_cm * me)
{
    int i;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "CM is being stopped\n");
    me->available = FALSE;
    if (me->thread_pool) {
        apr_thread_pool_tasks_cancel(me->thread_pool, me);
    }
    for (i = 0; i < jxta_vector_size(global_cm_registry); i++) {
        Jxta_cm *tmp_cm;
        jxta_vector_get_object_at(global_cm_registry, JXTA_OBJECT_PPTR(&tmp_cm), i);
        if (tmp_cm == me) {
            jxta_vector_remove_object_at(global_cm_registry, NULL, i);
        }
        JXTA_OBJECT_RELEASE(tmp_cm);
    }
    if (jxta_vector_size(global_cm_registry) == 0) {
        JXTA_OBJECT_RELEASE(global_cm_registry);
    }
}

/* Destroy the cm and free all allocated memory */
static void cm_free(Jxta_object * cm)
{
    Jxta_cm *me = (Jxta_cm *) cm;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "CM free\n");
    assert(!me->available);
    apr_thread_mutex_lock(me->mutex);
    terminate_cm_params();

    if (me->folders)
        JXTA_OBJECT_RELEASE(me->folders);
    if (me->addressSpaces)
        JXTA_OBJECT_RELEASE(me->addressSpaces);
    if (me->nameSpaces)
        JXTA_OBJECT_RELEASE(me->nameSpaces);
    if (me->db_connections)
        JXTA_OBJECT_RELEASE(me->db_connections);
    if (me->config_adv)
        JXTA_OBJECT_RELEASE(me->config_adv);
    if (me->cacheConfig)
        JXTA_OBJECT_RELEASE(me->cacheConfig);
    if (me->resolvedAdvs)
        JXTA_OBJECT_RELEASE(me->resolvedAdvs);
    if (me->jGroupID_string)
        JXTA_OBJECT_RELEASE(me->jGroupID_string);
    if (me->srdi_delta)
        JXTA_OBJECT_RELEASE(me->srdi_delta);
    if (me->root)
        free(me->root);
    if (me->defaultDB)
        JXTA_OBJECT_RELEASE(me->defaultDB);
    if (me->bestChoiceDB)
        JXTA_OBJECT_RELEASE(me->bestChoiceDB);
    if (me->dbSpaces) {
        JXTA_OBJECT_RELEASE(me->dbSpaces);
    }
    if (me->parent) {
        JXTA_OBJECT_RELEASE(me->parent);
    }
    if (me->localPeerId)
        JXTA_OBJECT_RELEASE(me->localPeerId);
    apr_thread_mutex_unlock(me->mutex);
    apr_thread_mutex_destroy(me->mutex);
    apr_pool_destroy(me->pool);

    /* Free the object itself */
    free(me);
}

static void cache_entry_free(Jxta_object * obj)
{
    Jxta_cache_entry *ce;
    ce = (Jxta_cache_entry *) obj;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "cache entry free\n");
    if (ce->profadv)
        JXTA_OBJECT_RELEASE(ce->profadv);
    if (ce->adv)
        JXTA_OBJECT_RELEASE(ce->adv);
    if (ce->profid)
        JXTA_OBJECT_RELEASE(ce->profid);
    free(ce);
}

static void dbConnection_free(Jxta_object * obj)
{
    DBConnection *conn = (DBConnection *) obj;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "dbConnection_free\n");
    if (conn->lock)
        apr_thread_mutex_lock(conn->lock);
    if (conn->sql) {
        apr_dbd_close(conn->driver, conn->sql);
    }
    if (conn->name)
        free(conn->name);
    if (conn->lock) {
        apr_thread_mutex_unlock(conn->lock);
        apr_thread_mutex_destroy(conn->lock);
    }
    free(conn);
}

static void dbSpace_free(Jxta_object * obj)
{
    DBSpace *gdb = (DBSpace *) obj;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "DBSpace free - %s\n", gdb->alias);
    gdb->available = FALSE;
    if (gdb->alias) {
        free(gdb->alias);
    }
    if (gdb->name != NULL)
        free(gdb->name);
    if (gdb->id != NULL)
        free(gdb->id);
    if (gdb->secondary_keys != NULL)
        JXTA_OBJECT_RELEASE(gdb->secondary_keys);
    if (gdb->jas != NULL)
        JXTA_OBJECT_RELEASE(gdb->jas);
    if (gdb->jId)
        JXTA_OBJECT_RELEASE(gdb->jId);
    if (gdb->conn)
        JXTA_OBJECT_RELEASE(gdb->conn);
    free(gdb);
}

static void advType_free(Jxta_object * obj)
{
    Jxta_resolved_adv_type *advType = (Jxta_resolved_adv_type *) obj;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "free adv type  -- %s\n", advType->adv);
    JXTA_OBJECT_RELEASE(advType->dbSpace);
    free(advType->adv);
    free(advType);
}

Jxta_cm *cm_shared_DB_new(Jxta_cm * cm, Jxta_id * group_id)
{
    Jxta_cm *cm_res;
    assert(NULL != cm);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Shared DB with %s\n", jstring_get_string(cm->jGroupID_string));
    cm_res = cm_new_priv(cm, NULL, group_id, cm->cacheConfig, cm->localPeerId, cm->thread_pool);
    cm_res->sharedDB = TRUE;
    /* the parent CM should be modified */
    cm->sharedDB = TRUE;
    return cm_res;
}

void entry_free(Jxta_object * obj)
{
    struct adv_entry *entry = (struct adv_entry *) obj;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "adv_entry free [%pp]\n", entry);
    JXTA_OBJECT_RELEASE(entry->advid);
    free(obj);
}

void replica_entry_free(Jxta_object * obj)
{
    Jxta_replica_entry * entry = (Jxta_replica_entry *) obj;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "replica_entry free [%pp]\n", entry);
    JXTA_OBJECT_RELEASE(entry->adv_id);
    JXTA_OBJECT_RELEASE(entry->peer_id);
    JXTA_OBJECT_RELEASE(entry->db_alias);
    JXTA_OBJECT_RELEASE(entry->dup_id);
    JXTA_OBJECT_RELEASE(entry->name);
    free(obj);
}

/* Create a new Cm Structure */
Jxta_cm *cm_new(const char *home_directory, Jxta_id * group_id,
                Jxta_CacheConfigAdvertisement * conf_adv, Jxta_PID * localPeerId, apr_thread_pool_t * thread_pool)
{
    Jxta_cm *cm_res;
    cm_res = cm_new_priv(NULL, home_directory, group_id, conf_adv, localPeerId, thread_pool);
    /* if any cm's are created and inherit this cm sharedDB will be changed to TRUE at that time */
    cm_res->sharedDB = FALSE;
    return cm_res;
}

Jxta_cm *cm_new_priv(Jxta_cm * cm, const char *home_directory, Jxta_id * group_id,
                     Jxta_CacheConfigAdvertisement * conf_adv, Jxta_PID * localPeerId, apr_thread_pool_t * thread_pool)
{
    apr_status_t res;
    Jxta_status status;
    Jxta_cm *self;
    int root_len;
    int thd_max;
    self = (Jxta_cm *) calloc(1, sizeof(Jxta_cm));
    JXTA_OBJECT_INIT(self, cm_free, 0);
    self->thisType = "Jxta_cm";

    initiate_cm_params();
    res = apr_pool_create(&self->pool, NULL);
    if (res != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create apr_pool: %d\n", res);
        /* Free the memory that has been already allocated */
        free(self);
        return NULL;
    }

    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, self->pool);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(self->pool);
        free(self);
        return NULL;
    }

    self->cacheConfig = JXTA_OBJECT_SHARE(conf_adv);
    jxta_id_get_uniqueportion(group_id, &self->jGroupID_string);
    self->dbSpaces = jxta_vector_new(5);
    self->addressSpaces = jxta_cache_config_get_addressSpaces(self->cacheConfig);
    self->nameSpaces = jxta_cache_config_get_nameSpaces(self->cacheConfig);
    self->resolvedAdvs = jxta_hashtable_new(0);
    self->db_connections = jxta_hashtable_new(0);

    if (NULL != cm) {
        self->parent = JXTA_OBJECT_SHARE(cm);
    }
    if (NULL == thread_pool) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "No Thread pool for the CM\n");
    }

    /* If there is an apr api for this, maybe we should use it */
    if (NULL != self->parent) {
        self->root = calloc(1, strlen(self->parent->root) + 1);
        memcpy(self->root, self->parent->root, strlen(self->parent->root) + 1);
    } else {
        root_len = strlen(home_directory) + jstring_length(self->jGroupID_string) + 2;
        self->root = calloc(1, root_len);
#ifdef WIN32
        _mkdir(home_directory);
        apr_snprintf(self->root, root_len, "%s\\", home_directory);
#else
        mkdir(home_directory, 0755);
        apr_snprintf(self->root, root_len, "%s/", home_directory);
#endif
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "CM home_directory: %s\n", self->root);
    status = cm_address_spaces_process(self);
    if (JXTA_SUCCESS != status) {
        JXTA_OBJECT_RELEASE(self);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error processing address spaces -- %i\n", status);
        return NULL;
    }

    self->thread_pool = thread_pool;
    /* adjust thread max as needed, this is a temporary hack before we have a solution to configure PG */
    if (thread_pool) {
        thd_max = apr_thread_pool_thread_max_get(thread_pool);
        if (cache_config_threads_maximum(self->cacheConfig) > thd_max) {
            thd_max += cache_config_threads_needed(self->cacheConfig);
            thd_max = (thd_max <= cache_config_threads_maximum(self->cacheConfig)) ?
                thd_max : cache_config_threads_maximum(self->cacheConfig);
            apr_thread_pool_thread_max_set(thread_pool, thd_max);
        }
    }

    self->folders = jxta_hashtable_new_0(3, FALSE);
    self->srdi_delta = jxta_hashtable_new_0(3, TRUE);
    self->record_delta = TRUE;
    self->localPeerId = JXTA_OBJECT_SHARE(localPeerId);

    status = cm_name_spaces_process(self);
    if (JXTA_SUCCESS != status) {
        JXTA_OBJECT_RELEASE(self);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error processing name spaces -- %i\n", status);
        return NULL;
    }

    status = cm_name_spaces_populate(self);
    if (JXTA_SUCCESS != status) {
        JXTA_OBJECT_RELEASE(self);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error filling in name spaces -- %i\n", status);
        return NULL;
    }

    if (NULL == self->parent) {
        cm_best_choice_select(self);
        res = cm_best_choice_tables_create(self);
    } else {
        self->bestChoiceDB = JXTA_OBJECT_SHARE(self->parent->bestChoiceDB);
    }

    self->available = TRUE;
    if (APR_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create Best Choice Table - Using Default table - res:%d\n",
                        res);
        if (NULL != self->defaultDB) {
            self->bestChoiceDB = JXTA_OBJECT_SHARE(self->defaultDB);
        } else {
            self->available = FALSE;
        }
    }
    if (NULL == global_cm_registry) {
        global_cm_registry = jxta_vector_new(0);
    }
    if (NULL != self) {
        jxta_vector_add_object_last(global_cm_registry, (Jxta_object *) self);
    }
    return self;
}

static Jxta_status cm_name_space_register(Jxta_cm * me, DBSpace * dbSpace, const char *ns)
{
    Jxta_resolved_adv_type *resAdvType = NULL;
    Jxta_status status;
    status = jxta_hashtable_get(me->resolvedAdvs, ns, strlen(ns), JXTA_OBJECT_PPTR(&resAdvType));
    if (JXTA_SUCCESS != status) {
        resAdvType = (Jxta_resolved_adv_type *) calloc(1, sizeof(Jxta_resolved_adv_type));
        JXTA_OBJECT_INIT(resAdvType, advType_free, 0);
        resAdvType->dbSpace = JXTA_OBJECT_SHARE(dbSpace);
        resAdvType->adv = strdup(ns);
        jxta_hashtable_put(me->resolvedAdvs, ns, strlen(ns), (Jxta_object *) resAdvType);
        status = JXTA_SUCCESS;
    } else {
        status = JXTA_FAILED;
    }
    JXTA_OBJECT_RELEASE(resAdvType);
    return status;
}

static Jxta_status cm_address_spaces_process(Jxta_cm * self)
{
    DBSpace *dbSpace = NULL;
    unsigned int i;
    Jxta_status status = JXTA_SUCCESS;
    Jxta_addressSpace *jas = NULL;
    apr_status_t res = APR_SUCCESS;
    Jxta_hashtable *dbsHash = NULL;

    for (i = 0; i < jxta_vector_size(self->addressSpaces); i++) {
        JString *jDriver = NULL;
        JString *jName = NULL;
        DBSpace *dbCheck = NULL;
        const char *cName = NULL;
        /* create a db structure for the address space */
        dbSpace = calloc(1, sizeof(DBSpace));
        JXTA_OBJECT_INIT(dbSpace, dbSpace_free, 0);
        dbSpace->thread_pool = self->thread_pool;
        status = jxta_vector_get_object_at(self->addressSpaces, JXTA_OBJECT_PPTR(&jas), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error obtaining address space -- %i\n", status);
            JXTA_OBJECT_RELEASE(dbSpace);
            continue;
        }
        jName = jxta_cache_config_addr_get_name(jas);
        cName = jstring_get_string(jName);
        dbSpace->jas = JXTA_OBJECT_SHARE(jas);
        jDriver = jxta_cache_config_addr_get_DBEngine(jas);
        dbSpace->jId = JXTA_OBJECT_SHARE(self->jGroupID_string);
        dbSpace->id = calloc(1, jstring_length(self->jGroupID_string) + 1);
        apr_snprintf((void *) dbSpace->id, jstring_length(self->jGroupID_string) + 1, "%s",
                     jstring_get_string(self->jGroupID_string));
        if (jxta_cache_config_addr_is_default(jas)) {
            if (self->defaultDB) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Duplicate DEFAULT space -- %s is the default now.\n",
                                jstring_get_string(jxta_cache_config_addr_get_name(jas)));
                JXTA_OBJECT_RELEASE(self->defaultDB);
            }
            self->defaultDB = JXTA_OBJECT_SHARE(dbSpace);
        }
        dbSpace->xaction_threshold = jxta_cache_config_addr_get_xaction_threshold(dbSpace->jas);
        /* sqlite3 */
        if (!strcmp(jstring_get_string(jDriver), "sqlite3")) {
            status = cm_SQLite_parms_process(self, dbSpace, jas);
        } else {

        }
        JXTA_OBJECT_RELEASE(jDriver);
        if (NULL == dbsHash) {
            dbsHash = jxta_hashtable_new(2);
        }
        /* use the alias since multiple in-memory db's can be created by SQLite */
        status = jxta_hashtable_get(dbsHash, dbSpace->alias, strlen(dbSpace->alias), JXTA_OBJECT_PPTR(&dbCheck));
        if (JXTA_SUCCESS != status) {
            jxta_hashtable_put(dbsHash, dbSpace->alias, strlen(dbSpace->alias), (Jxta_object *) dbSpace);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Duplicate database -- %s -- for address space --- %s\n",
                            dbSpace->alias, jstring_get_string(jName));
            JXTA_OBJECT_RELEASE(dbSpace);
            JXTA_OBJECT_RELEASE(dbCheck);
            continue;
        }
        if (jxta_cache_config_addr_is_single_db(self->cacheConfig) && NULL != self->parent) {
            status = jxta_hashtable_get(self->parent->db_connections, cName, strlen(cName) + 1, JXTA_OBJECT_PPTR(&dbSpace->conn));
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Parent CM does not have %s\n", cName);
                status = JXTA_FAILED;
                goto FINAL_EXIT;
            }
        } else {
            status = cm_address_space_conn_get(self, jas, &dbSpace->conn);
            apr_thread_mutex_lock(self->mutex);
            dbSpace->conn->log_id = ++log_id;
            apr_thread_mutex_unlock(self->mutex);
            dbSpace->conn->name = strdup(cName);
            res = cm_sql_db_init(dbSpace, jas);
            jxta_hashtable_put(self->db_connections, cName, strlen(cName) + 1, (Jxta_object *) dbSpace->conn);
        }
        if (res != APR_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to open dbd database: %s with error %ld\n", dbSpace->name,
                            res);
            status = JXTA_FAILED;
            goto FINAL_EXIT;
        } else {
            status = jxta_vector_add_object_last(self->dbSpaces, (Jxta_object *) dbSpace);
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error adding dbSpace --- %i\n", status);
                goto FINAL_EXIT;
            }
        }
        dbSpace->available = TRUE;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d %s is initialized dbSpace - [%pp]\n", dbSpace->conn->log_id,
                        jstring_get_string(jName), dbSpace);
        if (jName)
            JXTA_OBJECT_RELEASE(jName);
        JXTA_OBJECT_RELEASE(dbSpace);
        dbSpace = NULL;
        JXTA_OBJECT_RELEASE(jas);
        jas = NULL;
    }
  FINAL_EXIT:
    if (dbsHash)
        JXTA_OBJECT_RELEASE(dbsHash);
    if (jas)
        JXTA_OBJECT_RELEASE(jas);
    if (dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
    return status;
}

static Jxta_status cm_address_space_conn_get(Jxta_cm * me, Jxta_addressSpace * jas, DBConnection ** conn)
{
    Jxta_status status;
    apr_status_t res;
    status = JXTA_SUCCESS;
    *conn = calloc(1, sizeof(DBConnection));
    JXTA_OBJECT_INIT(*conn, dbConnection_free, 0);
    (*conn)->pool = me->pool;
    /* Create the mutex */
    res = apr_thread_mutex_create(&(*conn)->lock, APR_THREAD_MUTEX_NESTED,  /* nested */
                                  me->pool);
    if (res != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create apr_mutex: %d\n", res);
        status = JXTA_FAILED;
    }

    return status;
}

static Jxta_status cm_SQLite_parms_process(Jxta_cm * self, DBSpace * dbSpace, Jxta_addressSpace * jas)
{
    int name_len;
    const char *dbExt = ".sqdb";
    if (jxta_cache_config_addr_is_ephemeral(jas)) {
        name_len = strlen(":memory:") + 2;
        dbSpace->name = calloc(1, name_len);
        apr_snprintf((void *) dbSpace->name, name_len, "%s", ":memory:");
        name_len += 2;
        dbSpace->alias = calloc(1, name_len);
        apr_snprintf((void *) dbSpace->alias, name_len, "%s%d", dbSpace->name, ++in_memories);
    } else {
        const char *DBName;
        JString *jDBName;
        DBName = "";
        jDBName = jxta_cache_config_addr_get_DBName(jas);
        name_len = strlen(self->root) + jstring_length(self->jGroupID_string) + strlen(dbExt) + 2;
        if (strcmp(jstring_get_string(jDBName), "groupID")) {
            DBName = jstring_get_string(jDBName);
            name_len += strlen(DBName);
        }
        dbSpace->name = calloc(1, name_len);
        apr_snprintf((void *) dbSpace->name, name_len, "%s%s%s%s", self->root, jstring_get_string(self->jGroupID_string),
                     DBName, dbExt);
        dbSpace->alias = calloc(1, name_len);
        apr_snprintf((void *) dbSpace->alias, name_len, "%s", dbSpace->name);
        JXTA_OBJECT_RELEASE(jDBName);
    }
    return JXTA_SUCCESS;
}

static Jxta_status cm_name_spaces_process(Jxta_cm * self)
{
    unsigned int i;
    Jxta_status status;

    status = JXTA_SUCCESS;
    for (i = 0; i < jxta_vector_size(self->nameSpaces); i++) {
        Jxta_nameSpace *jns = NULL;
        JString *jName = NULL;
        JString *jAdvType = NULL;
        const char *advType = NULL;
        const char *advTypeTemp = NULL;
        Jxta_resolved_adv_type *resAdvType = NULL;

        status = jxta_vector_get_object_at(self->nameSpaces, JXTA_OBJECT_PPTR(&jns), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error retrieving a name space %i \n", status);
            break;
        }
        jName = jxta_cache_config_ns_get_addressSpace_string(jns);
        jAdvType = jxta_cache_config_ns_get_ns(jns);
        advType = jstring_get_string(jAdvType);
        advTypeTemp = advType;

        if (!jxta_cache_config_ns_is_type_wildcard(jns)) {
            DBSpace *dbSpace = NULL;
            const char *asString;
            const char *nsType;
            Jxta_status sstat;
            nsType = jstring_get_string(jAdvType);
            asString = jstring_get_string(jName);
            sstat = jxta_hashtable_get(self->resolvedAdvs, nsType, strlen(nsType), JXTA_OBJECT_PPTR(&resAdvType));
            if (JXTA_SUCCESS != sstat) {
                dbSpace = cm_dbSpace_get(self, asString);
                if (NULL == dbSpace)
                    dbSpace = JXTA_OBJECT_SHARE(self->defaultDB);

                if (JXTA_SUCCESS == cm_name_space_register(self, dbSpace, jstring_get_string(jAdvType))) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s has been registered in %s --- dbSpace db_id: %d [%pp]\n",
                                    nsType, asString, dbSpace->conn->log_id, dbSpace);
                }
                JXTA_OBJECT_RELEASE(dbSpace);
            }
            JXTA_OBJECT_RELEASE(resAdvType);
        }
        JXTA_OBJECT_RELEASE(jAdvType);
        JXTA_OBJECT_RELEASE(jName);
        JXTA_OBJECT_RELEASE(jns);
    }
    return status;
}

static Jxta_status cm_name_spaces_populate(Jxta_cm * self)
{
    Jxta_status status = JXTA_SUCCESS;

    DBSpace *dbSpace = NULL;
    JString *where = jstring_new_0();
    JString *columns = jstring_new_0();
    JString *sort = jstring_new_0();
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row;
    const char *entry;
    int rv = 0;
    unsigned int i, j;

    jstring_append_2(columns, CM_COL_NameSpace);

    cm_sql_append_timeout_check_greater(where);
    jstring_append_2(sort, SQL_GROUP CM_COL_NameSpace);

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", dbSpace->id, aprs);
        goto finish;
    }
    for (j = 0; j < jxta_vector_size(self->dbSpaces); j++) {
        int num_tuples = 0;
        jxta_vector_get_object_at(self->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), j);

        rv = cm_sql_select(dbSpace, pool, CM_TBL_ADVERTISEMENTS, &res, columns, where, sort, TRUE);

        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- cm_name_spaces_populate Select failed: %s %i\n",
                            dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
            goto finish;
        }
        num_tuples = apr_dbd_num_tuples((apr_dbd_driver_t *) dbSpace->conn->driver, res);
        for (i = 0; i < num_tuples; i++) {
            rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);
            if (rv == 0) {
                entry = apr_dbd_get_entry(dbSpace->conn->driver, row, 0);
                if (entry != NULL) {
                    if (JXTA_SUCCESS == cm_name_space_register(self, dbSpace, entry)) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s is being registered in db_id: %d------- \n",
                                        entry, dbSpace->conn->log_id);
                        status = JXTA_SUCCESS;
                    }
                }
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- cm_name_spaces_populate Select failed: %s %i\n",
                                dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
                goto finish;
            }
        }
        JXTA_OBJECT_RELEASE(dbSpace);
        dbSpace = NULL;
    }

  finish:
    if (NULL != dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(sort);
    return status;
}

static void cm_best_choice_select(Jxta_cm * self)
{
    DBSpace *dbSpace;
    Jxta_time start;
    Jxta_time end;
    Jxta_time_diff best_so_far;
    Jxta_time_diff diff;
    Jxta_status status;
    unsigned int i;
    best_so_far = 0;
    for (i = 0; i < jxta_vector_size(self->dbSpaces); i++) {
        int j;
        status = jxta_vector_get_object_at(self->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "cm_best_choice_select - error retrieving dbSpaces %i\n", status);
            continue;
        }
        /* clean up */
        cm_item_delete(dbSpace, CM_TBL_ELEM_ATTRIBUTES, "performanceID");

        start = jpr_time_now();
        j = 10;
        while (j--) {
            cm_item_insert(dbSpace, CM_TBL_ELEM_ATTRIBUTES, "performance:Test", "performanceID", "NameSpace",
                           "performance:test", "", 1000, 1000);
            cm_item_delete(dbSpace, CM_TBL_ELEM_ATTRIBUTES, "performanceID");
        }
        end = jpr_time_now();

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "choice  - end time " JPR_DIFF_TIME_FMT " - " JPR_DIFF_TIME_FMT "\n",
                        end, start);
        diff = end - start;
        if (diff < best_so_far || 0 == best_so_far) {
            best_so_far = diff;
            if (self->bestChoiceDB)
                JXTA_OBJECT_RELEASE(self->bestChoiceDB);
            self->bestChoiceDB = JXTA_OBJECT_SHARE(dbSpace);
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO,
                        "cm_best_choice_select - best_so_far -  " JPR_DIFF_TIME_FMT " %s\n", best_so_far, dbSpace->alias);
        JXTA_OBJECT_RELEASE(dbSpace);
    }
}

/*
 * initialize a SQL database and create the tables and indexes
*/
static apr_status_t cm_sql_db_init(DBSpace * dbSpace, Jxta_addressSpace * jas)
{
    apr_status_t rv;
    Jxta_status status = JXTA_SUCCESS;
    JString *jDriver = NULL;
    JString *jParms = NULL;
    const char *driver;
    const char *advIdIndex[] = { CM_COL_AdvId, NULL };
    const char *nsIndex[] = { CM_COL_NameSpace, CM_COL_AdvId, NULL };
    const char *timeOutIndex[] = { CM_COL_TimeOut, NULL };

    jDriver = jxta_cache_config_addr_get_DBEngine(jas);
    driver = jstring_get_string(jDriver);
    if (!dbd_initialized) {
        rv = apr_pool_create(&apr_dbd_init_pool, NULL);

        if (rv != APR_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                            "Unable to create apr_pool initializing the dbd interface: %d\n", rv);
            goto FINAL_EXIT;
        }
        apr_dbd_init(apr_dbd_init_pool);
        dbd_initialized = TRUE;
    }
    rv = apr_dbd_get_driver(dbSpace->conn->pool, driver, &dbSpace->conn->driver);
    status = rv;

    switch (rv) {
    case APR_SUCCESS:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Loaded %s driver OK.\n", driver);
        break;
    case APR_EDSOOPEN:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to load driver file apr_dbd_%s.so\n", driver);
        goto FINAL_EXIT;
    case APR_ESYMNOTFOUND:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to load driver apr_dbd_%s_driver.\n", driver);
        goto FINAL_EXIT;
    case APR_ENOTIMPL:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "No driver available for %s.\n", driver);
        goto FINAL_EXIT;
    default:   /* it's a bug if none of the above happen */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Internal error loading %s.\n", driver);
        goto FINAL_EXIT;
    }
    if (NULL == dbSpace->name) {
        jParms = jxta_cache_config_addr_get_connectString(dbSpace->jas);
    } else {
        jParms = jstring_new_2(dbSpace->name);
    }
    rv = apr_dbd_open(dbSpace->conn->driver, dbSpace->conn->pool, jstring_get_string(jParms), &dbSpace->conn->sql);
    JXTA_OBJECT_RELEASE(jParms);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "     ===============  %s is assigned log Id : %d  ==============\n\n",
                    dbSpace->alias, dbSpace->conn->log_id);
    switch (rv) {
    case APR_SUCCESS:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "db_id: %d Opened %s[%s] OK\n", dbSpace->conn->log_id, driver,
                        dbSpace->alias);
        break;
    case APR_EGENERAL:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d Failed to open %s[%s]\n", dbSpace->conn->log_id, driver,
                        dbSpace->alias);
        goto FINAL_EXIT;
    default:   /* it's a bug if none of the above happen */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d Internal error opening %s[%s]\n", dbSpace->conn->log_id,
                        driver, dbSpace->alias);
        goto FINAL_EXIT;
    }

    /* sqlite3 specific */
    if (!strcmp(driver, "sqlite3")) {

        if (jxta_cache_config_addr_is_auto_vacuum(jas)) {
            cm_sql_pragma(dbSpace, " auto_vacuum = ON; ");
        }
        if (jxta_cache_config_addr_is_highPerformance(jas)) {
            cm_sql_pragma(dbSpace, " synchronous = OFF; ");
        } else {
            cm_sql_pragma(dbSpace, " synchronous = NORMAL; ");
        }
    }

    /** create tables for the advertisements, indexes and srdi 
    * 
    **/

    rv = cm_sql_table_create(dbSpace, jxta_elemTable, jxta_elemTable_fields[0]);
    if (rv != APR_SUCCESS)
        goto FINAL_EXIT;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d -- Element table created \n", dbSpace->conn->log_id);

    rv = cm_sql_table_create(dbSpace, jxta_advTable, jxta_advTable_fields[0]);
    if (rv != APR_SUCCESS)
        goto FINAL_EXIT;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d -- Advertisement table created\n", dbSpace->conn->log_id);

    rv = cm_sql_table_create(dbSpace, jxta_srdiTable, jxta_srdiTable_fields[0]);
    if (rv != APR_SUCCESS)
        goto FINAL_EXIT;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d -- SRDI table created\n", dbSpace->conn->log_id);

    cm_sql_delete_with_where(dbSpace, jxta_srdiTable, NULL);

    rv = cm_sql_table_create(dbSpace, jxta_replicaTable, jxta_replicaTable_fields[0]);
    if (rv != APR_SUCCESS)
        goto FINAL_EXIT;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d -- Replica table created\n", dbSpace->conn->log_id);

    cm_sql_delete_with_where(dbSpace, jxta_replicaTable, NULL);

    /* create indexes */

    rv = cm_sql_index_create(dbSpace, jxta_advTable, &nsIndex[0]);
    if (rv != APR_SUCCESS)
        goto FINAL_EXIT;

    /* create indexes */

    rv = cm_sql_index_create(dbSpace, jxta_advTable, &advIdIndex[0]);
    if (rv != APR_SUCCESS)
        goto FINAL_EXIT;

    /* create indexes */

    rv = cm_sql_index_create(dbSpace, jxta_advTable, &timeOutIndex[0]);
    if (rv != APR_SUCCESS)
        goto FINAL_EXIT;

    /* create indexes */

    rv = cm_sql_index_create(dbSpace, jxta_elemTable, &nsIndex[0]);
    if (rv != APR_SUCCESS)
        goto FINAL_EXIT;
    /* create indexes */

    rv = cm_sql_index_create(dbSpace, jxta_elemTable, &advIdIndex[0]);
    if (rv != APR_SUCCESS)
        goto FINAL_EXIT;

    /* create indexes */

    rv = cm_sql_index_create(dbSpace, jxta_elemTable, &timeOutIndex[0]);
    if (rv != APR_SUCCESS)
        goto FINAL_EXIT;

  FINAL_EXIT:
    if (jDriver)
        JXTA_OBJECT_RELEASE(jDriver);
    if (rv != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d -- error creating tables db return %i\n",
                        dbSpace->conn->log_id, rv);
    }
    return rv;
}

static apr_status_t cm_best_choice_tables_create(Jxta_cm * me)
{
    apr_status_t res;

    const char *idxIndex[] = { CM_COL_Peerid, CM_COL_SeqNumber, CM_COL_GroupID, NULL };
    const char *deltaIndex[] = { CM_COL_Handler, CM_COL_Peerid, CM_COL_AdvId, CM_COL_Name, CM_COL_Value, NULL };

    res = cm_sql_table_create(me->bestChoiceDB, jxta_srdiIndexTable, jxta_srdiIndexTable_fields[0]);
    if (res != APR_SUCCESS)
        goto FINAL_EXIT;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d -- SRDI Index table created\n", me->bestChoiceDB->conn->log_id);

    cm_sql_delete_with_where(me->bestChoiceDB, jxta_srdiIndexTable, NULL);

    res = cm_sql_table_create(me->bestChoiceDB, jxta_srdiDeltaTable, jxta_srdiDeltaTable_fields[0]);
    if (res != APR_SUCCESS)
        goto FINAL_EXIT;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d -- SRDI Delta table created\n", me->bestChoiceDB->conn->log_id);

    cm_sql_delete_with_where(me->bestChoiceDB, jxta_srdiDeltaTable, NULL);

    /* create indexes */

    res = cm_sql_index_create(me->bestChoiceDB, jxta_srdiIndexTable, &idxIndex[0]);
    if (res != APR_SUCCESS)
        goto FINAL_EXIT;

    /* create indexes */

    res = cm_sql_index_create(me->bestChoiceDB, jxta_srdiDeltaTable, &deltaIndex[0]);

  FINAL_EXIT:
    if (APR_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Error creating bestChoice dbs %s",
                        apr_dbd_error(me->bestChoiceDB->conn->driver, me->bestChoiceDB->conn->sql, res));
    }
    return res;
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
    Jxta_status status = JXTA_SUCCESS;
    DBSpace *dbSpace = NULL;
    unsigned int i;
    for (i = 0; i < jxta_vector_size(self->dbSpaces); i++) {
        status = jxta_vector_get_object_at(self->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "db_id: %d Error getting a dbSpace -- %i \n",
                            dbSpace->conn->log_id, status);
            continue;
        }
        status = cm_expired_records_remove(dbSpace, folder->name, dbSpace->conn->log_id == self->bestChoiceDB->conn->log_id ? TRUE:FALSE);
        JXTA_OBJECT_RELEASE(dbSpace);
    }
    return status;
}

/* remove an advertisement given the group_id, type and advertisement name */
Jxta_status cm_remove_advertisement(Jxta_cm * self, const char *folder_name, char *primary_key)
{
    Folder *folder;
    Jxta_status status;
    unsigned int i;
    DBSpace *dbSpace = NULL;
    JString *where = NULL;
    JString *jKey = NULL;
    JString *jFolder = NULL;
    Jxta_expiration_time exp_time;

    status = jxta_hashtable_get(self->folders, folder_name, (size_t) strlen(folder_name), JXTA_OBJECT_PPTR(&folder));

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "unable to get folder %s in cm folders hash status: %d\n", folder_name,
                        status);
        return status;
    }
    jKey = jstring_new_2(primary_key);
    jFolder = jstring_new_2(folder_name);

    where = jstring_new_0();
    jstring_append_2(where, CM_COL_AdvId);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jKey);
    jstring_append_2(where, SQL_AND);
    cm_sql_correct_space(folder_name, where);

    status = cm_get_time(self, folder_name, primary_key, &exp_time, CM_COL_TimeOutForOthers);
    if (JXTA_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "unable to get expiration time with status %d, won't flush SRDI\n",
                        status);
        exp_time = 0;
    }

    /* don't flush srdi with private advertisement */
    if (self->record_delta && exp_time != 0) {
        Jxta_vector *v_entries;
        v_entries = cm_create_srdi_entries(self, jFolder, jKey, NULL);
        if (v_entries != NULL) {
            Jxta_SRDIEntryElement *entry;
            for (i = 0; i < jxta_vector_size(v_entries); i++) {
                status = jxta_vector_get_object_at(v_entries, JXTA_OBJECT_PPTR(&entry), i);
                if (JXTA_SUCCESS != status) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "unable to get entry at %i in cm_remove_advertisement %d\n",
                                    i, status);
                }
                /* this will delete the entry at the rdv */
                entry->expiration = 0;
                cm_srdi_delta_add(self->srdi_delta, folder, entry);
                JXTA_OBJECT_RELEASE(entry);
            }
        }
        JXTA_OBJECT_RELEASE(v_entries);
    }
    /* remove the advertisement and all it's indexes */
    for (i = 0; i < jxta_vector_size(self->dbSpaces); i++) {
        status = jxta_vector_get_object_at(self->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "db_id: %d Error getting dbSpace -- %i\n", dbSpace->conn->log_id,
                            status);
            continue;
        }
        status = cm_sql_delete_with_where(dbSpace, CM_TBL_ADVERTISEMENTS, where);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s Error removing Advertisement -- %i\n",
                            dbSpace->conn->log_id, dbSpace->id, status);
        }
        status = cm_sql_delete_with_where(dbSpace, CM_TBL_ELEM_ATTRIBUTES, where);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s Error removing ElementAttributes -- %i\n",
                            dbSpace->conn->log_id, dbSpace->id, status);
        }
        status = cm_sql_delete_with_where(dbSpace, CM_TBL_SRDI, where);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s Error removing SRDI -- %i\n", dbSpace->conn->log_id,
                            dbSpace->id, status);
        }
        JXTA_OBJECT_RELEASE(dbSpace);
    }
    JXTA_OBJECT_RELEASE(jFolder);
    JXTA_OBJECT_RELEASE(folder);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jKey);
    return status;
}

Jxta_status cm_remove_advertisements(Jxta_cm * self, const char *folder_name, char **keys)
{
    Folder *folder;
    Jxta_status status;
    unsigned int i;
    int retry_count;
    apr_status_t rv;
    apr_pool_t *pool = NULL;
    DBSpace *dbSpace = NULL;
    JString *where = jstring_new_0();
    JString *jKey = jstring_new_0();
    char **keysSave = keys;
    status = jxta_hashtable_get(self->folders, folder_name, (size_t) strlen(folder_name), JXTA_OBJECT_PPTR(&folder));

    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(where);
        JXTA_OBJECT_RELEASE(jKey);
        return status;
    }
    keysSave = keys;
    rv = apr_pool_create(&pool, NULL);
    if (APR_SUCCESS != rv) {
        status = JXTA_NOMEM;
        goto FINAL_EXIT;
    }
    /* remove the advertisement and all it's indexes */
    for (i = 0; i < jxta_vector_size(self->dbSpaces); i++) {
        status = jxta_vector_get_object_at(self->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "db_id: %d Error getting dbSpace -- %i\n", dbSpace->conn->log_id,
                            status);
            continue;
        }
        apr_thread_mutex_lock(dbSpace->conn->lock);
        retry_count = 3;
        while (retry_count--) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,
                            "db_id: %d ------------------------        Starting Transaction -- %s\n", dbSpace->conn->log_id,
                            dbSpace->alias);
            rv = apr_dbd_transaction_start(dbSpace->conn->driver, pool, dbSpace->conn->sql,
                                           &dbSpace->conn->transaction);
            if (rv) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d Start transaction failed!\n%s\n",
                                dbSpace->conn->log_id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv));
                retry_count = 0;
                break;
            }
            while (*keys) {
                cm_remove_advertisement(self, folder_name, *keys);
                keys++;
            }
            rv = apr_dbd_transaction_end(dbSpace->conn->driver, pool, dbSpace->conn->transaction);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d ----------      Ending Transaction inside -- %s\n",
                            dbSpace->conn->log_id, dbSpace->alias);
            if (rv) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d End transaction failed! %d\n%s\n",
                                dbSpace->conn->log_id, rv, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv));
            } else if (JXTA_SUCCESS == status) {
                retry_count = 0;
            } else {
                keys = keysSave;
            }
        }
        keys = keysSave;
        dbSpace->conn->transaction = NULL;
        apr_thread_mutex_unlock(dbSpace->conn->lock);
        JXTA_OBJECT_RELEASE(dbSpace);
    }

FINAL_EXIT:
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(jKey);
    JXTA_OBJECT_RELEASE(folder);
    JXTA_OBJECT_RELEASE(where);
    return JXTA_SUCCESS;
}

/* return a pointer to all advertisements in folder folder_name */
/* a NULL pointer means the end of the list */
char **cm_get_primary_keys(Jxta_cm * self, char *folder_name, Jxta_boolean no_locals)
{
    JString *where = NULL;
    char **ret;
    if (no_locals) {
        where = jstring_new_2(CM_COL_TimeOutForOthers SQL_GREATER_THAN "0");
    }
    ret = cm_sql_get_primary_keys(self, folder_name, CM_TBL_ADVERTISEMENTS, where, NULL, -1);
    JXTA_OBJECT_RELEASE(where);
    return ret;
}

char **cm_sql_get_primary_keys(Jxta_cm * self, char *folder_name, const char *tableName, JString * jWhere,
                               JString * jGroup, int row_max)
{
    size_t nb_keys;
    char **result = NULL;
    char **resultReturn = NULL;
    DBSpace **dbSpace = NULL;
    DBSpace **saveDBSpace = NULL;
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row;
    const char *entry;
    int rv = 0;
    unsigned int n;
    unsigned int i = 0;
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

    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(where, self->jGroupID_string);

    if (NULL != jGroup) {
        jstring_append_1(where, jGroup);
    }
    if (!strcmp(folder_name, "Peers")) {
        dbSpace = cm_dbSpaces_get(self, "jxta:PA", NULL, FALSE);
    } else if (!strcmp(folder_name, "Groups")) {
        dbSpace = cm_dbSpaces_get(self, "jxta:PGA", NULL, FALSE);
    } else {
        dbSpace = cm_dbSpaces_get(self, "*:*", NULL, FALSE);
    }
    saveDBSpace = dbSpace;
    while (*dbSpace) {
        rv = cm_sql_select(*dbSpace, pool, tableName, &res, columns, where, NULL, FALSE);

        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                            "db_id: %d %s jxta_cm_get_primary_keys Select failed: %s %i \n %s   %s\n", (*dbSpace)->conn->log_id,
                            (*dbSpace)->id, apr_dbd_error((*dbSpace)->conn->driver, (*dbSpace)->conn->sql, rv), rv,
                            jstring_get_string(columns), jstring_get_string(where));
            goto finish;
        }
        nb_keys = apr_dbd_num_tuples((apr_dbd_driver_t *) (*dbSpace)->conn->driver, res);
        if (nb_keys == 0) {
            JXTA_OBJECT_RELEASE(*dbSpace);
            dbSpace++;
            continue;
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d ---- nb_keys - %i\n", (*dbSpace)->conn->log_id, nb_keys);
        result = (char **) calloc(1, sizeof(char *) * (nb_keys + 1));
        for (i = 0, rv = apr_dbd_get_row((*dbSpace)->conn->driver, pool, res, &row, -1);
             rv == 0 && (i < row_max || row_max == -1);
             i++, rv = apr_dbd_get_row((*dbSpace)->conn->driver, pool, res, &row, -1)) {
            int column_count = apr_dbd_num_cols((*dbSpace)->conn->driver, res);
            for (n = 0; n < column_count; ++n) {
                entry = apr_dbd_get_entry((*dbSpace)->conn->driver, row, n);
                if (entry == NULL) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- (null) in entry from get_primary_keys",
                                    (*dbSpace)->conn->log_id, (*dbSpace)->id);
                } else {
                    char *tmp;
                    int len = strlen(entry);
                    tmp = malloc(len + 1);
                    memcpy(tmp, entry, len + 1);
                    result[i] = tmp;
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "db_id: %d %s -- key: %s \n", (*dbSpace)->conn->log_id,
                                    (*dbSpace)->id, result[i]);
                }
            }
        }
        result[i] = NULL;
        resultReturn = (char **) cm_arrays_concatenate(JXTA_OBJECT_PPTR(resultReturn), JXTA_OBJECT_PPTR(result));
        free(result);
        JXTA_OBJECT_RELEASE(*dbSpace);
        dbSpace++;
    }
  finish:
    while (*dbSpace) {
        JXTA_OBJECT_RELEASE(*dbSpace);
        dbSpace++;
    }
    if (saveDBSpace)
        free(saveDBSpace);
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(columns);
    return resultReturn;
}

static void cm_delta_entry_create(JString * jGroupID, JString * dbAlias, JString * jPeerid, JString * jSourcePeerid, JString * jHandler, JString * sqlval,
                                  Jxta_SRDIEntryElement * entry, int delta_window)
{
    char aTime[64];
    JString *empty_j = NULL;
    Jpr_interval_time nnow;
    Jpr_interval_time actual_timeout;

    empty_j = jstring_new_2("");
    jstring_append_2(sqlval, SQL_LEFT_PAREN);
    SQL_VALUE(sqlval, entry->nameSpace);
    jstring_append_2(sqlval, SQL_COMMA);
    if (NULL != jHandler) {
        SQL_VALUE(sqlval, jHandler);
    } else {
        SQL_VALUE(sqlval, empty_j);
    }
    jstring_append_2(sqlval, SQL_COMMA);
    if (NULL != jPeerid) {
        SQL_VALUE(sqlval, jPeerid);
    } else {
        SQL_VALUE(sqlval, empty_j);
    }
    JXTA_OBJECT_RELEASE(empty_j);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, entry->advId);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, entry->key);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, entry->value);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, entry->range);
    jstring_append_2(sqlval, SQL_COMMA);
    memset(aTime, 0, sizeof(aTime));

    nnow = jpr_time_now();
    actual_timeout = lifetime_get(nnow, entry->expiration);

    /* timeout */
    if (apr_snprintf(aTime, sizeof(aTime), "%" APR_INT64_T_FMT, actual_timeout) != 0) {
        jstring_append_2(sqlval, aTime);
    } else {
        actual_timeout = lifetime_get(nnow, DEFAULT_EXPIRATION);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                        "Unable to format time for me and current time - set to default timeout \n");
        apr_snprintf(aTime, sizeof(aTime), "%" APR_INT64_T_FMT, actual_timeout);
        jstring_append_2(sqlval, aTime);
    }

    jstring_append_2(sqlval, SQL_COMMA);
    /* time out for others */
    memset(aTime, 0, sizeof(aTime));
    if (apr_snprintf(aTime, sizeof(aTime), "%" APR_INT64_T_FMT, entry->expiration) != 0) {
        jstring_append_2(sqlval, aTime);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to format time for me - set to default timeout \n");
        apr_snprintf(aTime, sizeof(aTime), "%" APR_INT64_T_FMT, DEFAULT_EXPIRATION);
        jstring_append_2(sqlval, aTime);
    }

    /* groupid */
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jGroupID);
    /* sequence number */
    jstring_append_2(sqlval, SQL_COMMA);
    memset(aTime, 0, sizeof(aTime));
    apr_snprintf(aTime, sizeof(aTime), JXTA_SEQUENCE_NUMBER_FMT, entry->seqNumber);
    jstring_append_2(sqlval, aTime);

    /* initialize next update to 0 */
    jstring_append_2(sqlval, SQL_COMMA);
    jstring_append_2(sqlval, "0");

    memset(aTime, 0, sizeof(aTime));
    apr_snprintf(aTime, sizeof(aTime), "%d", delta_window);

    jstring_append_2(sqlval, SQL_COMMA);
    jstring_append_2(sqlval, aTime);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Create an entry --- time: %" APR_INT64_T_FMT " next_update: 0 Timeout: %" APR_INT64_T_FMT "\n", nnow, actual_timeout);

    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jSourcePeerid);
    jstring_append_2(sqlval, SQL_COMMA);
    jstring_append_2(sqlval, TRUE == entry->duplicate ? "1":"0");
    jstring_append_2(sqlval, SQL_RIGHT_PAREN);

}

static DBSpace *cm_dbSpace_by_alias_get(Jxta_cm * me, const char *dbAlias)
{
    DBSpace *dbSpace = NULL;
    int i;
    for (i = 0; i < jxta_vector_size(me->dbSpaces); i++) {
        jxta_vector_get_object_at(me->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), i);
        if (!strcmp(dbAlias, dbSpace->alias)) {
            return dbSpace;
        }
        JXTA_OBJECT_RELEASE(dbSpace);
    }
    return NULL;
}
static Jxta_status cm_srdi_index_get(Jxta_cm * me, JString * jPeerid, Jxta_sequence_number seqNumber
                        , DBSpace ** dbRet, JString ** jSrcPeerid, JString ** jAdvId, JString ** jName
                        , Jxta_boolean * bReplica, Jxta_boolean * bReplicate
                        , JString ** jSQLPredicate)
{
    Jxta_status status = JXTA_SUCCESS;
    char aTmp[64];
    long nb_keys;
    apr_status_t rv;
    apr_dbd_row_t *row = NULL;
    DBSpace *dbSpace = NULL;
    apr_status_t aprs;
    const char *advId;
    const char *name;
    const char *dbAlias;
    const char *replica;
    const char *replicate;
    const char *source_id;
    JString *jSeq = NULL;
    JString *jWhere = NULL;
    JString *jWhere_seq = NULL;
    JString *jWhere_index = NULL;
    JString *jColumns = NULL;
    apr_pool_t *pool = NULL;
    apr_dbd_results_t *res = NULL;
    jWhere = jstring_new_0();
    jWhere_seq = jstring_new_0();
    jWhere_index = jstring_new_0();

    jColumns = jstring_new_2(CM_COL_DBAlias SQL_COMMA CM_COL_AdvId SQL_COMMA CM_COL_Name SQL_COMMA CM_COL_Replica SQL_COMMA CM_COL_Replicate SQL_COMMA CM_COL_SourcePeerid);

    /* get the database alias from the srdi index */

    jstring_append_2(jWhere_seq, CM_COL_SeqNumber SQL_EQUAL);

    memset(aTmp, 0, sizeof(aTmp));
    apr_snprintf(aTmp, sizeof(aTmp), JXTA_SEQUENCE_NUMBER_FMT, seqNumber);
    jSeq = jstring_new_2(aTmp);
    jstring_append_2(jWhere_seq, aTmp);

    jstring_append_2(jWhere, CM_COL_Peerid SQL_EQUAL);
    SQL_VALUE(jWhere, jPeerid);
    jstring_append_2(jWhere, SQL_AND CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(jWhere, me->jGroupID_string);

    jstring_append_1(jWhere_index, jWhere_seq);
    jstring_append_2(jWhere_index, SQL_AND);
    jstring_append_1(jWhere_index, jWhere);

    dbSpace = JXTA_OBJECT_SHARE(me->bestChoiceDB);
    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", dbSpace->id, aprs);
        status = JXTA_NOMEM;
        goto FINAL_EXIT;
    }

    /* ------------  get the index entry for this seq. no. */

    status = cm_sql_select(dbSpace, pool, CM_TBL_SRDI_INDEX, &res, jColumns, jWhere_index, NULL, TRUE);

    nb_keys = apr_dbd_num_tuples((apr_dbd_driver_t *) dbSpace->conn->driver, res);
    if (nb_keys > 0) {
        rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);
        dbAlias = apr_dbd_get_entry(dbSpace->conn->driver, row, 0);
        advId = apr_dbd_get_entry(dbSpace->conn->driver, row, 1);
        name = apr_dbd_get_entry(dbSpace->conn->driver, row, 2);
        replica = apr_dbd_get_entry(dbSpace->conn->driver, row, 3);
        replicate = apr_dbd_get_entry(dbSpace->conn->driver, row, 4);
        source_id = apr_dbd_get_entry(dbSpace->conn->driver, row, 5);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d There is no entry for seq: %s from: %s in groupid:%s\n",
                        dbSpace->conn->log_id, jstring_get_string(jSeq), jstring_get_string(jPeerid),
                        jstring_get_string(me->jGroupID_string));
        status = JXTA_ITEM_NOTFOUND;
        goto FINAL_EXIT;
    }
    *jAdvId = jstring_new_2(advId);
    *jName = jstring_new_2(name);
    *jSrcPeerid = jstring_new_2(source_id);
    if (jSQLPredicate != NULL) {
        *jSQLPredicate = JXTA_OBJECT_SHARE(jWhere_index);
    }
    *dbRet = cm_dbSpace_by_alias_get(me, dbAlias);
    *bReplica = !strcmp(replica, "1") ? TRUE:FALSE;
    *bReplicate = !strcmp(replicate, "1") ? TRUE:FALSE;


  FINAL_EXIT:
    JXTA_OBJECT_RELEASE(jSeq);
    JXTA_OBJECT_RELEASE(jWhere);
    JXTA_OBJECT_RELEASE(jWhere_seq);
    JXTA_OBJECT_RELEASE(jWhere_index);
    JXTA_OBJECT_RELEASE(jColumns);

    if (NULL != pool) {
        apr_pool_destroy(pool);
    }

    if (dbSpace) {
        JXTA_OBJECT_RELEASE(dbSpace);
    }
    return status;
}

Jxta_status cm_get_srdi_with_seq_number(Jxta_cm * me, JString * jPeerid, Jxta_sequence_number seq, Jxta_SRDIEntryElement * entry)
{

    Jxta_status status = JXTA_SUCCESS;
    long nb_keys;
    apr_status_t rv;
    apr_pool_t *pool = NULL;
    apr_dbd_results_t *res = NULL;
    apr_status_t aprs;
    Jxta_boolean bReplica;
    Jxta_boolean bReplicate;
    const char *value;
    const char *nameSpace;
    apr_dbd_row_t *row = NULL;
    DBSpace *dbSRDI = NULL;
    JString *jWhere = NULL;
    JString *jColumns = NULL;
    JString *jSrcPeerid = NULL;
    JString *jName = NULL;
    JString *jAdvId = NULL;

    status = cm_srdi_index_get(me, jPeerid, seq, &dbSRDI, &jSrcPeerid, &jAdvId, &jName, &bReplica, &bReplicate, NULL);
    if (JXTA_SUCCESS != status || NULL == dbSRDI ) {
        goto FINAL_EXIT;
    }

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", dbSRDI->id, aprs);
        status = JXTA_NOMEM;
        goto FINAL_EXIT;
    }
    /* drill down */
    jWhere = jstring_new_2(CM_COL_Peerid SQL_EQUAL);
    SQL_VALUE(jWhere, jSrcPeerid);
    jstring_append_2(jWhere, SQL_AND CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(jWhere, me->jGroupID_string);

    jstring_append_2(jWhere, SQL_AND CM_COL_AdvId SQL_EQUAL);
    SQL_VALUE(jWhere, jAdvId);
    jstring_append_2(jWhere, SQL_AND CM_COL_Name SQL_EQUAL);
    SQL_VALUE(jWhere, jName);

    jColumns = jstring_new_2(CM_COL_Value SQL_COMMA CM_COL_NameSpace);
    status = cm_sql_select(dbSRDI, pool, bReplica ? CM_TBL_REPLICA : CM_TBL_SRDI, &res, jColumns, jWhere, NULL, FALSE);


    nb_keys = apr_dbd_num_tuples((apr_dbd_driver_t *) dbSRDI->conn->driver, res);

    if (nb_keys > 0) {
        rv = apr_dbd_get_row(dbSRDI->conn->driver, pool, res, &row, -1);
        value = apr_dbd_get_entry(dbSRDI->conn->driver, row, 0);
        nameSpace = apr_dbd_get_entry(dbSRDI->conn->driver, row, 1);
        if (NULL != entry->value)
            JXTA_OBJECT_RELEASE(entry->value);
        if (NULL != entry->key)
            JXTA_OBJECT_RELEASE(entry->key);
        if (NULL != entry->advId)
            JXTA_OBJECT_RELEASE(entry->advId);
        if (NULL != entry->nameSpace)
            JXTA_OBJECT_RELEASE(entry->nameSpace);
        entry->value = jstring_new_2(value);
        entry->key = jstring_clone(jName);
        entry->advId = jstring_clone(jAdvId);
        entry->nameSpace = jstring_new_2(nameSpace);
        entry->seqNumber = seq;
        entry->replicate = bReplicate;
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "db_id: %d There is no entry for seq: " 
                                                         JXTA_SEQUENCE_NUMBER_FMT " from: %s in groupid:%s WHERE %s\n",
                        dbSRDI->conn->log_id, seq, jstring_get_string(jPeerid),
                        jstring_get_string(me->jGroupID_string), jstring_get_string(jWhere));
        status = JXTA_ITEM_NOTFOUND;
    }

FINAL_EXIT:
    if (jAdvId)
        JXTA_OBJECT_RELEASE(jAdvId);
    if (jSrcPeerid)
        JXTA_OBJECT_RELEASE(jSrcPeerid);
    if (jName)
        JXTA_OBJECT_RELEASE(jName);
    if (jWhere)
        JXTA_OBJECT_RELEASE(jWhere);

    if (NULL != pool)
        apr_pool_destroy(pool);

    JXTA_OBJECT_RELEASE(jColumns);
    if (dbSRDI)
        JXTA_OBJECT_RELEASE(dbSRDI);
    return status;
}

static Jxta_status cm_srdi_seq_number_update(Jxta_cm * me, JString * jPeerid, Jxta_SRDIEntryElement * entry)
{
    Jxta_status status = JXTA_SUCCESS;
    char aTmp[64];
    int nrows = 0;
    apr_status_t rv;
    char *table_name;
    Jxta_boolean bReplica = FALSE;
    Jxta_boolean bReplicate = FALSE;
    DBSpace *dbSpace = NULL;
    DBSpace *dbSRDI = NULL;
    JString *jUpdate_final = NULL;
    JString *jUpdate_sql = NULL;
    JString *jWhere = NULL;
    JString *jWhere_index = NULL;
    JString *jColumns = NULL;
    JString *jSourcePeerid = NULL;
    JString *jName = NULL;
    JString *jAdvId = NULL;
    jWhere = NULL;

    status = cm_srdi_index_get(me, jPeerid, entry->seqNumber, &dbSRDI, &jSourcePeerid, &jAdvId, &jName, &bReplica, &bReplicate, &jWhere_index);
    if (JXTA_SUCCESS != status) {
        goto FINAL_EXIT;
    }
    jUpdate_sql = jstring_new_0();
    jUpdate_final = jstring_new_0();
    jColumns = jstring_new_0();

    /* update the index table */

    if (entry->expiration > 0) {
        jstring_append_2(jUpdate_sql, SQL_UPDATE CM_TBL_SRDI_INDEX SQL_SET);

        jstring_append_2(jColumns, CM_COL_TimeOut SQL_EQUAL);

        memset(aTmp, 0, sizeof(aTmp));
        if (apr_snprintf(aTmp, sizeof(aTmp), "%" APR_INT64_T_FMT, lifetime_get(jpr_time_now(), entry->expiration)) != 0) {
            jstring_append_2(jColumns, aTmp);
        } else {
            goto FINAL_EXIT;
        }

        jstring_append_1(jUpdate_final, jUpdate_sql);
        jstring_append_1(jUpdate_final, jColumns);
    } else {
        jstring_append_2(jUpdate_sql, SQL_DELETE CM_TBL_SRDI_INDEX);
        jstring_append_1(jUpdate_final, jUpdate_sql);
    }
    if (NULL != jWhere_index) {
        jstring_append_2(jUpdate_final, SQL_WHERE);
        jstring_append_1(jUpdate_final, jWhere_index);
    }
    dbSpace = JXTA_OBJECT_SHARE(me->bestChoiceDB);

    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(jUpdate_final));

    if (rv != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Couldn't Update SRDI Index\n%s\nerrmsg==%s  rc=%i\n",
                        dbSpace->conn->log_id, dbSpace->id, jstring_get_string(jUpdate_final),
                        apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
        status = JXTA_FAILED;
        goto FINAL_EXIT;
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d SRDI Sequence Update  %d rows - %s\n", dbSpace->conn->log_id,
                        nrows, jstring_get_string(jUpdate_final));
        status = JXTA_SUCCESS;
    }

    JXTA_OBJECT_RELEASE(dbSpace);
    dbSpace = NULL;

    if (NULL != entry->value) {
        if (jstring_length(jColumns) > 0) {
            jstring_append_2(jColumns, SQL_COMMA);
        }
        jstring_append_2(jColumns, CM_COL_Value SQL_EQUAL);
        SQL_VALUE(jColumns, entry->value);
    }

    /* drill down */
    jWhere = jstring_new_2(CM_COL_Peerid SQL_EQUAL);
    SQL_VALUE(jWhere, jSourcePeerid);
    jstring_append_2(jWhere, SQL_AND CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(jWhere, me->jGroupID_string);

    jstring_append_2(jWhere, SQL_AND CM_COL_AdvId SQL_EQUAL);
    SQL_VALUE(jWhere, jAdvId);
    jstring_append_2(jWhere, SQL_AND CM_COL_Name SQL_EQUAL);
    SQL_VALUE(jWhere, jName);

    /* update the entry */
    jstring_reset(jUpdate_final, NULL);

    if (entry->expiration > 0) {
        if (jstring_length(jColumns) > 0) {
            jstring_append_2(jColumns, SQL_COMMA);
        }
        jstring_append_2(jColumns, CM_COL_TimeOutForOthers SQL_EQUAL);
        memset(aTmp, 0, sizeof(aTmp));
        if (apr_snprintf(aTmp, sizeof(aTmp), "%" APR_INT64_T_FMT, entry->expiration) != 0) {
            jstring_append_2(jColumns, aTmp);
        } else {
            goto FINAL_EXIT;
        }
    }

    if (bReplica) {
        table_name = CM_TBL_REPLICA;
    } else {
        table_name = CM_TBL_SRDI;
    }
    if (NULL != entry->value && !entry->expanded) {
        jstring_append_2(jColumns, SQL_COMMA CM_COL_Value SQL_EQUAL);
        SQL_VALUE(jColumns, entry->value);
    }
    if (entry->expiration > 0) {
        jstring_append_2(jUpdate_final, SQL_UPDATE );
        jstring_append_2(jUpdate_final, table_name);
        jstring_append_2(jUpdate_final, SQL_SET);
        jstring_append_1(jUpdate_final, jColumns);
    } else {
        jstring_append_2(jUpdate_final, SQL_DELETE);
        jstring_append_2(jUpdate_final, table_name);
    }

    jstring_append_2(jUpdate_final, SQL_WHERE);
    jstring_append_1(jUpdate_final, jWhere);

    rv = apr_dbd_query(dbSRDI->conn->driver, dbSRDI->conn->sql, &nrows, jstring_get_string(jUpdate_final));

    if (rv != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Couldn't Update SRDI\n%s\nerrmsg==%s  rc=%i\n",
                        dbSRDI->conn->log_id, dbSRDI->id, jstring_get_string(jUpdate_final),
                        apr_dbd_error(dbSRDI->conn->driver, dbSRDI->conn->sql, rv), rv);
        status = JXTA_FAILED;
        goto FINAL_EXIT;
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Sequence updated %d rows - %s\n", dbSRDI->conn->log_id, nrows,
                        jstring_get_string(jUpdate_final));
        status = JXTA_SUCCESS;
        if (0 == nrows) {
            status = JXTA_ITEM_NOTFOUND;
        }
    }

  FINAL_EXIT:
    if (jUpdate_final)
        JXTA_OBJECT_RELEASE(jUpdate_final);
    if (jUpdate_sql)
        JXTA_OBJECT_RELEASE(jUpdate_sql);
    if (jAdvId)
        JXTA_OBJECT_RELEASE(jAdvId);
    if (jSourcePeerid)
        JXTA_OBJECT_RELEASE(jSourcePeerid);
    if (jName)
        JXTA_OBJECT_RELEASE(jName);
    if (jWhere)
        JXTA_OBJECT_RELEASE(jWhere);
    if (jWhere_index)
        JXTA_OBJECT_RELEASE(jWhere_index);
    if (jColumns)
        JXTA_OBJECT_RELEASE(jColumns);

    if (dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
    if (dbSRDI)
        JXTA_OBJECT_RELEASE(dbSRDI);
    return status;
}

Jxta_status cm_update_srdi_times(Jxta_cm * me, JString * jPeerid, Jxta_time ttime)
{
    Jxta_status status = JXTA_SUCCESS;
    apr_status_t rv;
    int i;
    apr_status_t aprs;
    apr_pool_t *pool;
    apr_dbd_results_t *res = NULL;
    int num_tuples = 0;
    DBSpace *dbSpace = NULL;
    JString *jWhere = NULL;
    JString *jColumns = NULL;
    JString *jUpdateColumns = NULL;
    JString *where = NULL;
    const char *advid;
    const char *time_string;
    char **db_keys;
    char **db_keys_save;
    Jxta_hashtable *dbsHash = NULL;
    Jxta_hashtable *advsHash = NULL;
    JString *jSRDIAdvIds = NULL;
    JString *jReplicaAdvIds = NULL;
    char aTmp[64];
    struct adv_entry *entry;

    /* find entries of this peerid */
    jColumns = jstring_new_2(CM_COL_DBAlias SQL_COMMA CM_COL_AdvId SQL_COMMA CM_COL_Replica SQL_COMMA CM_COL_TimeOut);
    jWhere = jstring_new_2(CM_COL_Peerid SQL_EQUAL);
    SQL_VALUE(jWhere, jPeerid);

    /* find if original timeout is greater than time */
    jstring_append_2(jWhere, SQL_AND CM_COL_OriginalTimeout SQL_GREATER_THAN);
    apr_snprintf(aTmp, sizeof(aTmp), "%" APR_INT64_T_FMT, ttime);
    jstring_append_2(jWhere, aTmp);
    dbSpace = JXTA_OBJECT_SHARE(me->bestChoiceDB);

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", dbSpace->id, aprs);
        goto FINAL_EXIT;
    }
    status = cm_sql_select(dbSpace, pool, CM_TBL_SRDI_INDEX, &res, jColumns, jWhere, NULL, TRUE);
    if (JXTA_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error in select from SRDI_INDEX table %d\n", status);
        goto FINAL_EXIT;
    }
    num_tuples = apr_dbd_num_tuples((apr_dbd_driver_t *) dbSpace->conn->driver, res);
    for (i = 0; i < num_tuples; i++) {
        const char *alias;
        const char *replica;
        apr_dbd_row_t *row = NULL;
        rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);
        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- cm_update_srdi_times Select failed: %s %i\n",
                            dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
            status = JXTA_FAILED;
            goto FINAL_EXIT;
        }
        alias = apr_dbd_get_entry(dbSpace->conn->driver, row, 0);
        advid = apr_dbd_get_entry(dbSpace->conn->driver, row, 1);
        replica = apr_dbd_get_entry(dbSpace->conn->driver, row, 2);
        time_string = apr_dbd_get_entry(dbSpace->conn->driver, row, 3);
        if (NULL == dbsHash) {
            dbsHash = jxta_hashtable_new(0);
            if (NULL == dbsHash) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Could not create a dbsHash table\n");
                status = JXTA_NOMEM;
                goto FINAL_EXIT;
            }
        }
        if (JXTA_SUCCESS != jxta_hashtable_get(dbsHash, alias, strlen(alias) , JXTA_OBJECT_PPTR(&advsHash))) {
            advsHash = jxta_hashtable_new(0);
            jxta_hashtable_put(dbsHash, alias, strlen(alias) , (Jxta_object *) advsHash);
        }
        if (JXTA_SUCCESS != jxta_hashtable_get(advsHash, advid, strlen(advid) + 1, JXTA_OBJECT_PPTR(&entry))) {
            entry = calloc(1, sizeof(adv_entry));
            JXTA_OBJECT_INIT(entry, entry_free, NULL);
            entry->advid = jstring_new_2(advid);
            entry->replica = strcmp(replica, "1") ? FALSE : TRUE;
            sscanf(time_string, jstring_get_string(jInt64_for_format), &entry->timeout);
            jxta_hashtable_put(advsHash, jstring_get_string(entry->advid), strlen(jstring_get_string(entry->advid)) + 1,
                               (Jxta_object *) entry);
        }
        JXTA_OBJECT_RELEASE(advsHash);
        JXTA_OBJECT_RELEASE(entry);
    }
    JXTA_OBJECT_RELEASE(dbSpace);
    dbSpace = NULL;
    if (NULL == dbsHash)
        goto FINAL_EXIT;
    jReplicaAdvIds = jstring_new_0();
    jSRDIAdvIds = jstring_new_0();

    /* get the alias of the db's */
    db_keys = jxta_hashtable_keys_get(dbsHash);
    db_keys_save = db_keys;

    /* create the column for the update */
    jUpdateColumns = jstring_new_2(CM_COL_TimeOut SQL_EQUAL);
    memset(aTmp, 0, sizeof(aTmp));
    apr_snprintf(aTmp, sizeof(aTmp), JPR_ABS_TIME_FMT, ttime);
    jstring_append_2(jUpdateColumns, aTmp);

    /* for each db */
    while (db_keys && *db_keys) {
        Jxta_vector *entries = NULL;
        /* get the databases */
        if (JXTA_SUCCESS != jxta_hashtable_get(dbsHash, *db_keys, strlen(*db_keys) + 1, JXTA_OBJECT_PPTR(&advsHash))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve advsHash status:%d key: %s", status, *db_keys);
            continue;
        }
        /* get all the entries for this database and build a SQL string for the advs */
        entries = jxta_hashtable_values_get(advsHash);
        JXTA_OBJECT_RELEASE(advsHash);
        advsHash = NULL;
        jstring_reset(jReplicaAdvIds, NULL);
        jstring_reset(jSRDIAdvIds, NULL);
        /* get the different db's and their entries */
        for (i = 0; i < jxta_vector_size(entries); i++) {
            jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
            if (entry->replica) {
                if (jstring_length(jReplicaAdvIds) > 0) {
                    jstring_append_2(jReplicaAdvIds, SQL_OR);
                }
                jstring_append_2(jReplicaAdvIds, CM_COL_AdvId SQL_EQUAL);
                SQL_VALUE(jReplicaAdvIds, entry->advid);
            } else {
                if (jstring_length(jSRDIAdvIds) > 0) {
                    jstring_append_2(jSRDIAdvIds, SQL_OR);
                }
                jstring_append_2(jSRDIAdvIds, CM_COL_AdvId SQL_EQUAL);
                SQL_VALUE(jSRDIAdvIds, entry->advid);
            }
            JXTA_OBJECT_RELEASE(entry);
        }
        /* use the alias to get the db */
        dbSpace = cm_dbSpace_by_alias_get(me, *db_keys);
        if (dbSpace) {
            const char *table;
            jstring_reset(jWhere, NULL);
            if (jstring_length(jSRDIAdvIds) > 0) {
                table = CM_TBL_SRDI;
                jstring_append_1(jWhere, jSRDIAdvIds);
            } else {
                table = CM_TBL_REPLICA;
                jstring_append_1(jWhere, jReplicaAdvIds);
            }
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "update %s\n%s\n", table, jstring_get_string(jWhere));
            status = cm_sql_update_with_where(dbSpace, table, jUpdateColumns, jWhere);
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to update %s status:%d\n", table, status);
                goto FINAL_EXIT;
            }
            JXTA_OBJECT_RELEASE(dbSpace);
            dbSpace = JXTA_OBJECT_SHARE(me->bestChoiceDB);
            status = cm_sql_update_with_where(dbSpace, CM_TBL_SRDI_INDEX, jUpdateColumns, jWhere);
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to update SRDI index status:%d\n", status);
                goto FINAL_EXIT;
            }
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "update complete\n");
            JXTA_OBJECT_RELEASE(dbSpace);
            dbSpace = NULL;
        }
        if (entries)
            JXTA_OBJECT_RELEASE(entries);
        free(*db_keys);
        db_keys++;
    }

    if (db_keys_save)
        free(db_keys_save);

  FINAL_EXIT:
    if (dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
    if (dbsHash)
        JXTA_OBJECT_RELEASE(dbsHash);
    if (advsHash)
        JXTA_OBJECT_RELEASE(advsHash);
    if (jWhere)
        JXTA_OBJECT_RELEASE(jWhere);
    if (jColumns)
        JXTA_OBJECT_RELEASE(jColumns);
    if (jUpdateColumns)
        JXTA_OBJECT_RELEASE(jUpdateColumns);
    if (jSRDIAdvIds)
        JXTA_OBJECT_RELEASE(jSRDIAdvIds);
    if (jReplicaAdvIds)
        JXTA_OBJECT_RELEASE(jReplicaAdvIds);
    if (where)
        JXTA_OBJECT_RELEASE(where);

    if (NULL != pool)
        apr_pool_destroy(pool);
    return status;
}

Jxta_status cm_get_replica_entries(Jxta_cm * cm, JString *peer_id_j, Jxta_vector **replicas_v)
{
    Jxta_status status = JXTA_SUCCESS;
    apr_status_t rv;
    int i;
    apr_status_t aprs;
    apr_pool_t *pool;
    apr_dbd_results_t *res = NULL;
    int num_tuples = 0;
    DBSpace *dbSpace = NULL;
    JString *jWhere = NULL;
    JString *jColumns = NULL;
    DBSpace * dbReplica = NULL;

    Jxta_replica_entry *entry;

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", dbSpace->id, aprs);
        goto FINAL_EXIT;
    }

    /* find replica entries of this peerid */
    jColumns = jstring_new_2(CM_COL_DBAlias SQL_COMMA CM_COL_AdvId SQL_COMMA CM_COL_Name);
    jWhere = jstring_new_2(CM_COL_Peerid SQL_EQUAL);
    SQL_VALUE(jWhere, peer_id_j);

    jstring_append_2(jWhere, SQL_AND CM_COL_Replica SQL_EQUAL "\"1\"");

    dbSpace = JXTA_OBJECT_SHARE(cm->bestChoiceDB);


    status = cm_sql_select(dbSpace, pool, CM_TBL_SRDI_INDEX, &res, jColumns, jWhere, NULL, TRUE);
    if (JXTA_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error in select from SRDI_INDEX table %d\n", status);
        goto FINAL_EXIT;
    }
    num_tuples = apr_dbd_num_tuples((apr_dbd_driver_t *) dbSpace->conn->driver, res);
    *replicas_v = jxta_vector_new(num_tuples);
    for (i = 0; i < num_tuples; i++) {
        apr_dbd_results_t *rep_res = NULL;
        apr_dbd_row_t *rep_row = NULL;
        apr_dbd_row_t *row = NULL;
        int rep_num_tuples = 0;

        const char *advid;
        const char *alias;
        const char *name;
        const char *dup_id;

        rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);
        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- cm_get_replica_entries Select failed: %s %i\n",
                            dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
            status = JXTA_FAILED;
            goto FINAL_EXIT;
        }
        alias = apr_dbd_get_entry(dbSpace->conn->driver, row, 0);
        advid = apr_dbd_get_entry(dbSpace->conn->driver, row, 1);
        name = apr_dbd_get_entry(dbSpace->conn->driver, row, 2);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Removing entry advid:%s name:%s dbReplica:%s\n",advid, name, alias != NULL? alias:"(null)");

        dbReplica = cm_dbSpace_by_alias_get(cm, alias);
        if (NULL == dbReplica) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve dbSpace with alias %s\n",alias != NULL? alias:"(null)");
            continue;
        }
        entry = calloc(1, sizeof(Jxta_replica_entry));
        JXTA_OBJECT_INIT(entry, replica_entry_free, NULL);
        entry->db_alias = jstring_new_2(alias);
        entry->adv_id = jstring_new_2(advid);
        entry->name = jstring_new_2(name);

        jstring_reset(jColumns, NULL);
        jstring_append_2(jColumns, CM_COL_DupPeerid);
        jstring_reset(jWhere, NULL);
        jstring_append_2(jWhere, CM_COL_Peerid SQL_EQUAL);
        SQL_VALUE(jWhere, peer_id_j);
        jstring_append_2(jWhere, SQL_AND CM_COL_AdvId SQL_EQUAL);
        SQL_VALUE(jWhere, entry->adv_id);
        jstring_append_2(jWhere, SQL_AND CM_COL_Name SQL_EQUAL);
        SQL_VALUE(jWhere, entry->name);

        cm_sql_select(dbReplica, pool, CM_TBL_REPLICA , &rep_res, jColumns , jWhere, NULL, TRUE);

        rep_num_tuples = apr_dbd_num_tuples((apr_dbd_driver_t *) dbReplica->conn->driver, rep_res);
        if (rep_num_tuples > 0) {
            rv = apr_dbd_get_row(dbReplica->conn->driver, pool, rep_res, &rep_row, -1);
            if (rv) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- cm_get_replica_entries Select failed: %s %i\n",
                            dbSpace->id, apr_dbd_error(dbReplica->conn->driver, dbReplica->conn->sql, rv), rv);
                status = JXTA_FAILED;
                goto FINAL_EXIT;
            }
            dup_id = apr_dbd_get_entry(dbReplica->conn->driver, rep_row, 0);
            entry->dup_id = jstring_new_2(dup_id);
            jxta_vector_add_object_last(*replicas_v, (Jxta_object *) entry);
        }
        JXTA_OBJECT_RELEASE(entry);
        JXTA_OBJECT_RELEASE(dbReplica);
        dbReplica = NULL;
    }

FINAL_EXIT:
    if (jWhere)
        JXTA_OBJECT_RELEASE(jWhere);
    if (jColumns)
        JXTA_OBJECT_RELEASE(jColumns);
    if (dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
    if (dbReplica)
        JXTA_OBJECT_RELEASE(dbReplica);
    if (pool)
        apr_pool_destroy(pool);

    return status;
}

void cm_update_replica_forward_peers(Jxta_cm * cm, Jxta_vector * replica_entries, JString *peer_id_j)
{
    Jxta_status status = JXTA_SUCCESS;
    apr_status_t aprs;
    apr_pool_t *pool=NULL;
    apr_dbd_prepared_t *rep_prepared_statement=NULL;
    apr_dbd_prepared_t *idx_prepared_statement=NULL;
    apr_status_t rv;
    JString *prepare_j=NULL;
    JString *set_prepare_j = NULL;
    DBSpace *dbSpace=NULL;
    DBSpace *indexdbSpace=NULL;
    Jxta_hashtable *dbs_hash=NULL;
    unsigned int i;
    Jxta_replica_entry *entry=NULL;
    Jxta_vector * db_entries_v=NULL;
    char ** dbs_keys=NULL;
    char ** dbs_keys_save=NULL;

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Unable to create apr_pool: %d\n", aprs);
        goto FINAL_EXIT;
    }

    prepare_j = jstring_new_2(SQL_UPDATE);
    jstring_append_2(prepare_j, CM_TBL_SRDI_INDEX);
    set_prepare_j = jstring_new_2(SQL_SET CM_COL_Peerid SQL_EQUAL SQL_VARIABLE SQL_WHERE CM_COL_AdvId SQL_EQUAL SQL_VARIABLE SQL_AND CM_COL_Peerid SQL_EQUAL);
    SQL_VALUE(set_prepare_j, peer_id_j);
    jstring_append_2(set_prepare_j, SQL_AND CM_COL_Name SQL_EQUAL SQL_VARIABLE);
    jstring_append_1(prepare_j, set_prepare_j);

    indexdbSpace = JXTA_OBJECT_SHARE(cm->bestChoiceDB);
    if (NULL != indexdbSpace) {
        rv = apr_dbd_prepare (indexdbSpace->conn->driver, pool, indexdbSpace->conn->sql, jstring_get_string(prepare_j), "update_index_table", &idx_prepared_statement);
        if (0 != rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to prepare SQL query: %s %s\n", apr_dbd_error(indexdbSpace->conn->driver, indexdbSpace->conn->sql, rv), jstring_get_string(prepare_j));
                idx_prepared_statement = NULL;
            }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Received no best choice DB\n");
        goto FINAL_EXIT;
    }

    dbs_hash = jxta_hashtable_new(0);
 
    /* create a vector for each db */
    for (i=0; i<jxta_vector_size(replica_entries); i++) {
        jxta_vector_get_object_at(replica_entries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != jxta_hashtable_get(dbs_hash, jstring_get_string(entry->db_alias), jstring_length(entry->db_alias) + 1, JXTA_OBJECT_PPTR(&db_entries_v))) {
            db_entries_v = jxta_vector_new(0);
            jxta_hashtable_put(dbs_hash, jstring_get_string(entry->db_alias), jstring_length(entry->db_alias) + 1, (Jxta_object *) db_entries_v);
        }
        jxta_vector_add_object_last(db_entries_v, (Jxta_object *) entry);
        JXTA_OBJECT_RELEASE(entry);
        entry = NULL;
        JXTA_OBJECT_RELEASE(db_entries_v);
    }

    dbs_keys = jxta_hashtable_keys_get(dbs_hash);
    dbs_keys_save = dbs_keys;

    while (*dbs_keys) {
        Jxta_boolean locked = FALSE;
        status = jxta_hashtable_get(dbs_hash, *dbs_keys, strlen(*dbs_keys) + 1, JXTA_OBJECT_PPTR(&db_entries_v));
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve DB entry from dbs_hash for key:%s\n", *dbs_keys);
            free(*(dbs_keys++));
            continue;
        }
        jstring_reset(prepare_j, NULL);
        jstring_append_2(prepare_j, SQL_UPDATE CM_TBL_REPLICA);
        jstring_append_1(prepare_j, set_prepare_j);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Preparing statement :%s for db_alias: %s\n",jstring_get_string(prepare_j), *dbs_keys);
        dbSpace = cm_dbSpace_by_alias_get( cm, *dbs_keys);
        if (NULL != dbSpace) {
            apr_thread_mutex_lock(dbSpace->conn->lock);
            locked = TRUE;
            rv = apr_dbd_prepare (dbSpace->conn->driver, pool, dbSpace->conn->sql, jstring_get_string(prepare_j), "update_replica_table", &rep_prepared_statement);
            if (0 != rv) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to prepare SQL query: %s %s\n", apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), jstring_get_string(prepare_j));
                rep_prepared_statement = NULL;
            }
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Received an invalid db alias: %s\n", *dbs_keys);
        }
        if (NULL != rep_prepared_statement && NULL != idx_prepared_statement && NULL != dbSpace) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Updating %d replicas in %s\n", jxta_vector_size(db_entries_v), *dbs_keys);
            for (i=0; i<jxta_vector_size(db_entries_v); i++) {
                int nrows;
                const char * adv_peers[3];

                jxta_vector_get_object_at(db_entries_v, JXTA_OBJECT_PPTR(&entry), i);
                adv_peers[0] = jstring_get_string(entry->peer_id);
                adv_peers[1] = jstring_get_string(entry->adv_id);
                adv_peers[2] = jstring_get_string(entry->name);
                apr_thread_mutex_lock(indexdbSpace->conn->lock);
                rv = apr_dbd_pquery (dbSpace->conn->driver, pool, dbSpace->conn->sql, &nrows, rep_prepared_statement, 3, (const char **) &adv_peers);
                if (0 != rv) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to execute SQL query: %s\n", apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv));
                } else {

                    rv = apr_dbd_pquery (indexdbSpace->conn->driver, pool, indexdbSpace->conn->sql, &nrows, idx_prepared_statement, 3, (const char **) &adv_peers);
                    if (0 != rv) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to update index: %s\n", apr_dbd_error(indexdbSpace->conn->driver, indexdbSpace->conn->sql, rv));
                    }
                }
                apr_thread_mutex_unlock(indexdbSpace->conn->lock);
                JXTA_OBJECT_RELEASE(entry);
                entry = NULL;
            }
        }
        free(*(dbs_keys++));
        if (dbSpace) {
            if (locked)
                apr_thread_mutex_unlock(dbSpace->conn->lock);
            JXTA_OBJECT_RELEASE(dbSpace);
            dbSpace = NULL;
        }
        JXTA_OBJECT_RELEASE(db_entries_v);
        db_entries_v = NULL;
        rep_prepared_statement = NULL;
    }
    free(dbs_keys_save);


FINAL_EXIT:

    if (dbs_hash)
        JXTA_OBJECT_RELEASE(dbs_hash);
    if (dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
    if (indexdbSpace)
        JXTA_OBJECT_RELEASE(indexdbSpace);
    if (prepare_j)
        JXTA_OBJECT_RELEASE(prepare_j);
    if (set_prepare_j)
        JXTA_OBJECT_RELEASE(set_prepare_j);
    if (db_entries_v)
        JXTA_OBJECT_RELEASE(db_entries_v);
    if (entry)
        JXTA_OBJECT_RELEASE(entry);
    if (pool)
        apr_pool_destroy(pool);
}

static void cm_sql_create_delta_where_clause(JString * where, JString * jGroupId, JString * jPeerid, JString * jSourceid, JString * jHandler,
                                             Jxta_SRDIEntryElement * entry)
{
    jstring_append_2(where, CM_COL_Handler SQL_EQUAL);
    SQL_VALUE(where, jHandler);
    jstring_append_2(where, SQL_AND CM_COL_Peerid SQL_EQUAL);
    SQL_VALUE(where, jPeerid);
    jstring_append_2(where, SQL_AND CM_COL_AdvId SQL_EQUAL);
    SQL_VALUE(where, entry->advId);
    jstring_append_2(where, SQL_AND CM_COL_Name SQL_EQUAL);
    SQL_VALUE(where, entry->key);
    jstring_append_2(where, SQL_AND CM_COL_Value SQL_EQUAL);
    SQL_VALUE(where, entry->value);
    jstring_append_2(where, SQL_AND CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(where, jGroupId);
    jstring_append_2(where, SQL_AND CM_COL_SourcePeerid SQL_EQUAL);
    SQL_VALUE(where, jSourceid);
}

Jxta_status cm_save_delta_entry(Jxta_cm * me, JString * jPeerid, JString * jSourcePeerid, JString * jHandler, Jxta_SRDIEntryElement * entry,
                                JString ** jNewValue, Jxta_sequence_number * newSeqNumber, Jxta_boolean * update_srdi, int window)
{
    Jxta_status status = JXTA_SUCCESS;
    apr_status_t rv = 0;
    apr_dbd_results_t *res = NULL;
    DBSpace *dbSpace = NULL;
    JString *jVal = NULL;
    JString *jAlias = NULL;
    JString *sqlval;
    JString *insert_sql = NULL;
    JString *columns = NULL;
    JString *where = NULL;
    const char *seqNumber = NULL;
    const char *value = NULL;
    const char *next_update = NULL;
    const char *expiration = NULL;
    const char *timeout = NULL;
    const char *delta_window = NULL;
    const char *advid = NULL;
    const char *duplicate = NULL;
    Jxta_boolean locked = FALSE;
    apr_dbd_row_t *row = NULL;
    apr_pool_t *pool;
    int nrows = 0;

    JXTA_OBJECT_SHARE(entry);
    *jNewValue = NULL;
    insert_sql = jstring_new_0();
    sqlval = jstring_new_0();
    rv = apr_pool_create(&pool, NULL);
    if (rv != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create apr_pool: %d\n", rv);
        goto FINAL_EXIT;
    }
    status = JXTA_ITEM_NOTFOUND;
    columns = jstring_new_0();

    jstring_append_2(columns, CM_COL_SeqNumber SQL_COMMA CM_COL_Value 
                    SQL_COMMA CM_COL_NextUpdate SQL_COMMA CM_COL_TimeOut 
                    SQL_COMMA CM_COL_TimeOutForOthers SQL_COMMA CM_COL_DeltaWindow
                    SQL_COMMA CM_COL_AdvId SQL_COMMA CM_COL_Duplicate);

    where = jstring_new_0();

    cm_sql_create_delta_where_clause(where, me->jGroupID_string, jPeerid, jSourcePeerid, jHandler, entry);

    dbSpace = NULL;
    dbSpace = JXTA_OBJECT_SHARE(me->bestChoiceDB);
    apr_thread_mutex_lock(dbSpace->conn->lock);

    locked = TRUE;
    rv = cm_sql_select(dbSpace, pool, CM_TBL_SRDI_DELTA, &res, columns, where, NULL, FALSE);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- cm_save_delta_entry Select failed: %s %i\n",
                        dbSpace->conn->log_id, dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
        status = JXTA_FAILED;
        goto FINAL_EXIT;
    }
    rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);
    if (0 == rv) {
        char aTmp[64];
        Jpr_interval_time next_update_time;
        Jpr_interval_time expiration_time;

        /* return indicator that entry was a delta */
        status = JXTA_SUCCESS;
        seqNumber = apr_dbd_get_entry(dbSpace->conn->driver, row, 0);
        value = apr_dbd_get_entry(dbSpace->conn->driver, row, 1);
        next_update = apr_dbd_get_entry(dbSpace->conn->driver, row, 2);
        timeout = apr_dbd_get_entry(dbSpace->conn->driver, row, 3);
        expiration = apr_dbd_get_entry(dbSpace->conn->driver, row, 4);
        delta_window = apr_dbd_get_entry(dbSpace->conn->driver, row, 5);
        advid = apr_dbd_get_entry(dbSpace->conn->driver, row, 6);
        duplicate = apr_dbd_get_entry(dbSpace->conn->driver, row, 7);
        if (NULL == seqNumber || NULL == value) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "db_id: %d %s -- (null) entry in cm_save_delta_entry\n",
                            dbSpace->conn->log_id, dbSpace->id);
        }
        /* don't leak */
        if (NULL != entry->advId) {
            JXTA_OBJECT_RELEASE(entry->advId);
        }
        entry->advId = jstring_new_2(advid);
        /* expired entries should flow immediately */
        if (0 == entry->expiration) {
            *update_srdi = TRUE;
        /* if the entry being saved changed from the delta return a new value */
        } else if (NULL != entry->value && strcmp(value, jstring_get_string(entry->value))) {
            jVal = jstring_clone(entry->value);
            *jNewValue = JXTA_OBJECT_SHARE(jVal);
        } else {
            memset(aTmp, 0, 64);
            Jpr_interval_time nnow = jpr_time_now();
            Jpr_interval_time window_update;
            Jpr_interval_time timeout_time;
            next_update_time = apr_atoi64(next_update);
            expiration_time = apr_atoi64(expiration);
            timeout_time = apr_atoi64(timeout);
            entry->timeout = timeout_time;
            entry->next_update_time = next_update_time;
            entry->delta_window = apr_atoi64(delta_window);
            entry->duplicate = !strcmp(duplicate, "1") ? TRUE:FALSE;
            /* assume no update */
            *update_srdi = FALSE;
            window_update = (timeout_time * ((float)(100-entry->delta_window)/100));

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Save an entry --- time: %" APR_INT64_T_FMT " next_update:%" APR_INT64_T_FMT 
                        " Timeout: %" APR_INT64_T_FMT " diff:%" APR_INT64_T_FMT "\n", nnow, next_update_time, timeout_time, timeout_time - next_update_time);
            if (next_update_time >= nnow) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,"Have to wait for the next update in %" APR_INT64_T_FMT "\n", next_update_time - nnow);
            } else if (next_update_time > (nnow - window_update)) {
                *update_srdi = TRUE;
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,"Need to update it - we're late %" APR_INT64_T_FMT "\n", nnow - next_update_time);
            }
        }
    }

    /* if there was an entry flag a new sequence number */
    if (JXTA_SUCCESS == status) {
        char aTmp[64];
        memset(aTmp, 0, sizeof(aTmp));
        apr_snprintf(aTmp, sizeof(aTmp), JXTA_SEQUENCE_NUMBER_FMT, entry->seqNumber);
        jstring_append_2(where, SQL_AND CM_COL_SeqNumber SQL_EQUAL);
        jstring_append_2(where, seqNumber);
        *newSeqNumber = apr_atoi64(seqNumber);
        goto FINAL_EXIT;
    } else if (status == JXTA_ITEM_NOTFOUND) {

        /* if no entry was found insert the entry */
        jstring_append_2(insert_sql, SQL_INSERT_INTO CM_TBL_SRDI_DELTA SQL_VALUES);
        jAlias = jstring_new_2(dbSpace->alias);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID,"Didn't find sequence %d \n", entry->seqNumber);
        if (0 == entry->seqNumber ) {
                apr_thread_mutex_lock(me->mutex);
                entry->seqNumber = ++me->delta_seq_number;
                apr_thread_mutex_unlock(me->mutex);
        }
        cm_delta_entry_create(me->jGroupID_string, jAlias, jPeerid, jSourcePeerid, jHandler, sqlval, entry, window);
        JXTA_OBJECT_RELEASE(jAlias);
        jAlias = NULL;
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Error searching for srdi delta entry %i \n",
                        dbSpace->conn->log_id, dbSpace->id, status);
        goto FINAL_EXIT;
    }
    jstring_append_1(insert_sql, sqlval);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d %s -- Save srdi delta: %s \n", dbSpace->conn->log_id, dbSpace->id,
                    jstring_get_string(insert_sql));
    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(insert_sql));
    if (rv != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Couldn't insert srdi delta\n %s  rc=%i\n",
                        dbSpace->conn->log_id, dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
    }

  FINAL_EXIT:
    if (locked)
        apr_thread_mutex_unlock(dbSpace->conn->lock);
    if (dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
    if (NULL != pool)
        apr_pool_destroy(pool);
    if (jVal)
        JXTA_OBJECT_RELEASE(jVal);
    JXTA_OBJECT_RELEASE(insert_sql);
    JXTA_OBJECT_RELEASE(sqlval);
    JXTA_OBJECT_RELEASE(entry);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(where);
    return status;
}

static Jxta_status secondary_indexing(Jxta_cm * me, DBSpace * dbSpace, Folder * folder, Jxta_advertisement * adv,
                                      const char *key, Jxta_expiration_time exp, Jxta_expiration_time exp_others,
                                      Jxta_hashtable * srdi_delta)
{
    unsigned int i = 0;
    Jxta_status status = JXTA_SUCCESS;
    Jxta_status statusRet = JXTA_SUCCESS;
    JString *jns = jstring_new_0();
    JString *jval = NULL;
    JString *jskey = NULL;
    JString *jrange = NULL;
    JString *jPkey = jstring_new_2(key);
    char *full_index_name;
    const char *documentName;
    Jxta_SRDIEntryElement *entry = NULL;
    Jxta_vector *sv = NULL;

    documentName = jxta_advertisement_get_document_name(adv);
    jstring_append_2(jns, documentName);

    sv = jxta_advertisement_get_indexes(adv);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID,
                    "db_id: %d Advertisement : SKey:%s Expiration:%" APR_INT64_T_FMT " others:%" APR_INT64_T_FMT "\n",
                    dbSpace->conn->log_id, documentName, exp, exp_others);
    for (i = 0; sv != NULL && i < jxta_vector_size(sv); i++) {
        char *val = NULL;
        const char *range = NULL;
        Jxta_index *ji = NULL;
        Jxta_boolean send_this_srdi = TRUE;
        Jxta_kwd_entry_flags flags;

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
        val = jxta_advertisement_get_string_1(adv, ji, &flags);
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
        statusRet =
            cm_item_insert(dbSpace, CM_TBL_ELEM_ATTRIBUTES, documentName, key, full_index_name, val, range, exp,
                            flags & NO_REPLICATION ? 0:exp_others);
        if (JXTA_SUCCESS != statusRet) {
            send_this_srdi = FALSE;
        }
 
        /* Publish SRDI delta */
        if (exp > 0 && srdi_delta && exp_others > 0 && send_this_srdi) {
            Jxta_sequence_number seq_number;
            apr_thread_mutex_lock(me->mutex);
            seq_number = ++me->delta_seq_number;
            apr_thread_mutex_unlock(me->mutex);
            entry = jxta_srdi_new_element_4(jskey, jval, jns, jPkey, jrange, exp, seq_number, !(flags & NO_REPLICATION));
            if (entry != NULL) {
                cm_srdi_delta_add(srdi_delta, folder, entry);
                JXTA_OBJECT_RELEASE(entry);
            }
        }
        free(full_index_name);
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
    return statusRet;
}

static void cm_srdi_delta_add(Jxta_hashtable * srdi_delta, Folder * folder, Jxta_SRDIEntryElement * entry)
{
    Jxta_status status;
    Jxta_vector *delta_entries;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "add SRDI delta entry: SKey:%s Val:%s\n",
                    jstring_get_string(entry->key), jstring_get_string(entry->value));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, jstring_get_string(fmtTime), entry->expiration);

    status = jxta_hashtable_get(srdi_delta, folder->name, (size_t) strlen(folder->name), JXTA_OBJECT_PPTR(&delta_entries));
    if (status != JXTA_SUCCESS) {
        /* first time we see that value, add a list for it */
        delta_entries = jxta_vector_new(0);
        jxta_hashtable_put(srdi_delta, folder->name, strlen(folder->name), (Jxta_object *) delta_entries);
    }
    status = jxta_vector_add_object_last(delta_entries, (Jxta_object *) entry);
    if (JXTA_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error Adding delta entry %i\n", status);
    }
    JXTA_OBJECT_RELEASE(delta_entries);
}

/* save advertisement */
Jxta_status cm_save(Jxta_cm * self, const char *folder_name, char *primary_key,
                    Jxta_advertisement * adv, Jxta_expiration_time timeOutForMe, Jxta_expiration_time timeOutForOthers,
                    Jxta_boolean update)
{
    Folder *folder;
    Jxta_status status;
    apr_status_t rv;
    apr_status_t aprs;
    apr_pool_t *pool=NULL;
    const char *name;
    DBSpace *dbSpace = NULL;
    DBSpace **dbSpaces = NULL;
    DBSpace **dbSpacesSave = NULL;
    Jxta_hashtable *delta_hash = NULL;
    int retries;

    name = jxta_advertisement_get_document_name(adv);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s primary key: %s\n", name, primary_key);

    status = jxta_hashtable_get(self->folders, folder_name, (size_t) strlen(folder_name), JXTA_OBJECT_PPTR(&folder));
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Not saved - Couldn't retrieve folder %s\n", folder_name);
        return status;
    }
    dbSpaces = cm_dbSpaces_get(self, name, NULL, FALSE);
    dbSpace = *dbSpaces;
    retries = 3;
    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create apr_pool: %d\n", aprs);
        goto CommonExit;
    }
    apr_thread_mutex_lock(dbSpace->conn->lock);
    while (retries--) {
        rv = apr_dbd_transaction_start(dbSpace->conn->driver, pool, dbSpace->conn->sql,
                                       &dbSpace->conn->transaction);

        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s Start transaction failed!\n%s\n",
                            dbSpace->conn->log_id, dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv));
            goto CommonExit;
        }

        status = cm_advertisement_save(dbSpace, primary_key, adv, timeOutForMe, timeOutForOthers, update);

        if (JXTA_SUCCESS != status) {
            Jxta_vector *delta_entries;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "db_id: %d Couldn't save advertisement %i\n",
                            dbSpace->conn->log_id, status);
            apr_dbd_transaction_end(dbSpace->conn->driver, pool, dbSpace->conn->transaction);
            if (JXTA_SUCCESS ==
                jxta_hashtable_get(self->srdi_delta, folder_name, (size_t) strlen(folder_name),
                                   JXTA_OBJECT_PPTR(&delta_entries))) {
                jxta_vector_clear(delta_entries);
                JXTA_OBJECT_RELEASE(delta_entries);
            }
        } else {
            /*
             * Do secondary indexing now.
             */
            if (self->record_delta) {
                delta_hash = self->srdi_delta;
            }
            status = secondary_indexing(self, dbSpace, folder, adv, primary_key, timeOutForMe, timeOutForOthers, delta_hash);

            rv = apr_dbd_transaction_end(dbSpace->conn->driver, pool, dbSpace->conn->transaction);

            if (rv) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d End transaction failed! %d\n%s\n",
                                dbSpace->conn->log_id, rv, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv));
                goto CommonExit;
            }

            if (JXTA_SUCCESS == status) {
                cm_name_space_register(self, dbSpace, name);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "db_id: %d Saved\n", dbSpace->conn->log_id);
                retries = 0;
            }
        }
    }
  CommonExit:
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(folder);
    if (*dbSpaces) {
        dbSpacesSave = dbSpaces;
        dbSpace->conn->transaction = NULL;
        apr_thread_mutex_unlock(dbSpace->conn->lock);
        while (*dbSpaces) {
            JXTA_OBJECT_RELEASE(*dbSpaces);
            dbSpaces++;
        }
        free(dbSpacesSave);
    }
    return status;
}

static void release_task_parms(Jxta_cm_srdi_task * srdi_parms)
{
    JXTA_OBJECT_RELEASE(srdi_parms->self);
    JXTA_OBJECT_RELEASE(srdi_parms->dbSpace);
    JXTA_OBJECT_RELEASE(srdi_parms->handler);
    JXTA_OBJECT_RELEASE(srdi_parms->peerid);
    JXTA_OBJECT_RELEASE(srdi_parms->src_peerid);
    JXTA_OBJECT_RELEASE(srdi_parms->primaryKey);
    JXTA_OBJECT_RELEASE(srdi_parms->entriesV);
    free(srdi_parms);
}

static void *APR_THREAD_FUNC srdi_save_transaction_thread(apr_thread_t * thread, void *arg)
{

    Jxta_cm_srdi_task *srdi_parms = (Jxta_cm_srdi_task *) arg;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ref count starting the thread %d\n",
                    JXTA_OBJECT_GET_REFCOUNT(srdi_parms->self));

    if (srdi_parms->self->available) {
        cm_srdi_transaction_save(srdi_parms);
    }
    release_task_parms(srdi_parms);
    return NULL;
}

static Jxta_status cm_save_srdi_transaction_start(Jxta_cm * self, DBSpace * dbSpace, JString * handler, JString * peerid, JString * src_peerid,
                                                  JString * primaryKey, Jxta_vector * entries, Jxta_boolean bReplica)
{
    Jxta_cm_srdi_task *srdi_task;
    srdi_task = calloc(1, sizeof(struct cm_srdi_transaction_task));
    srdi_task->self = JXTA_OBJECT_SHARE(self);
    srdi_task->dbSpace = JXTA_OBJECT_SHARE(dbSpace);
    srdi_task->handler = JXTA_OBJECT_SHARE(handler);
    srdi_task->peerid = JXTA_OBJECT_SHARE(peerid);
    srdi_task->src_peerid = JXTA_OBJECT_SHARE(src_peerid);
    srdi_task->primaryKey = JXTA_OBJECT_SHARE(primaryKey);
    srdi_task->entriesV = JXTA_OBJECT_SHARE(entries);
    srdi_task->bReplica = bReplica;
    if (NULL != self->thread_pool) {
        apr_thread_pool_push(self->thread_pool, srdi_save_transaction_thread, srdi_task, APR_THREAD_TASK_PRIORITY_NORMAL, self);
    } else {
        cm_srdi_transaction_save(srdi_task);
        release_task_parms(srdi_task);
    }
    return JXTA_SUCCESS;
}

/*
 * Save SRDI elements
 *
 * SQL transactions are created to save the SRDI elements. A transaction threshold
 * is defined in the cache configuration advertisement. It is the number of elements
 * that are saved within a transaction before ending and starting another. This serves
 * two purposes. It reduces the size of transactions and also provides a point to release
 * the lock on the database allowing other threads access to the database.
 *
 */

static Jxta_status cm_srdi_transaction_save(Jxta_cm_srdi_task * task_parms)
{
    apr_status_t rv;
    apr_pool_t *pool=NULL;
    Jxta_status status = JXTA_SUCCESS;
    int retry_count;
    Jxta_cm *self;
    DBSpace *dbSpace;
    Jxta_vector *xactionElements = NULL;

    self = task_parms->self;
    dbSpace = task_parms->dbSpace;
    apr_thread_mutex_lock(dbSpace->conn->lock);
    rv = apr_pool_create(&pool, NULL);
    if (APR_SUCCESS != rv) {
        status = JXTA_NOMEM;
        goto FINAL_EXIT;
    }
    /* iterate through the element vectors - elements are grouped by the DB's */
    while (0 < jxta_vector_size(task_parms->entriesV)) {
        int i = 0;
        Jxta_vector *vElements = NULL;
        Jxta_vector *vElementsSave = NULL;
        jxta_vector_remove_object_at(task_parms->entriesV, JXTA_OBJECT_PPTR(&vElements), 0);
        vElementsSave = vElements;
        rv = apr_dbd_transaction_start(dbSpace->conn->driver, pool, dbSpace->conn->sql,
                                       &dbSpace->conn->transaction);
        if (rv) {
            JXTA_OBJECT_RELEASE(vElements);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d Start transaction failed in save SRDI! %d\n%s\n",
                            dbSpace->conn->log_id, rv, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv));
            goto FINAL_EXIT;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d ----      Start Transaction -- %s vElements size: %d\n", dbSpace->conn->log_id,
                        dbSpace->alias, jxta_vector_size(vElements));
        }
        retry_count = 3;
        /* go through the elements */
        while (NULL != vElements && jxta_vector_size(vElements) > 0 && retry_count > 0 && self->available) {
            Jxta_boolean retry_transaction = FALSE;
            Jxta_boolean retries_exhausted = FALSE;
            Jxta_SRDIEntryElement *entry = NULL;
            jxta_vector_remove_object_at(vElements, JXTA_OBJECT_PPTR(&entry), 0);
            if (NULL == xactionElements) {
                xactionElements = jxta_vector_new(0);
            }
            jxta_vector_add_object_last(xactionElements, (Jxta_object *) entry);
            status = cm_srdi_element_save(dbSpace, task_parms->handler, task_parms->src_peerid, entry, task_parms->bReplica);
            if (JXTA_ITEM_NOTFOUND == status) {
                status = cm_srdi_index_save(self, dbSpace->alias, task_parms->peerid, task_parms->src_peerid, entry, task_parms->bReplica);
            }
            if (JXTA_SUCCESS == status) {
                cm_name_space_register(self, dbSpace, jstring_get_string(entry->nameSpace));
            }
            JXTA_OBJECT_RELEASE(entry);
            if (JXTA_SUCCESS != status && JXTA_ITEM_NOTFOUND != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d Save failed in save element SRDI!\n%s\n",
                                dbSpace->conn->log_id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, status));
                /* retry this transaction */
                if (--retry_count > 0) {
                    retry_transaction = TRUE;
                } else {
                    retries_exhausted = TRUE;
                    retry_transaction = FALSE;
                }
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d ns:%s el:%s  %s Exp: %" APR_INT64_T_FMT "\n",
                                dbSpace->conn->log_id,
                                NULL ==
                                entry->nameSpace ? jstring_get_string(task_parms->primaryKey) : jstring_get_string(entry->
                                                                                                                   nameSpace),
                                NULL == entry->key ? "NoKey" : jstring_get_string(entry->key),
                                NULL == entry->advId ? "noAdvID" : jstring_get_string(entry->advId), entry->expiration);
            }
            /*
               End a transaction if
               -- the transaction threshold is reached
               -- run out of elements
               -- retry
             */
            if (i++ > dbSpace->xaction_threshold || jxta_vector_size(vElements) == 0
                || FALSE == self->available || retry_transaction || retries_exhausted) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d ----      Ending Transaction -- %s\n",
                                dbSpace->conn->log_id, dbSpace->alias);
                rv = apr_dbd_transaction_end(dbSpace->conn->driver, pool, dbSpace->conn->transaction);
                if (rv) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d End transaction failed in save SRDI! %d\n%s\n",
                                    dbSpace->conn->log_id, rv, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv));
                    /* this should not happen - clean up */
                    /* this does happen on a system has an instable disk */
                    if (--retry_count > 0) {
                        retry_transaction = TRUE;
                    } else {
                        retries_exhausted = TRUE;
                        retry_transaction = FALSE;
                    }
                }
                if (NULL != xactionElements && retry_transaction) {
                    unsigned int j;
                    for (j = jxta_vector_size(xactionElements); j > 0; j--) {
                        status = jxta_vector_remove_object_at(xactionElements, JXTA_OBJECT_PPTR(&entry), j - 1);
                        if (JXTA_SUCCESS != status) {
                            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                            "db_id: %d Error remove object from xactionElements %d\n", dbSpace->conn->log_id,
                                            status);
                            goto FINAL_EXIT;
                        } else {
                            /* rebuild vElements */
                            jxta_vector_add_object_first(vElements, (Jxta_object *) entry);
                            JXTA_OBJECT_RELEASE(entry);
                        }
                    }
                    JXTA_OBJECT_RELEASE(xactionElements);
                    xactionElements = NULL;
                }
                dbSpace->conn->transaction = NULL;
                if (jxta_vector_size(vElements) > 0 && self->available && !retry_transaction && !retries_exhausted) {
                    /* unlock the database to allow other threads access and clear the xaction elements */
                    apr_thread_mutex_unlock(dbSpace->conn->lock);
                    i = 0;
                    if (xactionElements) {
                        jxta_vector_clear(xactionElements);
                    }
                    /* lock it again to start a new transaction */
                    apr_thread_mutex_lock(dbSpace->conn->lock);
                    rv = apr_dbd_transaction_start(dbSpace->conn->driver, pool, dbSpace->conn->sql,
                                                   &dbSpace->conn->transaction);
                    if (rv) {
                        JXTA_OBJECT_RELEASE(vElements);
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                        "db_id: %d Start transaction failed inside in save SRDI! error: %d\n%s\n", dbSpace->conn->log_id, rv,
                                        apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv));
                        goto FINAL_EXIT;
                    }
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d ----      Starting Inner Transaction\n",
                                    dbSpace->conn->log_id);
                } else if (retry_transaction && self->available) {
                    /* retry the transaction */
                    rv = apr_dbd_transaction_start(dbSpace->conn->driver, pool, dbSpace->conn->sql,
                                                   &dbSpace->conn->transaction);
                    if (rv) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d Start transaction failed on retry! %d \n%s\n", 
                                        dbSpace->conn->log_id, rv, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv));
                        goto FINAL_EXIT;
                    }
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d ----      Starting Retry Transaction\n",
                                    dbSpace->conn->log_id);
                }
            }
        }
        if (xactionElements)
            JXTA_OBJECT_RELEASE(xactionElements);
        xactionElements = NULL;
        if (vElements)
            JXTA_OBJECT_RELEASE(vElements);
    }
  FINAL_EXIT:
    if (NULL != pool)
        apr_pool_destroy(pool);
    if (xactionElements)
        JXTA_OBJECT_RELEASE(xactionElements);
    if (dbSpace) {
        dbSpace->conn->transaction = NULL;
        apr_thread_mutex_unlock(dbSpace->conn->lock);
    }
    return status;
}

/*
 * Save SRDI elements.
 *
 * Create a vector of element vectors that are grouped by the database in which they're stored.
 *
 * For each database a vector is created that contains the elements for that database. A task is started
 * that will save the elements in the appropriate database.
 * 
 */
static Jxta_status cm_srdi_elements_save_priv(Jxta_cm * me, JString * handler, JString * peerid, JString * src_peerid, JString * primaryKey,
                                              Jxta_vector * entries, Jxta_boolean bReplica, Jxta_vector ** resendEntries)
{
    Jxta_status status = JXTA_SUCCESS;
    int i;
    DBSpace *dbSpace = NULL;
    DBSpace **dbSpaces = NULL;
    Jxta_hashtable *entriesHash = NULL;
    Jxta_hashtable *dbsHash = NULL;
    char **keys;
    char **keysSave;
    Jxta_vector *nsEntries = NULL;
    if (NULL == entries || 0 == jxta_vector_size(entries))
        return JXTA_INVALID_ARGUMENT;

    entriesHash = jxta_hashtable_new(0);
    dbsHash = jxta_hashtable_new(0);

    /* group the entries by their name space */
    for (i = 0; i < jxta_vector_size(entries); i++) {
        const char *nameSpace = NULL;
        Jxta_SRDIEntryElement *entry = NULL;
        status = jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve element from entries %i\n", status);
            continue;
        }
        if (entry->expanded && entry->seqNumber > 0) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,
                            "Update sequence number " JXTA_SEQUENCE_NUMBER_FMT "\n", entry->seqNumber);
            status = cm_srdi_seq_number_update(me, peerid, entry);
            if (JXTA_ITEM_NOTFOUND == status && NULL != resendEntries && entry->expiration > 0) {
                Jxta_SRDIEntryElement *rEntry = NULL;
                if (NULL == *resendEntries) {
                    *resendEntries = jxta_vector_new(0);
                }
                rEntry = jxta_srdi_new_element_resend(entry->seqNumber);
                jxta_vector_add_object_last(*resendEntries, (Jxta_object *) rEntry);
                JXTA_OBJECT_RELEASE(rEntry);
            }
            JXTA_OBJECT_RELEASE(entry);
            continue;
        }
        if (NULL == entry->nameSpace) {
            nameSpace = jstring_get_string(primaryKey);
        } else {
            nameSpace = jstring_get_string(entry->nameSpace);
        }
        status = jxta_hashtable_get(entriesHash, nameSpace, strlen(nameSpace) + 1, JXTA_OBJECT_PPTR(&nsEntries));
        if (JXTA_SUCCESS != status) {
            nsEntries = jxta_vector_new(0);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "adding %s  with a length of %i\n", nameSpace,
                            strlen(nameSpace) + 1);
            jxta_hashtable_put(entriesHash, nameSpace, strlen(nameSpace) + 1, (Jxta_object *) nsEntries);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Already had an entry %s\n", nameSpace);
        }
        status = jxta_vector_add_object_last(nsEntries, (Jxta_object *) entry);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error adding element to the nsEntries %i\n", status);
        }
        JXTA_OBJECT_RELEASE(entry);
        JXTA_OBJECT_RELEASE(nsEntries);
        nsEntries = NULL;
    }

    keys = jxta_hashtable_keys_get(entriesHash);
    keysSave = keys;
    /* match the entries to a db */
    while (*keys) {
        Jxta_vector *entriesV;
        DBSpace **dbSpacesSave;
        status = jxta_hashtable_get(entriesHash, *keys, strlen(*keys) + 1, JXTA_OBJECT_PPTR(&nsEntries));
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve entries with key %s %i\n", *keys, status);
            free(*keys);
            keys++;
            continue;
        }
        dbSpaces = cm_dbSpaces_get(me, *keys, NULL, FALSE);
        status = jxta_hashtable_get(dbsHash, (*dbSpaces)->alias, strlen((*dbSpaces)->alias) + 1, JXTA_OBJECT_PPTR(&entriesV));
        if (JXTA_SUCCESS != status) {
            entriesV = jxta_vector_new(0);
            jxta_hashtable_put(dbsHash, (*dbSpaces)->alias, strlen((*dbSpaces)->alias) + 1, (Jxta_object *) entriesV);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Adding entries to db_id: %d \n", (*dbSpaces)->conn->log_id);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Already had an entry in db_id: %d\n", (*dbSpaces)->conn->log_id);
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d the size of nsEntries %i\n", (*dbSpaces)->conn->log_id,
                        jxta_vector_size(nsEntries));
        jxta_vector_add_object_last(entriesV, (Jxta_object *) nsEntries);
        dbSpacesSave = dbSpaces;
        while (*dbSpaces) {
            JXTA_OBJECT_RELEASE(*dbSpaces);
            dbSpaces++;
        }
        free(dbSpacesSave);
        JXTA_OBJECT_RELEASE(nsEntries);
        nsEntries = NULL;
        JXTA_OBJECT_RELEASE(entriesV);
        free(*keys);
        keys++;
    }
    if (NULL != nsEntries)
        JXTA_OBJECT_RELEASE(nsEntries);
    free(keysSave);
    /* cycle through the DB's */
    for (i = 0; i < jxta_vector_size(me->dbSpaces); i++) {
        Jxta_vector *entriesV = NULL;

        status = jxta_vector_get_object_at(me->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve dbSpace %i\n", status);
            continue;
        }
        status = jxta_hashtable_get(dbsHash, dbSpace->alias, strlen(dbSpace->alias) + 1, JXTA_OBJECT_PPTR(&entriesV));
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No entries for db_id: %d\n", dbSpace->conn->log_id);
            JXTA_OBJECT_RELEASE(dbSpace);
            dbSpace = NULL;
            status = JXTA_SUCCESS;
            continue;
        }
        cm_save_srdi_transaction_start(me, dbSpace, handler, peerid, src_peerid, primaryKey, entriesV, bReplica);
        if (entriesV)
            JXTA_OBJECT_RELEASE(entriesV);
        if (dbSpace) {
            JXTA_OBJECT_RELEASE(dbSpace);
        }
        status = JXTA_SUCCESS;
    }

    JXTA_OBJECT_RELEASE(dbsHash);
    JXTA_OBJECT_RELEASE(entriesHash);
    return status;
}

Jxta_status cm_get_resend_delta_entries(Jxta_cm * me, JString * peerid_j, Jxta_vector * resendEntries, Jxta_hashtable ** resend_entries)
{
    int i;
    Jxta_status status = JXTA_SUCCESS;
    apr_pool_t *pool;
    apr_status_t aprs;
    apr_dbd_results_t *res = NULL;
    JString *columns = NULL;
    JString *where = NULL;
    DBSpace *dbSpace = NULL;
    dbSpace = JXTA_OBJECT_SHARE(me->bestChoiceDB);

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create apr_pool: %d\n", aprs);
        goto FINAL_EXIT;
    }
    columns =
        jstring_new_2(CM_COL_AdvId SQL_COMMA CM_COL_Name SQL_COMMA CM_COL_Value SQL_COMMA CM_COL_NumRange 
                        SQL_COMMA CM_COL_TimeOutForOthers SQL_COMMA CM_COL_NameSpace SQL_COMMA CM_COL_SeqNumber 
                        SQL_COMMA CM_COL_Duplicate SQL_COMMA CM_COL_SourcePeerid);
    where = jstring_new_0();
    for (i = 0; i < jxta_vector_size(resendEntries); i++) {
        apr_dbd_row_t *row;
        char aTmp[64];
        Jxta_SRDIEntryElement *entry;
        Jxta_expiration_time timeout;
        apr_status_t rv;
        int nb_keys = 0;

        status = jxta_vector_get_object_at(resendEntries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != status) {
            continue;
        }
        jstring_reset(where, NULL);
        if (entry->seqNumber > 0) {
        jstring_append_2(where, CM_COL_SeqNumber SQL_EQUAL);
        memset(aTmp, 0, sizeof(aTmp));
        apr_snprintf(aTmp, sizeof(aTmp), JXTA_SEQUENCE_NUMBER_FMT, entry->seqNumber);
        jstring_append_2(where, aTmp);
        } else if (entry->advId) {
            jstring_append_2(where, CM_COL_AdvId SQL_EQUAL);
            SQL_VALUE(where, entry->advId);
            jstring_append_2(where, SQL_AND CM_COL_Peerid SQL_EQUAL);
            SQL_VALUE(where, peerid_j);
            if (NULL != entry->sn_cs_values) {
                JString **sn_ptr = NULL;
                JString **sn_ptr_save = NULL;
                Jxta_boolean first = TRUE;

                jstring_append_2(where, SQL_INTERSECT SQL_SELECT);
                jstring_append_1(where, columns);
                jstring_append_2(where, SQL_FROM CM_TBL_SRDI_DELTA SQL_WHERE);
                sn_ptr = tokenize_string_j(entry->sn_cs_values, ',');
                sn_ptr_save = sn_ptr;
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Update entries - %s \n", jstring_get_string(entry->sn_cs_values));

                while (*sn_ptr) {
                    if (!first) {
                        jstring_append_2(where, SQL_OR);
                    }
                    first = FALSE;
                    jstring_append_2(where, CM_COL_SeqNumber SQL_EQUAL);
                    jstring_append_1(where, *sn_ptr);
                    JXTA_OBJECT_RELEASE(*sn_ptr);
                    sn_ptr++;
                }
                free(sn_ptr_save);
            }
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid entry received in SRDI resend Entry no-seq number or advid\n");
            JXTA_OBJECT_RELEASE(entry);
            continue;
        }
        rv = cm_sql_select(dbSpace, pool, CM_TBL_SRDI_DELTA, &res, columns, where, NULL, TRUE);
        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- cm_save_delta_entry Select failed: %s %i\n",
                            dbSpace->conn->log_id, dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
            status = JXTA_FAILED;
            goto FINAL_EXIT;
        }
        nb_keys = apr_dbd_num_tuples((apr_dbd_driver_t *) dbSpace->conn->driver, res);
        while (nb_keys > 0) {
            JString *jAdvID = NULL;
            JString *jName = NULL;
            JString *jValue = NULL;
            JString *jRange = NULL;
            const char *timeForOthers = 0;
            const char *duplicate;
            const char *source_id;
            JString *jNameSpace = NULL;
            Jxta_sequence_number seq_no;
            Jxta_SRDIEntryElement *newEntry = NULL;
            Jxta_vector *entries = NULL;

            rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);
            status = JXTA_SUCCESS;
            jAdvID = jstring_new_2(apr_dbd_get_entry(dbSpace->conn->driver, row, 0));
            jName = jstring_new_2(apr_dbd_get_entry(dbSpace->conn->driver, row, 1));
            jValue = jstring_new_2(apr_dbd_get_entry(dbSpace->conn->driver, row, 2));
            jRange = jstring_new_2(apr_dbd_get_entry(dbSpace->conn->driver, row, 3));
            timeForOthers = apr_dbd_get_entry(dbSpace->conn->driver, row, 4);
            jNameSpace = jstring_new_2(apr_dbd_get_entry(dbSpace->conn->driver, row, 5));
            timeout = apr_atoi64(timeForOthers);
            seq_no = apr_atoi64(apr_dbd_get_entry(dbSpace->conn->driver, row, 6));
            duplicate = apr_dbd_get_entry(dbSpace->conn->driver, row, 7);
            source_id = apr_dbd_get_entry(dbSpace->conn->driver, row, 8);

            newEntry = jxta_srdi_new_element_3(jName, jValue, jNameSpace, jAdvID, jRange, timeout, entry->seqNumber == 0 ? seq_no : entry->seqNumber );
            newEntry->duplicate = !strcmp(duplicate, "1") ? TRUE:FALSE;

            if (NULL == *resend_entries) {
                *resend_entries = jxta_hashtable_new(0);
            }
            if (JXTA_SUCCESS != jxta_hashtable_get(*resend_entries, source_id, strlen(source_id) + 1, JXTA_OBJECT_PPTR(&entries))) {
                entries = jxta_vector_new(0);
                jxta_hashtable_put(*resend_entries, source_id, strlen(source_id) + 1, (Jxta_object *) entries);
            }
            jxta_vector_add_object_last(entries, (Jxta_object *) newEntry);
            JXTA_OBJECT_RELEASE(newEntry);
            JXTA_OBJECT_RELEASE(jAdvID);
            JXTA_OBJECT_RELEASE(jName);
            JXTA_OBJECT_RELEASE(jValue);
            JXTA_OBJECT_RELEASE(jRange);
            JXTA_OBJECT_RELEASE(jNameSpace);
            JXTA_OBJECT_RELEASE(entries);
            nb_keys--;
        }
        JXTA_OBJECT_RELEASE(entry);
        if (NULL == *resend_entries) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d sequence resend did not find %s\n", dbSpace->conn->log_id,jstring_get_string(where));
        }
    }
  FINAL_EXIT:
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(dbSpace);
    return status;
}

Jxta_status cm_save_srdi(Jxta_cm * self, JString * handler, JString * peerid, JString * primaryKey, Jxta_SRDIEntryElement * entry)
{
    Jxta_status status;
    JXTA_DEPRECATED_API();

    if (primaryKey) {
    }
    status = cm_srdi_element_save(self->defaultDB, handler, peerid, entry, FALSE);
    if (JXTA_SUCCESS == status) {
        cm_name_space_register(self, self->defaultDB, jstring_get_string(entry->nameSpace));
    }
    return status;
}

/* save srdi entries in the SRDI table */
Jxta_status cm_save_srdi_elements(Jxta_cm * me, JString * handler, JString * peerid, JString * src_peerid, JString * primaryKey,
                                  Jxta_vector * entries, Jxta_vector ** resendEntries)
{
    Jxta_status status;
    status = cm_srdi_elements_save_priv(me, handler, peerid, src_peerid, primaryKey, entries, FALSE, resendEntries);
    return status;
}

/* save srdi entries in the replica table */
Jxta_status cm_save_replica_elements(Jxta_cm * me, JString * handler, JString * peerid, JString * primaryKey,
                                     Jxta_vector * entries, Jxta_vector ** resendEntries)
{
    Jxta_status status;
    status = cm_srdi_elements_save_priv(me, handler, peerid, peerid, primaryKey, entries, TRUE, resendEntries);
    return status;
}


/* search for advertisements with specific (attribute,value) pairs */
/* a NULL pointer means the end of the list */
char **cm_search(Jxta_cm * self, char *folder_name, const char *aattribute, const char *value, Jxta_boolean no_locals, int n_adv)
{
    char **all_primaries;

    JString *where = jstring_new_0();
    JString *jAttr = jstring_new_2(aattribute);
    JString *jVal = jstring_new_2(value);
    JString *jGroup = jstring_new_0();

    jstring_append_2(where, CM_COL_Name);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jAttr);
    if (no_locals) {
        jstring_append_2(where, SQL_AND CM_COL_TimeOutForOthers SQL_GREATER_THAN "0");
    }
    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, CM_COL_Value);
    if (cm_sql_escape_and_wc_value(jVal, TRUE)) {
        jstring_append_2(where, SQL_LIKE);
    } else {
        jstring_append_2(where, SQL_EQUAL);
    }
    SQL_VALUE(where, jVal);
    jstring_append_2(jGroup, SQL_GROUP);
    jstring_append_2(jGroup, CM_COL_AdvId);

    all_primaries = cm_sql_get_primary_keys(self, folder_name, CM_TBL_ELEM_ATTRIBUTES, where, jGroup, n_adv);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "got a query for attr - %s  val - %s \n", aattribute, value);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jAttr);
    JXTA_OBJECT_RELEASE(jVal);
    JXTA_OBJECT_RELEASE(jGroup);

    return all_primaries;
}

/* search for advertisements with specific (attribute,value) pairs */
/* a NULL pointer means the end of the list */
char **cm_search_srdi(Jxta_cm * self, char *folder_name, const char *aattribute, const char *value, int n_adv)
{
    char **all_primaries;

    JString *where = jstring_new_0();
    JString *jAttr = jstring_new_2(aattribute);
    JString *jVal = jstring_new_2(value);
    JString *jGroup = jstring_new_0();

    jstring_append_2(where, CM_COL_Name);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jAttr);

    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, CM_COL_Value);
    if (cm_sql_escape_and_wc_value(jVal, TRUE)) {
        jstring_append_2(where, SQL_LIKE);
    } else {
        jstring_append_2(where, SQL_EQUAL);
    }
    SQL_VALUE(where, jVal);
    jstring_append_2(jGroup, SQL_GROUP);
    jstring_append_2(jGroup, CM_COL_AdvId);

    all_primaries = cm_sql_get_primary_keys(self, folder_name, CM_TBL_SRDI, where, jGroup, n_adv);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "got a query for attr - %s  val - %s \n", aattribute, value);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jAttr);
    JXTA_OBJECT_RELEASE(jVal);
    JXTA_OBJECT_RELEASE(jGroup);

    return all_primaries;
}

Jxta_advertisement **cm_sql_query(Jxta_cm * self, const char *folder_name, JString * where)
{
    Jxta_advertisement **results = NULL;
    Jxta_status status = JXTA_SUCCESS;
    DBSpace *dbSpace = self->defaultDB;
    Jxta_advertisement *newAdv = NULL;
    Jxta_vector *res_vect = NULL;
    Jxta_advertisement *res_adv = NULL;
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

    rv = cm_sql_select_join(dbSpace, pool, &res, where);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- jxta_cm_sql_query Select failed: %s %i\n",
                        dbSpace->conn->log_id, dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
        goto finish;
    }
    nTuples = apr_dbd_num_tuples(dbSpace->conn->driver, res);
    if (nTuples == 0)
        goto finish;
    results = (Jxta_advertisement **) malloc(sizeof(Jxta_advertisement *) * (nTuples + 1));
    i = 0;
    for (rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);
         rv == 0; rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1)) {
        const char *advId;

        advId = apr_dbd_get_entry(dbSpace->conn->driver, row, 0);
        entry = apr_dbd_get_entry(dbSpace->conn->driver, row, 1);
        if (entry == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "db_id: %d %s -- (null) in entry from jxta_cm_sql_query",
                            dbSpace->conn->log_id, dbSpace->id);
            continue;
        } else {
            char *tmp;
            int len;
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
                jxta_vector_get_object_at(res_vect, JXTA_OBJECT_PPTR(&res_adv), (unsigned int) 0);
                jxta_advertisement_set_local_id_string(res_adv, advId);
                results[i++] = res_adv;
                res_adv = NULL;
            }
            JXTA_OBJECT_RELEASE(res_vect);
            res_vect = NULL;
        }

    }
    if (results != NULL) {
        results[i] = NULL;
    }

  finish:
    JXTA_OBJECT_RELEASE(columns);
    if (NULL != pool)
        apr_pool_destroy(pool);

    return results;
}

Jxta_cache_entry **cm_query_ctx(Jxta_cm * me, Jxta_credential ** scope, int threshold, Jxta_query_context * jContext)
{
    Jxta_cache_entry **resultsReturn = NULL;
    DBSpace **dbSpaces = NULL;
    DBSpace **dbSpacesSave = NULL;
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    apr_dbd_results_t *res = NULL;
    JString *jSQLcmd = NULL;
    JString *jJoin = NULL;
    JString *jGroup = NULL;
    Jxta_cache_entry **addsStart = NULL;
    Jxta_boolean needOR = FALSE;
    JString *lastGroup = NULL;
    int groups = 0;
    int rv;
    const char *ns = NULL;
    JString *columns = jstring_new_0();

    jJoin = jstring_new_0();

    /* the advid in the first column is used by cm_proffer_advertisements_build to populate the profid in Jxta_cache_entry */
    jstring_append_2(jJoin, CM_COL_JOIN SQL_DOT CM_COL_AdvId);
    jstring_append_2(jJoin, SQL_COMMA CM_COL_SRC SQL_DOT CM_COL_NameSpace);
    jstring_append_2(jJoin, SQL_COMMA CM_COL_SRC SQL_DOT CM_COL_TimeOut SQL_COMMA CM_COL_SRC SQL_DOT CM_COL_TimeOutForOthers);
    jstring_append_2(jJoin, SQL_COMMA CM_COL_Advert);
    jstring_append_2(jJoin, SQL_FROM CM_TBL_ELEM_ATTRIBUTES_SRC SQL_JOIN);
    jstring_append_2(jJoin, CM_TBL_ADVERTISEMENTS_JOIN SQL_ON);
    jstring_append_2(jJoin, CM_COL_SRC SQL_DOT CM_COL_NameSpace SQL_EQUAL CM_COL_JOIN SQL_DOT CM_COL_NameSpace);
    jstring_append_2(jJoin, SQL_AND CM_COL_SRC SQL_DOT CM_COL_AdvId SQL_EQUAL CM_COL_JOIN SQL_DOT CM_COL_AdvId);

    /* concatenate groupIDs within the scope of the query */
    if (me->sharedDB) {
        jstring_append_2(jJoin, SQL_AND CM_COL_SRC SQL_DOT CM_COL_GroupID SQL_EQUAL CM_COL_JOIN SQL_DOT CM_COL_GroupID);
    }

    jSQLcmd = jstring_new_2(jstring_get_string(jContext->sqlcmd));

    while (NULL != scope && *scope && me->sharedDB) {
        JString *jScope;
        Jxta_id *jId;
        jxta_credential_get_peergroupid(*scope, &jId);
        jxta_id_get_uniqueportion(jId, &jScope);
        JXTA_OBJECT_RELEASE(jId);
        /* no need to add the group of this cm */
        if (0 == jstring_equals(jScope, me->jGroupID_string)) {
            JXTA_OBJECT_RELEASE(jScope);
            scope++;
            continue;
        }
        if (needOR) {
            jstring_append_2(jSQLcmd, SQL_OR);
        } else {
            jstring_append_2(jSQLcmd, SQL_AND SQL_LEFT_PAREN);
        }
        needOR = TRUE;
        jstring_append_2(jSQLcmd, CM_COL_SRC SQL_DOT CM_COL_GroupID SQL_EQUAL);
        SQL_VALUE(jSQLcmd, jScope);
        scope++;
        JXTA_OBJECT_RELEASE(jScope);
    }
    if (needOR) {
        jstring_append_2(jSQLcmd, SQL_OR CM_COL_SRC SQL_DOT CM_COL_GroupID SQL_EQUAL);
        SQL_VALUE(jSQLcmd, me->jGroupID_string);
        jstring_append_2(jSQLcmd, SQL_RIGHT_PAREN);
    }
    jGroup = jstring_new_2(SQL_GROUP CM_COL_SRC SQL_DOT CM_COL_AdvId);

    if (NULL != jContext->queryNameSpace) {
        ns = jstring_get_string(jContext->queryNameSpace);
    }
    dbSpaces = cm_dbSpaces_get(me, ns, scope, FALSE);
    dbSpacesSave = dbSpaces;
    jstring_append_2(columns, CM_COL_Advert);

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create apr_pool: %d\n", aprs);
        goto FINAL_EXIT;
    }
    while (*dbSpaces) {
        Jxta_cache_entry **tempAdds = NULL;
        rv = cm_sql_select_join_generic(*dbSpaces, pool, jJoin, jGroup, &res, jSQLcmd, needOR ? FALSE : TRUE);
        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- cm_query_ctx Select join failed: %s %i\n",
                            (*dbSpaces)->conn->log_id, (*dbSpaces)->id, apr_dbd_error((*dbSpaces)->conn->driver,
                                                                                      (*dbSpaces)->conn->sql, rv), rv);
            goto FINAL_EXIT;
        }
        tempAdds = cm_advertisements_build(*dbSpaces, pool, res);

        if (NULL != tempAdds) {
            addsStart = (Jxta_cache_entry **) cm_arrays_concatenate((Jxta_object **) addsStart, (Jxta_object **) tempAdds);
            free(tempAdds);
            if (NULL == lastGroup || jstring_equals(lastGroup, (*dbSpaces)->jId) > 0) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Incrementing groups when change and results groups: %d\n",
                                groups);
                groups++;
            }
        }
        if (NULL != lastGroup) {
            JXTA_OBJECT_RELEASE(lastGroup);
            lastGroup = NULL;
        }
        if (groups >= threshold) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Threshold of %d reached in cm_query_ctx\n", threshold);
            break;
        }
        lastGroup = JXTA_OBJECT_SHARE((*dbSpaces)->jId);
        JXTA_OBJECT_RELEASE(*dbSpaces);
        dbSpaces++;
    }
    resultsReturn = addsStart;
  FINAL_EXIT:
    while (*dbSpaces) {
        JXTA_OBJECT_RELEASE(*dbSpaces);
        dbSpaces++;
    }
    if (lastGroup)
        JXTA_OBJECT_RELEASE(lastGroup);
    if (jSQLcmd)
        JXTA_OBJECT_RELEASE(jSQLcmd);
    if (jJoin)
        JXTA_OBJECT_RELEASE(jJoin);
    if (jGroup)
        JXTA_OBJECT_RELEASE(jGroup);
    if (dbSpacesSave)
        free(dbSpacesSave);
    JXTA_OBJECT_RELEASE(columns);
    if (NULL != pool)
        apr_pool_destroy(pool);

    return resultsReturn;
}

static Jxta_cache_entry **cm_advertisements_build(DBSpace * dbSpace, apr_pool_t * pool, apr_dbd_results_t * res)
{
    Jxta_status status;
    const char *nameSpace;
    const char *lifetime;
    const char *expiration;
    const char *adv = NULL;
    const char *advid = NULL;
    int nTuples, i, rv;
    Jxta_cache_entry **results = NULL;
    Jxta_cache_entry **ret = NULL;
    Jxta_cache_entry *cache_entry = NULL;
    apr_dbd_row_t *row = NULL;
    Jxta_advertisement *jAdv = NULL;
    int column_count;

    /**
    *    Column Order
    *    ------------
    *    AdvId
    *    NameSpace
    *    Timeout
    *    TimeoutForOthers
    *    Advertisement
    **/

    nTuples = apr_dbd_num_tuples(dbSpace->conn->driver, res);
    if (nTuples == 0)
        return NULL;
    column_count = apr_dbd_num_cols(dbSpace->conn->driver, res);
    assert(column_count == 5);
    ret = calloc(nTuples + 1, sizeof(struct cache_entry *));
    if (NULL == ret) 
        goto FINAL_EXIT;
    results = ret;
    i = 0;
    for (rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);
         rv == 0; rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1)) {
        if (NULL != cache_entry) {
            JXTA_OBJECT_RELEASE(cache_entry);
        }
        cache_entry = calloc(1, sizeof(Jxta_cache_entry));
        if (NULL == cache_entry) {
            goto FINAL_EXIT;
        }
        JXTA_OBJECT_INIT(cache_entry, cache_entry_free, NULL);
        advid = apr_dbd_get_entry(dbSpace->conn->driver, row, 0);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Adv ID: %s\n", dbSpace->conn->log_id,
                        advid ? advid : "NULL");
        if (advid == NULL) {
            continue;
        }
        nameSpace = apr_dbd_get_entry(dbSpace->conn->driver, row, 1);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Namespace: %s\n", dbSpace->conn->log_id,
                        nameSpace ? nameSpace : "NULL");
        if (nameSpace == NULL) {
            continue;
        }
        lifetime = apr_dbd_get_entry(dbSpace->conn->driver, row, 2);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Timeout: %s\n", dbSpace->conn->log_id,
                        lifetime ? lifetime : "NULL");
        if (lifetime == NULL) {
            continue;
        }
        expiration = apr_dbd_get_entry(dbSpace->conn->driver, row, 3);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Expiration: %s\n", dbSpace->conn->log_id,
                        expiration ? expiration : "NULL");
        if (expiration == NULL) {
            continue;
        }
        adv = apr_dbd_get_entry(dbSpace->conn->driver, row, 4);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Adv: %s\n", dbSpace->conn->log_id, adv ? adv : "NULL");
        if (adv == NULL) {
            continue;
        }
        jAdv = jxta_advertisement_new();
        if (NULL == jAdv) {
            goto FINAL_EXIT;
        }
        status = jxta_advertisement_parse_charbuffer(jAdv, adv, strlen(adv));
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d Unable to parse Adv: %s\n", dbSpace->conn->log_id,
                            adv ? adv : "NULL");
            JXTA_OBJECT_RELEASE(jAdv);
            jAdv = NULL;
            continue;
        }
        cache_entry->adv = jAdv;
        jAdv = NULL;
        sscanf(lifetime, jstring_get_string(jInt64_for_format), &cache_entry->lifetime);
        sscanf(expiration, jstring_get_string(jInt64_for_format), &cache_entry->expiration);
        *(results++) = cache_entry;
        cache_entry = NULL;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "db_id: %d Add advertisement with advid %s\n",
                        dbSpace->conn->log_id, NULL == advid ? "(null)" : advid);
    }
    *(results) = NULL;
  FINAL_EXIT:
    if (NULL != cache_entry)
        JXTA_OBJECT_RELEASE(cache_entry);
    if (jAdv)
        JXTA_OBJECT_RELEASE(jAdv);
    return ret;
}


static void cm_proffer_advertisements_columns_build(JString * columns)
{
    /* these are the columns required by cm_proffer_advertisements_build CM_COL_Peerid is used to populate the profid in Jxta_cache_entry  */
    jstring_append_2(columns,
                     CM_COL_Peerid SQL_COMMA CM_COL_AdvId SQL_COMMA CM_COL_Name SQL_COMMA CM_COL_Value SQL_COMMA
                     CM_COL_NameSpace);
    jstring_append_2(columns, SQL_COMMA CM_COL_TimeOut SQL_COMMA CM_COL_TimeOutForOthers);
}

static Jxta_cache_entry **cm_proffer_advertisements_build(DBSpace * dbSpace, apr_pool_t * pool, apr_dbd_results_t * res,
                                                          Jxta_boolean adv_present)
{
    Jxta_status status;
    const char *name;
    const char *value;
    const char *profid;
    const char *nameSpace;
    const char *lifetime;
    const char *expiration;
    const char *adv = NULL;
    const char *advid = NULL;
    const char *tmpid = NULL;
    const char *tmpPid = NULL;
    int nTuples, i, rv;
    Jxta_cache_entry **results = NULL;
    Jxta_cache_entry **ret = NULL;
    Jxta_cache_entry *cache_entry = NULL;
    apr_dbd_row_t *row = NULL;
    Jxta_ProfferAdvertisement *profferAdv = NULL;
    Jxta_advertisement *jAdv = NULL;
    int column_count;
    /**
    *    Column Order
    *    ------------
    *    Peerid or AdvId (Determined by the columns returned)
    *    AdvId
    *    Name
    *    Value
    *    NameSpace
    *    Timeout
    *    TimeoutForOthers
    *    Advertisement (optional)
    **/

    nTuples = apr_dbd_num_tuples(dbSpace->conn->driver, res);
    if (nTuples == 0)
        return NULL;
    column_count = apr_dbd_num_cols(dbSpace->conn->driver, res);
    adv_present == TRUE ? assert(column_count == 8) : assert(column_count == 7);
    ret = calloc(nTuples + 1, sizeof(struct cache_entry *));
    if (NULL == ret)
        goto FINAL_EXIT;
    results = ret;
    i = 0;
    for (rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);
         rv == 0; rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1)) {
        if (NULL == cache_entry) {
            cache_entry = calloc(1, sizeof(Jxta_cache_entry));
            JXTA_OBJECT_INIT(cache_entry, cache_entry_free, NULL);
        }
        profid = apr_dbd_get_entry(dbSpace->conn->driver, row, 0);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d profid ID: %s\n", dbSpace->conn->log_id,
                        profid ? profid : "NULL");
        if (profid == NULL) {
            continue;
        }
        advid = apr_dbd_get_entry(dbSpace->conn->driver, row, 1);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Adv ID: %s\n", dbSpace->conn->log_id,
                        advid ? advid : "NULL");
        if (advid == NULL) {
            continue;
        }
        name = apr_dbd_get_entry(dbSpace->conn->driver, row, 2);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Name: %s\n", dbSpace->conn->log_id, name ? name : "NULL");
        if (name == NULL) {
            continue;
        }
        value = apr_dbd_get_entry(dbSpace->conn->driver, row, 3);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Value: %s\n", dbSpace->conn->log_id,
                        value ? value : "NULL");
        if (value == NULL) {
            continue;
        }
        nameSpace = apr_dbd_get_entry(dbSpace->conn->driver, row, 4);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Namespace: %s\n", dbSpace->conn->log_id,
                        nameSpace ? nameSpace : "NULL");
        if (nameSpace == NULL) {
            continue;
        }
        lifetime = apr_dbd_get_entry(dbSpace->conn->driver, row, 5);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Timeout: %s\n", dbSpace->conn->log_id,
                        lifetime ? lifetime : "NULL");
        if (lifetime == NULL) {
            continue;
        }
        expiration = apr_dbd_get_entry(dbSpace->conn->driver, row, 6);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Expiration: %s\n", dbSpace->conn->log_id,
                        expiration ? expiration : "NULL");
        if (expiration == NULL) {
            continue;
        }
        if (adv_present) {
            adv = apr_dbd_get_entry(dbSpace->conn->driver, row, 7);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Adv: %s\n", dbSpace->conn->log_id, adv ? adv : "NULL");
            if (adv == NULL) {
                continue;
            }
            if (NULL == cache_entry->adv) {
                jAdv = jxta_advertisement_new();
                status = jxta_advertisement_parse_charbuffer(jAdv, adv, strlen(adv));
                if (JXTA_SUCCESS != status) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d Unable to parse Adv: %s\n", dbSpace->conn->log_id,
                                    adv ? adv : "NULL");
                    ret = NULL;
                    JXTA_OBJECT_RELEASE(cache_entry);
                    JXTA_OBJECT_RELEASE(profferAdv);
                    goto FINAL_EXIT;
                }
                cache_entry->adv = jAdv;
                jAdv = NULL;
            }
        }
        if (NULL == cache_entry->profid) {
            cache_entry->profid = jstring_new_2(profid);
        }
        if (NULL == profferAdv) {
            profferAdv = jxta_ProfferAdvertisement_new_1(nameSpace, advid, profid);
            cache_entry->profadv = profferAdv;
            sscanf(lifetime, jstring_get_string(jInt64_for_format), &cache_entry->lifetime);
            sscanf(expiration, jstring_get_string(jInt64_for_format), &cache_entry->expiration);
        }
        tmpid = jxta_proffer_adv_get_advId(profferAdv);
        tmpPid = jxta_proffer_adv_get_peerId(profferAdv);
        if (!strcmp(tmpPid, profid) && !strcmp(tmpid, advid)) {
            jxta_proffer_adv_add_item(profferAdv, name, value);
        } else {
            *(results++) = cache_entry;
            profferAdv = jxta_ProfferAdvertisement_new_1(nameSpace, advid, profid);
            jxta_proffer_adv_add_item(profferAdv, name, value);
            cache_entry = calloc(1, sizeof(Jxta_cache_entry));
            JXTA_OBJECT_INIT(cache_entry, cache_entry_free, NULL);
            cache_entry->profadv = profferAdv;
            sscanf(lifetime, jstring_get_string(jInt64_for_format), &cache_entry->lifetime);
            sscanf(expiration, jstring_get_string(jInt64_for_format), &cache_entry->expiration);
            if (adv_present) {
                jAdv = jxta_advertisement_new();
                status = jxta_advertisement_parse_charbuffer(jAdv, adv, strlen(adv));
                if (JXTA_SUCCESS != status) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d Unable to parse Adv: %s\n", dbSpace->conn->log_id,
                                    adv ? adv : "NULL");
                    ret = NULL;
                    goto FINAL_EXIT;
                }
                cache_entry->adv = jAdv;
                jAdv = NULL;
            }
            cache_entry->profid = jstring_new_2(profid);
        }
    }
    if (cache_entry != NULL) {
        *(results++) = cache_entry;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "db_id: %d Add proffer with profid %s, advid %s\n",
                        dbSpace->conn->log_id, NULL == tmpPid ? "(null)" : tmpPid, jxta_proffer_adv_get_advId(profferAdv));
    }
    *(results) = NULL;
  FINAL_EXIT:
    if (jAdv)
        JXTA_OBJECT_RELEASE(jAdv);
    return ret;
}

static Jxta_object **cm_arrays_concatenate(Jxta_object ** destination, Jxta_object ** source)
{
    Jxta_object **tempAdds;
    int sourceLength = 0;
    int destLength = 0;
    Jxta_object **addPoint;
    Jxta_object **newDest;
    if (NULL == source)
        return destination;
    tempAdds = source;
    while (*tempAdds) {
        sourceLength++;
        tempAdds++;
    }
    if (NULL != destination) {
        tempAdds = destination;
        while (*tempAdds) {
            destLength++;
            tempAdds++;
        }
    }
    if (sourceLength > 0) {
        int size = 0;
        size = destLength + sourceLength;
        newDest = realloc(destination, (size + 1) * sizeof(Jxta_object *));
        addPoint = newDest + destLength;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "cm_arrays_concatenate %d objects for a total of: %d objects\n",
                        sourceLength, size);
        while (*source) {
            *addPoint = *source;
            source++;
            addPoint++;
        }
        *addPoint = NULL;
    } else {
        newDest = destination;
    }
    return newDest;
}

Jxta_cache_entry **cm_sql_query_srdi_ns(Jxta_cm * self, const char *ns, JString * where)
{
    DBSpace **dbSpace = NULL;
    DBSpace **dbSpaceSave = NULL;
    JString *group_j = NULL;
    JString *whereClone = jstring_clone(where);
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    apr_dbd_results_t *res = NULL;
    Jxta_cache_entry **cache_entries = NULL;
    int rv;
    JString *columns = jstring_new_0();
    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", (*dbSpace)->id, aprs);
        goto FINAL_EXIT;
    }

    jstring_append_2(columns, CM_COL_SRC SQL_DOT CM_COL_Peerid SQL_COMMA CM_COL_SRC SQL_DOT CM_COL_AdvId);
    jstring_append_2(columns, SQL_COMMA CM_COL_SRC SQL_DOT CM_COL_TimeOut SQL_COMMA CM_COL_SRC SQL_DOT CM_COL_TimeOutForOthers);

    jstring_append_2(whereClone, SQL_AND);
    cm_sql_append_timeout_check_greater(whereClone);
    group_j = jstring_new_2(SQL_GROUP CM_COL_SRC SQL_DOT CM_COL_Peerid);

    dbSpace = cm_dbSpaces_get(self, ns, NULL, FALSE);
    dbSpaceSave = dbSpace;
    while (*dbSpace) {
        int nTuples = 0;

        rv = cm_sql_select(*dbSpace, pool, CM_TBL_SRDI_SRC, &res, columns, whereClone, group_j, TRUE);

        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- jxta_cm_sql_query_srdi Select failed: %s %i\n",
                            (*dbSpace)->conn->log_id, (*dbSpace)->id, apr_dbd_error((*dbSpace)->conn->driver,
                                                                                    (*dbSpace)->conn->sql, rv), rv);
            goto FINAL_EXIT;
        }

        nTuples = apr_dbd_num_tuples((*dbSpace)->conn->driver, res);

        if (nTuples > 0) {
            Jxta_cache_entry **tempAdds = NULL;
            Jxta_cache_entry **tempAddsSave = NULL;

            tempAdds = calloc(nTuples + 1, sizeof(struct cache_entry *));

            if (NULL == tempAdds) {
                goto FINAL_EXIT;
            }
            tempAddsSave = tempAdds;

            while (nTuples > 0) {
                apr_dbd_row_t *row;

                rv = apr_dbd_get_row((*dbSpace)->conn->driver, pool, res, &row, -1);

                if (rv == 0) {
                    const char * entry;
                    Jxta_cache_entry * cache_entry;

                    entry = apr_dbd_get_entry((*dbSpace)->conn->driver, row, 0);
                    cache_entry = calloc(1, sizeof(Jxta_cache_entry));
                    JXTA_OBJECT_INIT(cache_entry, cache_entry_free, NULL);

                    cache_entry->profid = jstring_new_2(entry);
                    *(tempAdds++) = cache_entry;
                }
                nTuples--;
            }
            cache_entries = (Jxta_cache_entry **) cm_arrays_concatenate((Jxta_object **) cache_entries, (Jxta_object **) tempAddsSave);
            free(tempAddsSave);
        }
        JXTA_OBJECT_RELEASE(*dbSpace);
        dbSpace++;
    }

  FINAL_EXIT:
    while (*dbSpace) {
        JXTA_OBJECT_RELEASE(*dbSpace);
        dbSpace++;
    }
    if (dbSpaceSave) {
        free(dbSpaceSave);
    }
    if (NULL != pool)
        apr_pool_destroy(pool);
    if (group_j)
        JXTA_OBJECT_RELEASE(group_j);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(whereClone);
    return cache_entries;
}

Jxta_vector *cm_query_replica(Jxta_cm * self, JString * nameSpace, Jxta_vector * queries)
{
    Jxta_status status;
    DBSpace **dbSpace = NULL;
    DBSpace **dbSpaceSave = NULL;
    JString *jWhere = NULL;
    JString *jOrder = NULL;
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
    dbSpace = cm_dbSpaces_get(self, jstring_get_string(nameSpace), NULL, FALSE);
    dbSpaceSave = dbSpace;

    cm_proffer_advertisements_columns_build(jColumns);

    jstring_append_2(jNsWhere, CM_COL_SRC);
    jstring_append_2(jNsWhere, SQL_DOT);
    jstring_append_2(jNsWhere, CM_COL_NameSpace);
    if (cm_sql_escape_and_wc_value(jNs, TRUE)) {
        jstring_append_2(jNsWhere, SQL_LIKE);
    } else {
        jstring_append_2(jNsWhere, SQL_EQUAL);
    }
    SQL_VALUE(jNsWhere, jNs);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- query with %i sub queries\n", (*dbSpace)->id,
                    jxta_vector_size(queries));

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", (*dbSpace)->id, aprs);
        goto finish;
    }
    if (0 == jxta_vector_size(queries)) {
        bNoSubQueries = TRUE;
    }

    for (i = 0; i < jxta_vector_size(queries) || bNoSubQueries; i++) {
        Jxta_query_element *elem = NULL;
        Jxta_cache_entry *cache_entry;
        const char *bool = NULL;

        if (bNoSubQueries) {
            bool = SQL_OR;
        } else {
            status = jxta_vector_get_object_at(queries, JXTA_OBJECT_PPTR(&elem), i);
            if (!elem->isReplicated) {
                JXTA_OBJECT_RELEASE(elem);
                continue;
            }
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

        if (jOrder != NULL) {
            JXTA_OBJECT_RELEASE(jOrder);
            jOrder = NULL;
        }

        jOrder = jstring_new_2(SQL_ORDER);
        jstring_append_2(jOrder, CM_COL_AdvId);
        jstring_append_2(jOrder, SQL_COMMA);
        jstring_append_2(jOrder, CM_COL_Peerid);
        dbSpace = dbSpaceSave;
        adds = NULL;
        while (*dbSpace) {
            Jxta_cache_entry **tempAdds;
            rv = cm_sql_select(*dbSpace, pool, CM_TBL_REPLICA_SRC, &res, jColumns, jWhere, jOrder, TRUE);

            if (rv) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                "db_id: %d %s -- jxta_cm_sql_query_replica Select failed: %s %i\n", (*dbSpace)->conn->log_id,
                                (*dbSpace)->id, apr_dbd_error((*dbSpace)->conn->driver, (*dbSpace)->conn->sql, rv), rv);
                goto finish;
            }
            tempAdds = cm_proffer_advertisements_build(*dbSpace, pool, res, FALSE);
            if (NULL != tempAdds) {
                adds = cm_arrays_concatenate(adds, (Jxta_object **) tempAdds);
                free(tempAdds);
            }
            dbSpace++;
        }
        if (NULL == adds && !bNoSubQueries)
            continue;
        if (NULL == peersHash) {
            peersHash = jxta_hashtable_new(1);
        }
        saveAdds = adds;
        if (!strcmp(bool, SQL_OR)) {
            while (NULL != adds && *adds) {
                cache_entry = (Jxta_cache_entry *) * (adds);
                peerId = jstring_get_string(cache_entry->profid);
                if (jxta_hashtable_get(peersHash, peerId, strlen(peerId), JXTA_OBJECT_PPTR(&jPeerId)) != JXTA_SUCCESS) {
                    JString *addClone = NULL;
                    addClone = jstring_clone(cache_entry->profid);
                    jxta_hashtable_put(peersHash, peerId, strlen(peerId), (Jxta_object *) addClone);
                    JXTA_OBJECT_RELEASE(addClone);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Added to the peersHash: %s\n",
                                    jstring_get_string(self->jGroupID_string), peerId);
                } else {
                    JXTA_OBJECT_RELEASE(jPeerId);
                    jPeerId = NULL;
                }
                JXTA_OBJECT_RELEASE(*(adds++));
            }
        } else {
            Jxta_vector *peersV = jxta_vector_new(1);
            if (NULL == andHash) {
                andHash = jxta_hashtable_new(1);
            }
            saveAdds = adds;
            while (NULL != adds && *adds) {
                JString *addClone = NULL;
                cache_entry = (Jxta_cache_entry *) * adds;
                addClone = jstring_clone(cache_entry->profid);
                jxta_vector_add_object_first(peersV, (Jxta_object *) addClone);
                JXTA_OBJECT_RELEASE(addClone);
                JXTA_OBJECT_RELEASE(*(adds++));
            }
            jxta_hashtable_put(andHash, elemSQL, strlen(elemSQL), (Jxta_object *) peersV);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Added to the andHash:%i %s\n",
                            jstring_get_string(self->jGroupID_string), jxta_vector_size(peersV), elemSQL);
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
                                jstring_get_string(self->jGroupID_string), status);
                if (result != NULL)
                    JXTA_OBJECT_RELEASE(result);
                continue;
            }
            size = jxta_vector_size(result);
            if (leastPopular == 0) {
                leastPopular = size;
                jxta_vector_add_object_first(finalResult, (Jxta_object *) result);
            } else if (size == leastPopular) {
                jxta_vector_add_object_first(finalResult, (Jxta_object *) result);
            } else if (size < leastPopular) {
                jxta_vector_clear(finalResult);
                jxta_vector_add_object_first(finalResult, (Jxta_object *) result);
            }
            JXTA_OBJECT_RELEASE(result);
        }

        JXTA_OBJECT_RELEASE(results);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s -- Final result:%i items\n",
                        jstring_get_string(self->jGroupID_string), jxta_vector_size(finalResult));
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
                                    "%s -- Error returning result from the finalResults vector : %i \n",
                                    jstring_get_string(self->jGroupID_string), status);
                    continue;
                }
                if (peersHash == NULL) {
                    peersHash = jxta_hashtable_new(jxta_vector_size(peerEntries));
                }
                tmpPeer = jstring_get_string(peer);
                if (jxta_hashtable_contains(peersHash, tmpPeer, strlen(tmpPeer)) != JXTA_SUCCESS) {
                    jxta_hashtable_put(peersHash, tmpPeer, strlen(tmpPeer), (Jxta_object *) peer);
                }
                JXTA_OBJECT_RELEASE(peer);
                peer = NULL;
            }
            JXTA_OBJECT_RELEASE(peerEntries);
        }
        JXTA_OBJECT_RELEASE(finalResult);
    }
    if (NULL != peersHash) {
        ret = jxta_hashtable_values_get(peersHash);
    } else {
        ret = NULL;
    }

  finish:
    if (dbSpaceSave) {
        dbSpace = dbSpaceSave;
        while (*dbSpace) {
            JXTA_OBJECT_RELEASE(*dbSpace);
            dbSpace++;
        }
        free(dbSpaceSave);
    }
    JXTA_OBJECT_RELEASE(jColumns);
    JXTA_OBJECT_RELEASE(jNs);
    JXTA_OBJECT_RELEASE(jNsWhere);
    JXTA_OBJECT_RELEASE(jWhere);
    if (jOrder)
        JXTA_OBJECT_RELEASE(jOrder);
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
Jxta_status cm_restore_bytes(Jxta_cm * self, char *folder_name, char *primary_key, JString ** bytes)
{
    Jxta_status status = JXTA_ITEM_NOTFOUND;

    DBSpace *dbSpace = NULL;
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
    unsigned int j;

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
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", dbSpace->id, aprs);
        goto finish;
    }
    for (j = 0; j < jxta_vector_size(self->dbSpaces); j++) {
        jxta_vector_get_object_at(self->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), j);
        rv = cm_sql_select(dbSpace, pool, CM_TBL_ADVERTISEMENTS, &res, columns, where, NULL, TRUE);

        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- jxta_cm_restore_bytes Select failed: %s %i\n",
                            dbSpace->conn->log_id, dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
            goto finish;
        }
        rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);

        if (rv == 0) {
            entry = apr_dbd_get_entry(dbSpace->conn->driver, row, 0);
            if (entry != NULL) {
                *bytes = jstring_new_0();
                jstring_append_2(*bytes, entry);
                status = JXTA_SUCCESS;
                break;
            }
        }
        JXTA_OBJECT_RELEASE(dbSpace);
        dbSpace = NULL;
    }

  finish:
    if (NULL != dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
    if (NULL != pool)
        apr_pool_destroy(pool);

    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jKey);
    JXTA_OBJECT_RELEASE(jFolder);
    return status;
}

static Jxta_status cm_get_time(Jxta_cm * self, const char *folder_name, const char *primary_key, Jxta_expiration_time * ttime,
                               const char *time_name)
{
    Folder *folder = NULL;
    Jxta_status status;
    Jxta_expiration_time exp[2];
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    int rv;
    DBSpace *dbSpace = NULL;
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row;
    unsigned int i;
    const char *entry = NULL;
    JString *columns = jstring_new_0();
    JString *where = jstring_new_0();
    JString *jKey = jstring_new_0();

    jstring_append_2(jKey, primary_key);

    status = jxta_hashtable_get(self->folders, folder_name, (size_t) strlen(folder_name), JXTA_OBJECT_PPTR(&folder));
    if (status != JXTA_SUCCESS) {
        return status;
    }

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", dbSpace->id, aprs);
        goto finish;
    }

    jstring_append_2(columns, time_name);
    jstring_append_2(where, CM_COL_AdvId);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jKey);
    for (i = 0; i < jxta_vector_size(self->dbSpaces); i++) {
        status = jxta_vector_get_object_at(self->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), i);
        rv = cm_sql_select(dbSpace, pool, CM_TBL_ADVERTISEMENTS, &res, columns, where, NULL, TRUE);

        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- jxta_cm_get_expiration_time Select failed: %s %i\n",
                            dbSpace->conn->log_id, dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
            goto finish;
        }
        rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);

        if (rv == 0) {
            entry = apr_dbd_get_entry(dbSpace->conn->driver, row, 0);
            break;
        }
        JXTA_OBJECT_RELEASE(dbSpace);
        dbSpace = NULL;
    }
    if (entry == NULL)
        status = JXTA_ITEM_NOTFOUND;

    if (status == JXTA_SUCCESS) {

        sscanf(entry, jstring_get_string(jInt64_for_format), &exp[0]);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d %s expiration time retreived \n", dbSpace->conn->log_id,
                        dbSpace->id);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, jstring_get_string(fmtTime), exp[0]);
        *ttime = exp[0];
    }

  finish:
    if (folder)
        JXTA_OBJECT_RELEASE(folder);
    if (NULL != pool)
        apr_pool_destroy(pool);
    if (dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jKey);

    return status;
}

Jxta_status cm_get_others_time(Jxta_cm * self, char *folder_name, char *primary_key, Jxta_expiration_time * ttime)
{
    return cm_get_time(self, folder_name, primary_key, ttime, CM_COL_TimeOutForOthers);
}

/* Get the expiration time of an advertisement */
Jxta_status cm_get_expiration_time(Jxta_cm * self, char *folder_name, char *primary_key, Jxta_expiration_time * ttime)
{
    return cm_get_time(self, folder_name, primary_key, ttime, CM_COL_TimeOut);
}

static Jxta_status cm_create_delta_entries(Jxta_cm * self, JString *peerid, JString * src_peerid, Jxta_SRDIEntryElement *pentry, Jxta_vector ** entries)
{
    Jxta_status status = JXTA_SUCCESS;

    JString **sn_ptr = NULL;
    JString **sn_ptr_save = NULL;
    Jxta_sequence_number seq_number;


    if (NULL == *entries)
        *entries = jxta_vector_new(0);

    if (NULL != pentry->sn_cs_values) {
        sn_ptr = tokenize_string_j(pentry->sn_cs_values, ',');
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Create entries - %s \n", jstring_get_string(pentry->sn_cs_values));
    } else if (pentry->seqNumber > 0) {
        char tmp[64];
        sn_ptr = calloc(1, sizeof(JString *) + 1);
        apr_snprintf(tmp, sizeof(tmp), JXTA_SEQUENCE_NUMBER_FMT, pentry->seqNumber);
        *sn_ptr = jstring_new_2(tmp);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Create entry - %s \n", jstring_get_string(*sn_ptr));
    } else {
        goto FINAL_EXIT;
    }
    sn_ptr_save = sn_ptr;

    while (*sn_ptr) {
        Jxta_SRDIEntryElement *entry=NULL;

        entry = jxta_srdi_new_element();
        sscanf(jstring_get_string(*sn_ptr), JPR_ABS_TIME_FMT, &seq_number);

        status = cm_get_srdi_with_seq_number(self, peerid, seq_number, entry);
        entry->resend = FALSE;
        entry->seqNumber = seq_number;
        entry->expiration = pentry->expiration;
        if (JXTA_ITEM_NOTFOUND == status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Didn't find seq %" APR_INT64_T_FMT "\n", seq_number);
            entry->resend = TRUE;
            entry->replicate = TRUE;
        } else {
            entry->expanded = TRUE;
        }
        jxta_vector_add_object_last(*entries, (Jxta_object *)entry);

        JXTA_OBJECT_RELEASE(*sn_ptr);
        sn_ptr++;
        JXTA_OBJECT_RELEASE(entry);
    }
    free(sn_ptr_save);

FINAL_EXIT:

    return status;
}

Jxta_status cm_expand_delta_entries(Jxta_cm * self, Jxta_vector * msg_entries, JString * peerid, JString * source_peerid, Jxta_vector ** ret_entries)
{
    Jxta_status status = JXTA_SUCCESS;
    unsigned int i;

    if (NULL == *ret_entries) {
        *ret_entries = jxta_vector_new(0);
    }
    for (i=0; i<jxta_vector_size(msg_entries); i++) {
        Jxta_SRDIEntryElement *entry;

        status = jxta_vector_get_object_at(msg_entries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != status) {
            continue;
        }
        if (entry->seqNumber > 0 && NULL == entry->sn_cs_values && NULL != entry->advId) {
            jxta_vector_add_object_last(*ret_entries, (Jxta_object *) entry);
            JXTA_OBJECT_RELEASE(entry);
            continue;
        } else {
            status = cm_create_delta_entries(self, peerid, source_peerid, entry, ret_entries);
        }
        JXTA_OBJECT_RELEASE(entry);
    }
    return status;
}

Jxta_status cm_get_delta_entries_for_update(Jxta_cm * self, const char *name, JString *peer, Jxta_hashtable ** entries)
{
    Jxta_status status = JXTA_SUCCESS;
    char aTime[64];
    Jxta_expiration_time tNow;
    JString * columns = NULL;
    JString * where = NULL;
    DBSpace *dbSpace=NULL;
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    apr_dbd_results_t *res = NULL;
    Jxta_hashtable *source_hash = NULL;
    Jxta_hashtable *dest_hash = NULL;
    Jxta_hashtable * ads_hash = NULL;
    Jxta_vector *entries_v = NULL;
    apr_dbd_row_t *row;
    int ret = 0;
    int num_tuples = 0;
    int i;

    dbSpace = JXTA_OBJECT_SHARE(self->bestChoiceDB);
    source_hash = jxta_hashtable_new(0);
    *entries = source_hash;
    ads_hash = jxta_hashtable_new(0);

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "%s -- Unable to create apr_pool: %d\n", dbSpace->id, aprs);
        goto FINAL_EXIT;
    }

    columns = jstring_new_2(CM_COL_SeqNumber SQL_COMMA CM_COL_TimeOut 
                            SQL_COMMA CM_COL_Peerid SQL_COMMA CM_COL_NameSpace
                            SQL_COMMA CM_COL_TimeOutForOthers SQL_COMMA CM_COL_NextUpdate
                            SQL_COMMA CM_COL_AdvId SQL_COMMA CM_COL_SourcePeerid SQL_COMMA CM_COL_Duplicate);

    where = jstring_new_2(CM_COL_NextUpdate SQL_NOT_EQUAL "0");

    jstring_append_2(where, SQL_AND CM_COL_NextUpdate SQL_LESS_THAN);

    memset(aTime, 0, 64);
    tNow = jpr_time_now();
    if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, tNow) != 0) {
        jstring_append_2(where, aTime);
    } else {
        status = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    jstring_append_2(where, SQL_AND CM_COL_TimeOut SQL_GREATER_THAN);

    jstring_append_2(where, aTime);

    if (NULL != peer) {
        jstring_append_2(where, SQL_AND CM_COL_Peerid SQL_EQUAL);
        SQL_VALUE(where, peer);
    }

    jstring_append_2(where, SQL_AND);
    cm_sql_correct_space(name, where);

    status = cm_sql_select(dbSpace, pool, CM_TBL_SRDI_DELTA, &res, columns, where, NULL, TRUE);
    if (status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "%s Select failed: %s %i\n",
                            dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, status), status);
        goto FINAL_EXIT;
    }
    num_tuples = apr_dbd_num_tuples((apr_dbd_driver_t *) dbSpace->conn->driver, res);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "SRDI Deltas tuples:%d %s\n", num_tuples, jstring_get_string(where));
    for (i = 0; i < num_tuples; i++) {
        const char *seq_no;
        const char *timeout_others;
        const char *peerid;
        const char *name_space;
        const char *timeout;
        const char *next_update;
        const char *advid;
        const char *source_peerid;
        const char *duplicate;
        JString *seq_no_j = NULL;
        Jpr_interval_time timeout_time;
        Jpr_interval_time next_update_time;
        Jxta_SRDIEntryElement *entry;
        JString *jPeerid = NULL;
        JString *key_j = NULL;
        Jxta_SRDIEntryElement *existing_entry;

        entries_v = NULL;
        ret = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);

        if (0 != ret) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "%s -- Select failed: %s %i\n",
                            dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, ret), ret);
            status = ret;
            JXTA_OBJECT_RELEASE(*entries);
            goto FINAL_EXIT;
        }

        seq_no = apr_dbd_get_entry(dbSpace->conn->driver, row, 0);
        timeout = apr_dbd_get_entry(dbSpace->conn->driver, row, 1);
        peerid = apr_dbd_get_entry(dbSpace->conn->driver, row, 2);
        name_space = apr_dbd_get_entry(dbSpace->conn->driver, row, 3);
        /* others */
        timeout_others = apr_dbd_get_entry(dbSpace->conn->driver, row, 4);
        next_update = apr_dbd_get_entry(dbSpace->conn->driver, row, 5);
        advid = apr_dbd_get_entry(dbSpace->conn->driver, row, 6);
        source_peerid = apr_dbd_get_entry(dbSpace->conn->driver, row, 7);
        duplicate = apr_dbd_get_entry(dbSpace->conn->driver, row, 8);

        jPeerid = jstring_new_2(peerid);
        entry = jxta_srdi_new_element();
        seq_no_j = jstring_new_2(seq_no);
        entry->duplicate = !strcmp(duplicate, "1") ? TRUE:FALSE;
        sscanf(seq_no, JPR_ABS_TIME_FMT, &entry->seqNumber);
        sscanf(timeout_others, JPR_ABS_TIME_FMT, &entry->expiration);
        sscanf(timeout, JPR_ABS_TIME_FMT, &timeout_time);
        sscanf(next_update, JPR_ABS_TIME_FMT, &next_update_time);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Getting source hash entry: %s \n", source_peerid);

        status = jxta_hashtable_get(source_hash, source_peerid, strlen(source_peerid) + 1, JXTA_OBJECT_PPTR(&dest_hash));
        if (JXTA_SUCCESS != status) {
            dest_hash = jxta_hashtable_new(0);
            jxta_hashtable_put(source_hash, source_peerid, strlen(source_peerid) +1, (Jxta_object *) dest_hash);
        }

        key_j = jstring_new_2(advid);
        if (NULL != entry->advId) {
            JXTA_OBJECT_RELEASE(entry->advId);
        }
        entry->advId = jstring_clone(key_j);
        jstring_append_1(key_j, jPeerid);

        jstring_append_2(key_j, source_peerid);

        status = jxta_hashtable_get(ads_hash, jstring_get_string(key_j), jstring_length(key_j) + 1, JXTA_OBJECT_PPTR(&existing_entry));
        if (JXTA_SUCCESS != status) {
            jxta_hashtable_put(ads_hash, jstring_get_string(key_j), jstring_length(key_j) + 1, (Jxta_object *) entry);

            if (JXTA_SUCCESS != (jxta_hashtable_get(dest_hash, peerid, strlen(peerid) + 1, JXTA_OBJECT_PPTR(&entries_v)))) {
                entries_v = jxta_vector_new(0);
                jxta_hashtable_put(dest_hash, peerid, strlen(peerid) + 1, (Jxta_object *) entries_v);
            }
            entry->expiration = timeout_time - jpr_time_now();
            entry->seqNumber = 0;
            entry->sn_cs_values = jstring_new_2(seq_no);
            entry->replicate = TRUE;
            jxta_vector_add_object_last(entries_v, (Jxta_object *)entry);
        } else {
            jstring_append_2(existing_entry->sn_cs_values, ",");
            jstring_append_2(existing_entry->sn_cs_values, seq_no);
            JXTA_OBJECT_RELEASE(existing_entry);
        }
        if (entries_v) {
            JXTA_OBJECT_RELEASE(entries_v);
        }
        JXTA_OBJECT_RELEASE(key_j);
        JXTA_OBJECT_RELEASE(seq_no_j);
        JXTA_OBJECT_RELEASE(entry);
        JXTA_OBJECT_RELEASE(jPeerid);
        JXTA_OBJECT_RELEASE(dest_hash);
        dest_hash = NULL;
    }

FINAL_EXIT:

    if (ads_hash)
        JXTA_OBJECT_RELEASE(ads_hash);
    if (dest_hash)
        JXTA_OBJECT_RELEASE(dest_hash);
    if (pool)
        apr_pool_destroy(pool);
    if (columns)
        JXTA_OBJECT_RELEASE(columns);
    if (dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
    if (where)
        JXTA_OBJECT_RELEASE(where);

    return status;
}

/* delete files where the expiration time is less than curr_expiration */
Jxta_status cm_remove_expired_records(Jxta_cm * self)
{
    Jxta_vector *all_folders;
    int cnt, i;
    Folder *folder;
    Jxta_status rt, latched_rt = JXTA_SUCCESS;


    all_folders = jxta_hashtable_values_get(self->folders);
    if (NULL == all_folders) {
        return JXTA_NOMEM;
    }
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
Jxta_status cm_create_folder(Jxta_cm * self, char *folder_name, const char *keys[])
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
        cm_create_adv_indexes(self, folder_name, iv);
        JXTA_OBJECT_RELEASE(iv);
    }

    apr_thread_mutex_unlock(self->mutex);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "-- Removing expired %s Advertisements \n", folder_name);

    folder_remove_expired_records(self, folder);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "-- Removing expired %s Advertisements - Done \n", folder_name);

    JXTA_OBJECT_RELEASE(folder);

    return JXTA_SUCCESS;
}

Jxta_status cm_create_adv_indexes(Jxta_cm * self, char *folder_name, Jxta_vector * jv)
{
    unsigned int i = 0;
    Folder *folder = NULL;
    Jxta_status status = 0;

    status = jxta_hashtable_get(self->folders, folder_name, (size_t) strlen(folder_name), JXTA_OBJECT_PPTR(&folder));
    if (status != JXTA_SUCCESS) {
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
                jxta_hashtable_put(folder->secondary_keys, full_index_name, strlen(full_index_name), (Jxta_object *) attrib);
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
unsigned long cm_hash(const char *key, size_t ksz)
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
void cm_close(Jxta_cm * self)
{
    DBSpace *dbSpace = NULL;

    apr_thread_mutex_lock(self->mutex);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "cm_close\n");
    JXTA_OBJECT_RELEASE(dbSpace);
    JXTA_OBJECT_RELEASE(self->folders);
    self->folders = NULL;
    apr_thread_mutex_unlock(self->mutex);
}

/*
 * obtain all CM SRDI delta entries for a particular folder
 */
Jxta_vector *cm_get_srdi_delta_entries(Jxta_cm * self, JString * folder_name)
{
    Jxta_vector *delta_entries = NULL;

    if (jxta_hashtable_del(self->srdi_delta, jstring_get_string(folder_name), (size_t) strlen(jstring_get_string(folder_name))
                           , JXTA_OBJECT_PPTR(&delta_entries)) != JXTA_SUCCESS) {
        if (NULL != delta_entries)
            JXTA_OBJECT_RELEASE(delta_entries);
        return NULL;
    }
    return delta_entries;
}

void cm_set_delta(Jxta_cm * self, Jxta_boolean recordDelta)
{
    self->record_delta = recordDelta;
}

/*
 * return SRDI elements created from the element attributes table
 */
Jxta_vector *cm_create_srdi_entries(Jxta_cm * self, JString * folder_name, JString * pAdvId, JString *pPeerId)
{
    return cm_get_srdi_entries_priv(self, CM_TBL_ELEM_ATTRIBUTES, folder_name, pAdvId, pPeerId);
}

/*
 * return SRDI entries from the SRDI table
 */
Jxta_vector *cm_get_srdi_entries_1(Jxta_cm * self, JString * folder_name, JString * pAdvId, JString * pPeerid)
{
    return cm_get_srdi_entries_priv(self, CM_TBL_SRDI, folder_name, pAdvId, pPeerid);
}

static Jxta_vector *cm_get_srdi_entries_priv(Jxta_cm *self, const char *table_name, JString *folder_name, JString *pAdvId, JString *pPeerId)
{
    size_t nb_keys;
    DBSpace *dbSpace = NULL;
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
    Jxta_vector *entries = NULL;
    JString *jFormatTime = jstring_new_0();
    JString *where = jstring_new_0();
    Jxta_SRDIEntryElement *element = NULL;
    const char *cFolder_name;
    const char *entry;
    int rv = 0;
    int n;
    unsigned int j;

    cFolder_name = jstring_get_string(folder_name);

    jstring_append_2(jFormatTime, "Exp Time ");
    jstring_append_1(jFormatTime, jInt64_for_format);
    jstring_append_2(jFormatTime, "\n");

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", dbSpace->id, aprs);
        goto finish;
    }
    if (NULL != folder_name) {
        cm_sql_correct_space(jstring_get_string(folder_name), where);
        jstring_append_2(where, SQL_AND);
    }

    jstring_append_2(columns, CM_COL_AdvId);
    jstring_append_2(columns, SQL_COMMA CM_COL_NameSpace SQL_COMMA CM_COL_Name);
    jstring_append_2(columns, SQL_COMMA CM_COL_Value SQL_COMMA CM_COL_NumValue);
    jstring_append_2(columns, SQL_COMMA CM_COL_NumRange SQL_COMMA CM_COL_TimeOutForOthers);

    cm_sql_append_timeout_check_greater(where);
    if (NULL != pAdvId) {
        jstring_append_2(where, SQL_AND CM_COL_AdvId SQL_EQUAL);
        SQL_VALUE(where, pAdvId);
    }
    if (NULL != pPeerId) {
        jstring_append_2(where, SQL_AND CM_COL_Peerid SQL_EQUAL);
        SQL_VALUE(where, pPeerId);

    }
    for (j = 0; j < jxta_vector_size(self->dbSpaces); j++) {
        Jxta_status status;
        status = jxta_vector_get_object_at(self->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), j);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error getting the dbSpace %i\n", status);
        }
        rv = cm_sql_select(dbSpace, pool, table_name, &res, columns, where, NULL, TRUE);
        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                            "db_id: %d %s -- jxta_cm_get_srdi_entries_priv Select failed: %s %i \n %s   \n", dbSpace->conn->log_id,
                            dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv,
                            jstring_get_string(columns));
            JXTA_OBJECT_RELEASE(dbSpace);
            dbSpace = NULL;
            continue;
        }
        nb_keys = apr_dbd_num_tuples((apr_dbd_driver_t *) dbSpace->conn->driver, res);
        if (nb_keys == 0) {
            JXTA_OBJECT_RELEASE(dbSpace);
            dbSpace = NULL;
            continue;
        }
        if (NULL == entries) {
            entries = jxta_vector_new(nb_keys);
        }
        for (rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);
             rv == 0; rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1)) {
            int column_count = apr_dbd_num_cols(dbSpace->conn->driver, res);

            jKey = jstring_new_0();
            jVal = jstring_new_0();
            jNumVal = jstring_new_0();
            jNameSpace = jstring_new_0();
            jAdvId = jstring_new_0();
            jRange = jstring_new_0();

            for (n = 0; n < column_count; ++n) {
                entry = apr_dbd_get_entry(dbSpace->conn->driver, row, n);
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

            /* don't return local only advertisements */
            if (exp > 0) {
                apr_thread_mutex_lock(self->mutex);
                ++self->delta_seq_number;
                element = jxta_srdi_new_element_3(jKey, jVal, jNameSpace, jAdvId, jRange, exp, self->delta_seq_number);
                if (dbSpace->alias)
                    element->db_alias = strdup(dbSpace->alias);
                apr_thread_mutex_unlock(self->mutex);
                jxta_vector_add_object_last(entries, (Jxta_object *) element);
                JXTA_OBJECT_RELEASE(element);
            }
            JXTA_OBJECT_RELEASE(jAdvId);
            JXTA_OBJECT_RELEASE(jKey);
            JXTA_OBJECT_RELEASE(jVal);
            JXTA_OBJECT_RELEASE(jNumVal);
            JXTA_OBJECT_RELEASE(jRange);
            JXTA_OBJECT_RELEASE(jNameSpace);
        }
        JXTA_OBJECT_RELEASE(dbSpace);
        dbSpace = NULL;
    }

  finish:
    if (dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
    if (NULL != pool)
        apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(columns);
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jFormatTime);
    if (NULL != entries) {
        if (0 == jxta_vector_size(entries)) {
            JXTA_OBJECT_RELEASE(entries);
            entries = NULL;
        }
    }
    return entries;
}

void cm_remove_srdi_entries(Jxta_cm * self, JString * peerid, Jxta_vector **srdi_entries)
{
    JString *where;
    unsigned int i;
    DBSpace *dbSpace;
    where = jstring_new_0();
    jstring_append_2(where, CM_COL_Peerid);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, peerid);
    *srdi_entries = cm_get_srdi_entries_1(self, NULL, NULL, peerid);
    dbSpace = JXTA_OBJECT_SHARE(self->bestChoiceDB);
    cm_sql_delete_with_where(dbSpace, CM_TBL_SRDI_DELTA, where);
    cm_sql_delete_with_where(dbSpace, CM_TBL_SRDI_INDEX, where);
    JXTA_OBJECT_RELEASE(dbSpace);
    for (i = 0; i < jxta_vector_size(self->dbSpaces); i++) {
        jxta_vector_get_object_at(self->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), i);
        cm_sql_delete_with_where(dbSpace, CM_TBL_SRDI, where);
        cm_sql_delete_with_where(dbSpace, CM_TBL_REPLICA, where);
        JXTA_OBJECT_RELEASE(dbSpace);
    }
    JXTA_OBJECT_RELEASE(where);
}

void cm_remove_srdi_delta_entries(Jxta_cm * me, JString * jPeerid, Jxta_vector * entries)
{
    int i;
    DBSpace *dbSpace = NULL;
    Jxta_status status;
    if (NULL != jPeerid || NULL != entries) {
        JString *jWhere;
        jWhere = jstring_new_0();
        if (NULL != jPeerid) {
            jstring_append_2(jWhere, CM_COL_Peerid SQL_EQUAL);
            SQL_VALUE(jWhere, jPeerid);
        }
        for (i = 0; NULL != entries && i < jxta_vector_size(entries); i++) {
            Jxta_SRDIEntryElement *entry;
            char aTmp[64];
            status = jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve entry removing srdi delta status=%i\n",
                                status);
                continue;
            }
            if (i > 0) {
                jstring_append_2(jWhere, SQL_OR);
            } else {
                if (NULL != jPeerid) {
                    jstring_append_2(jWhere, SQL_AND);
                }
            }
            jstring_append_2(jWhere, CM_COL_SeqNumber SQL_EQUAL);
            memset(aTmp, 0, sizeof(aTmp));
            apr_snprintf(aTmp, sizeof(aTmp), JXTA_SEQUENCE_NUMBER_FMT, entry->seqNumber);
            jstring_append_2(jWhere, aTmp);
            JXTA_OBJECT_RELEASE(entry);
        }
        dbSpace = JXTA_OBJECT_SHARE(me->bestChoiceDB);
        apr_thread_mutex_lock(dbSpace->conn->lock);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "db_id: %d %s Removing from delta table %s \n", dbSpace->conn->log_id,
                        dbSpace->id, jstring_get_string(jWhere));
        cm_sql_delete_with_where(dbSpace, CM_TBL_SRDI_DELTA, jWhere);
        apr_thread_mutex_unlock(dbSpace->conn->lock);
        JXTA_OBJECT_RELEASE(jWhere);
    } else {
        for (i = 0; i < jxta_vector_size(me->dbSpaces); i++) {
            status = jxta_vector_get_object_at(me->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), i);
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                "Unable to retrieve dbSpace removing srdi delta delta status=%i\n", status);
                continue;
            }
            cm_sql_delete_with_where(dbSpace, CM_TBL_SRDI_DELTA, NULL);
            JXTA_OBJECT_RELEASE(dbSpace);
            dbSpace = NULL;
        }
    }
    if (dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
}

static apr_status_t cm_sql_table_create(DBSpace * dbSpace, const char *table, const char *columns[])
{
    int rv = 0;
    int nrows;
    Jxta_status status;
    JString *cmd;

    status = cm_sql_table_found(dbSpace, table);
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

    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(cmd));

    if (rv != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Couldn't create %s  rc=%i\n", dbSpace->conn->log_id,
                        dbSpace->id, table, rv);
    }
    JXTA_OBJECT_RELEASE(cmd);
    return rv;
}

static Jxta_status cm_sql_table_found(DBSpace * dbSpace, const char *table)
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
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", dbSpace->id, aprs);
        status = JXTA_FAILED;
        goto finish;
    }

    rv = cm_sql_select(dbSpace, pool, SQL_TBL_MASTER, &res, columns, where, NULL, FALSE);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- cm_sql_table_found Select failed: %s %i\n",
                        dbSpace->conn->log_id, dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
        status = JXTA_FAILED;
        goto finish;
    }
    if (apr_dbd_num_tuples(dbSpace->conn->driver, res) > 0) {
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

static int cm_sql_index_create(DBSpace * dbSpace, const char *table, const char **fields)
{
    int nrows;
    int ret;
    Jxta_status status;
    JString *cmd;
    JString *jIdx = jstring_new_0();

    jstring_append_2(jIdx, table);
    jstring_append_2(jIdx, *fields);

    status = cm_sql_index_found(dbSpace, jstring_get_string(jIdx));
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
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "db_id: %d %s -- %s \n", dbSpace->conn->log_id, dbSpace->id,
                    jstring_get_string(cmd));

    ret = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(cmd));

    JXTA_OBJECT_RELEASE(cmd);
    return ret;
}

static Jxta_status cm_sql_index_found(DBSpace * dbSpace, const char *iindex)
{
    int rv = 0;
    Jxta_status status = JXTA_ITEM_NOTFOUND;
    JString *where = jstring_new_0();
    JString *columns = jstring_new_0();
    JString *statement = jstring_new_0();
    JString *jIndex = jstring_new_2(iindex);
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
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", dbSpace->id, aprs);
        status = JXTA_FAILED;
        goto finish;
    }

    rv = cm_sql_select(dbSpace, pool, SQL_TBL_MASTER, &res, columns, where, NULL, FALSE);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- cm_sql_table_found Select failed: %s %i\n",
                        dbSpace->conn->log_id, dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
        status = JXTA_FAILED;
        goto finish;
    }
    if (apr_dbd_num_tuples(dbSpace->conn->driver, res) > 0) {
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

#ifdef UNUSED_VWF
static int cm_sql_table_drop(DBSpace * dbSpace, char *table)
{
    int rv = 0;
    int nrows;
    JString *statement = jstring_new_2(SQL_DROP_TABLE);
    jstring_append_2(statement, table);

    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(statement));

    JXTA_OBJECT_RELEASE(statement);

    return rv;
}
#endif
static Jpr_interval_time lifetime_get(Jpr_interval_time base, Jpr_interval_time span)
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

static Jxta_status cm_advertisement_update(DBSpace * dbSpace, JString * jNameSpace, JString * jKey, JString * jXml,
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
    if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, lifetime_get(jpr_time_now(), timeOutForMe)) != 0) {
        jstring_append_2(update_sql, aTime);
    } else {
        jstring_append_2(update_sql, "0");
    }
    jstring_append_2(update_sql, SQL_COMMA);
    jstring_append_2(update_sql, CM_COL_TimeOutForOthers);
    jstring_append_2(update_sql, SQL_EQUAL);

    if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, timeOutForOthers) != 0) {
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

    jstring_append_2(update_sql, SQL_AND CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(update_sql, dbSpace->jId);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d %s -- update : %s \n", dbSpace->conn->log_id, dbSpace->id,
                    jstring_get_string(update_sql));

    sqlCmd = jstring_get_string(update_sql);
    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, sqlCmd);

    status = (Jxta_status) rv;
    JXTA_OBJECT_RELEASE(update_sql);
    return status;

}

static Jxta_status cm_advertisement_save(DBSpace * dbSpace, const char *key, Jxta_advertisement * adv,
                                         Jxta_expiration_time timeOutForMe, Jxta_expiration_time timeOutForOthers,
                                         Jxta_boolean update)
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
    JString *jGroupID = jstring_new_2(dbSpace->id);
    Jxta_expiration_time lifetime = 0;
    Jxta_expiration_time expiration = 0;
    char aTime[64];
    JString *jKey = jstring_new_2(key);

    jxta_advertisement_get_xml(adv, &jXml);
    if (jXml == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "db_id: %d %s -- Advertisement Not saved \n", dbSpace->conn->log_id,
                        dbSpace->id);
        status = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    status = cm_item_found(dbSpace, CM_TBL_ADVERTISEMENTS, nameSpace, jstring_get_string(jKey), NULL, &lifetime, &expiration);

    if (status == JXTA_SUCCESS) {
        Jxta_expiration_time expTime = 0;
        /* if update is allowed */
        if (update) {
            /* can't reduce the adv life time DoS */
            if (timeOutForMe > lifetime) {
                lifetime = timeOutForMe;
            }
            expTime = timeOutForOthers;
            if (expTime > lifetime) {
                expTime = lifetime;
            }
            status = cm_advertisement_update(dbSpace, jNameSpace, jKey, jXml, lifetime, expTime);
        }
        goto FINAL_EXIT;

    } else if (status == JXTA_ITEM_NOTFOUND) {
        jstring_append_2(insert_sql, SQL_INSERT_INTO);
        jstring_append_2(insert_sql, CM_TBL_ADVERTISEMENTS);
        jstring_append_2(insert_sql, SQL_VALUES);
    } else {
        goto FINAL_EXIT;
    }
    jstring_append_2(sqlval, SQL_LEFT_PAREN);
    SQL_VALUE(sqlval, jNameSpace);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jAdvType);
    jstring_append_2(sqlval, SQL_COMMA);


    SQL_VALUE(sqlval, jXml);
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jKey);
    jstring_append_2(sqlval, SQL_COMMA);
    memset(aTime, 0, 64);

    if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, lifetime_get(jpr_time_now(), timeOutForMe)) != 0) {
        jstring_append_2(sqlval, aTime);
    } else {
        status = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    memset(aTime, 0, 64);
    jstring_append_2(sqlval, SQL_COMMA);
    if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, timeOutForOthers) != 0) {
        jstring_append_2(sqlval, aTime);
    } else {
        status = JXTA_FAILED;
        goto FINAL_EXIT;
    }
    jstring_append_2(sqlval, SQL_COMMA);
    SQL_VALUE(sqlval, jGroupID);
    jstring_append_2(sqlval, SQL_RIGHT_PAREN);

    jstring_append_1(insert_sql, sqlval);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d %s -- Save Adv: %s \n", dbSpace->conn->log_id, dbSpace->id,
                    jstring_get_string(insert_sql));

    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(insert_sql));

    status = (Jxta_status) rv;

  FINAL_EXIT:

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Couldn't save advertisement %s  rc=%i\n",
                        dbSpace->conn->log_id, dbSpace->id, jstring_get_string(insert_sql), rv);
    }

    JXTA_OBJECT_RELEASE(jKey);
    JXTA_OBJECT_RELEASE(jXml);
    JXTA_OBJECT_RELEASE(sqlval);
    JXTA_OBJECT_RELEASE(insert_sql);
    JXTA_OBJECT_RELEASE(jTime);
    JXTA_OBJECT_RELEASE(jNameSpace);
    JXTA_OBJECT_RELEASE(jAdvType);
    JXTA_OBJECT_RELEASE(jGroupID);
    return status;
}

static Jxta_status cm_srdi_element_save(DBSpace * dbSpace, JString * Handler, JString * Peerid, Jxta_SRDIEntryElement * entry,
                                        Jxta_boolean replica)
{
    int nrows;
    int rv = 0;
    const char *tableName;
    Jxta_status status = JXTA_SUCCESS;
    JString *jNameSpace = NULL;
    JString *jHandler = NULL;
    JString *jPeerid = NULL;
    JString *jAdvid = NULL;
    JString *jRange = NULL;
    JString *jAttribute = NULL;
    JString *jValue = NULL;
    JString *jGroupID = NULL;
    JString *insert_sql=NULL;
    JString *sqlval=NULL;
    char aTime[64];

    if (NULL != entry->key) {
        jAttribute = jstring_clone(entry->key);
    } else {
        jAttribute = jstring_new_0();
    }

    if (entry->value) {
        jValue = jstring_clone(entry->value);
    } else {
        jValue = jstring_new_0();
    }

    if (entry->advId != NULL) {
        jAdvid = jstring_clone(entry->advId);
    } else {
      /**
       * Note: 20051004 mjan 
       * If we ended up with no key that means that it comes
       * from a Java peer. This fix emulates the behavior 
       * of the JXTA-J2SE side.
       */
      /**jKey = jstring_new_2("NoKEY");**/
        jAdvid = jstring_clone(jValue);
    }

    if (NULL != entry->range) {
        jRange = jstring_clone(entry->range);
    }

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

    if (entry->nameSpace != NULL) {
        /* assume the best */
        if (!strcmp(jstring_get_string(entry->nameSpace), "Peers")) {
            jNameSpace = jstring_new_2("jxta:PA");
        } else if (!strcmp(jstring_get_string(entry->nameSpace), "Groups")) {
            jNameSpace = jstring_new_2("jxta:PGA");
        } else {
            jNameSpace = jstring_clone(entry->nameSpace);
        }
    } else {
        jNameSpace = jstring_new_2("jxta:ADV");
    }

    if (entry->expiration > 0) {
        sqlval = jstring_new_0();
        jGroupID = jstring_new_2(dbSpace->id);
        jstring_append_2(sqlval, SQL_LEFT_PAREN);
        SQL_VALUE(sqlval, jNameSpace);
        jstring_append_2(sqlval, SQL_COMMA);
        SQL_VALUE(sqlval, jHandler);
        jstring_append_2(sqlval, SQL_COMMA);
        SQL_VALUE(sqlval, jPeerid);
        jstring_append_2(sqlval, SQL_COMMA);
        SQL_VALUE(sqlval, jAdvid);
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

        /* Timeout */
        if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, lifetime_get(jpr_time_now(), entry->expiration)) != 0) {
            jstring_append_2(sqlval, aTime);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                        "Unable to format time for me and current time - set to default timeout \n");
            apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, lifetime_get(jpr_time_now(), DEFAULT_EXPIRATION));
            jstring_append_2(sqlval, aTime);
        }
        /* Original Timeout */
        jstring_append_2(sqlval, SQL_COMMA);
        jstring_append_2(sqlval, aTime);

        /* expiration */
        jstring_append_2(sqlval, SQL_COMMA);
        memset(aTime, 0, 64);
        if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, entry->expiration) != 0) {
            jstring_append_2(sqlval, aTime);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to format time for me - set to default timeout \n");
            apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, DEFAULT_EXPIRATION);
            jstring_append_2(sqlval, aTime);
        }
        jstring_append_2(sqlval, SQL_COMMA);
        SQL_VALUE(sqlval, jGroupID);

        jstring_append_2(sqlval, SQL_COMMA);
        if (entry->dup_peerid) {
            SQL_VALUE(sqlval, entry->dup_peerid);
        } else {
            jstring_append_2(sqlval, "''");
        }
        jstring_append_2(sqlval, SQL_RIGHT_PAREN);
    }
    if (replica && !entry->duplicate) {
        tableName = CM_TBL_REPLICA;
    } else {
        tableName = CM_TBL_SRDI;
    }
    status = cm_srdi_item_found(dbSpace, tableName, jNameSpace, jHandler, jPeerid, jAdvid, jAttribute);

    if (status == JXTA_SUCCESS) {
            status =
                cm_srdi_item_update(dbSpace, tableName, jHandler, jPeerid, jAdvid, jAttribute, jValue, jRange, entry->expiration);
        goto FINAL_EXIT;

    } else if (status == JXTA_ITEM_NOTFOUND &&entry->expiration > 0) {
        insert_sql = jstring_new_0();
        jstring_append_2(insert_sql, SQL_INSERT_INTO);
        jstring_append_2(insert_sql, tableName);
        jstring_append_2(insert_sql, SQL_VALUES);
    } else if (status != JXTA_ITEM_NOTFOUND) {
         jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Error searching for srdi entry %i \n",
                            dbSpace->conn->log_id, dbSpace->id, status);
        goto FINAL_EXIT;
    }

    if (NULL != insert_sql) {
        jstring_append_1(insert_sql, sqlval);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d %s -- Save srdi: %s \n", dbSpace->conn->log_id, dbSpace->id,
                     jstring_get_string(insert_sql));
        rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(insert_sql));

        if (rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Unable to save srdi entry %s  rc=%i\n",
                        dbSpace->conn->log_id, dbSpace->id, jstring_get_string(insert_sql), rv);
            status = JXTA_FAILED;
        }
    }
  FINAL_EXIT:

    JXTA_OBJECT_RELEASE(jNameSpace);
    JXTA_OBJECT_RELEASE(jHandler);

    if (jPeerid)
        JXTA_OBJECT_RELEASE(jPeerid);
    if (jAdvid)
        JXTA_OBJECT_RELEASE(jAdvid);
    if (jAttribute)
        JXTA_OBJECT_RELEASE(jAttribute);
    JXTA_OBJECT_RELEASE(jValue);
    if (jGroupID)
        JXTA_OBJECT_RELEASE(jGroupID);
    if (jRange)
        JXTA_OBJECT_RELEASE(jRange);
    if (sqlval)
        JXTA_OBJECT_RELEASE(sqlval);
    if (insert_sql)
        JXTA_OBJECT_RELEASE(insert_sql);
    return status;
}

static Jxta_boolean cm_srdi_index_exists(DBSpace * dbSpace, JString *groupid_j, JString * jPeerid, JString * jSrcPeerid, Jxta_SRDIEntryElement * entry, Jxta_boolean bReplica)
{
    Jxta_status status = JXTA_SUCCESS;
    Jxta_boolean ret = FALSE;
    JString *where = NULL;
    apr_pool_t *pool = NULL;
    apr_status_t aprs;
    apr_dbd_results_t *res = NULL;
    int num_tuples = 0;
    JString *columns = NULL;

    aprs = apr_pool_create(&pool, NULL);
    if (aprs != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s -- Unable to create apr_pool: %d\n", dbSpace->id, aprs);
        status = JXTA_NOMEM;
        goto FINAL_EXIT;
    }


    where = jstring_new_2(CM_COL_NameSpace SQL_EQUAL);
    SQL_VALUE(where, entry->nameSpace);
    jstring_append_2(where, SQL_AND CM_COL_Peerid SQL_EQUAL);
    SQL_VALUE(where, jPeerid);
    jstring_append_2(where, SQL_AND CM_COL_SourcePeerid SQL_EQUAL);
    SQL_VALUE(where, jSrcPeerid);
    jstring_append_2(where,  SQL_AND CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(where, groupid_j);
    jstring_append_2(where,  SQL_AND CM_COL_AdvId SQL_EQUAL);
    SQL_VALUE(where, entry->advId);
    jstring_append_2(where,  SQL_AND CM_COL_Name SQL_EQUAL);
    SQL_VALUE(where, entry->key);
    jstring_append_2(where,  SQL_AND CM_COL_Replica SQL_EQUAL);
    jstring_append_2(where, bReplica == TRUE ? "1" : "0");

    columns = jstring_new_2(CM_COL_SeqNumber);

    status = cm_sql_select(dbSpace, pool, CM_TBL_SRDI_INDEX, &res, columns, where, NULL, FALSE);

    if (status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- cm_item_found Select failed: %s %i\n",
                        dbSpace->conn->log_id, dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, status), status);
        status = JXTA_FAILED;
        goto FINAL_EXIT;
    }
    num_tuples = apr_dbd_num_tuples((apr_dbd_driver_t *) dbSpace->conn->driver, res);
    if (num_tuples > 0 )
        ret = TRUE;


FINAL_EXIT:

    if (columns)
        JXTA_OBJECT_RELEASE(columns);
    if (NULL != pool)
        apr_pool_destroy(pool);
    if (where)
        JXTA_OBJECT_RELEASE(where);
    return ret;

}

static Jxta_status cm_srdi_index_save(Jxta_cm * me, const char *alias, JString * jPeerid, JString *jSrcPeerid, Jxta_SRDIEntryElement * entry,
                                      Jxta_boolean bReplica)
{
    Jxta_status status = JXTA_SUCCESS;
    int nrows;
    int rv = 0;
    JString *insert_sql = NULL;
    DBSpace *dbSpace = NULL;
    JString *jAlias = NULL;
    JString *jSeq = NULL;
    char aTmp[64];
    dbSpace = JXTA_OBJECT_SHARE(me->bestChoiceDB);
    if (cm_srdi_index_exists(dbSpace, me->jGroupID_string, jPeerid, jSrcPeerid, entry, bReplica)) {
        JXTA_OBJECT_RELEASE(dbSpace);
        /* no reason to add */
        return JXTA_SUCCESS;
    }
    insert_sql = jstring_new_0();
    jstring_append_2(insert_sql, SQL_INSERT_INTO CM_TBL_SRDI_INDEX SQL_VALUES SQL_LEFT_PAREN);
    jAlias = jstring_new_2(alias);
    SQL_VALUE(insert_sql, jAlias);
    jstring_append_2(insert_sql, SQL_COMMA);
    SQL_VALUE(insert_sql, entry->nameSpace);
    jstring_append_2(insert_sql, SQL_COMMA);
    SQL_VALUE(insert_sql, jPeerid);
    jstring_append_2(insert_sql, SQL_COMMA);
    SQL_VALUE(insert_sql, me->jGroupID_string);
    jstring_append_2(insert_sql, SQL_COMMA);
    SQL_VALUE(insert_sql, entry->advId);
    jstring_append_2(insert_sql, SQL_COMMA);
    SQL_VALUE(insert_sql, entry->key);
    jstring_append_2(insert_sql, SQL_COMMA);
    memset(aTmp, 0, sizeof(aTmp));
    apr_snprintf(aTmp, sizeof(aTmp), JXTA_SEQUENCE_NUMBER_FMT, entry->seqNumber);
    jSeq = jstring_new_2(aTmp);
    jstring_append_2(insert_sql, aTmp);
    jstring_append_2(insert_sql, SQL_COMMA);
    memset(aTmp, 0, sizeof(aTmp));

    if (apr_snprintf(aTmp, 64, "%" APR_INT64_T_FMT, lifetime_get(jpr_time_now(), entry->expiration)) != 0) {
        jstring_append_2(insert_sql, aTmp);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                        "Unable to format time for me and current time - set to default timeout \n");
        apr_snprintf(aTmp, 64, "%" APR_INT64_T_FMT, lifetime_get(jpr_time_now(), DEFAULT_EXPIRATION));
        jstring_append_2(insert_sql, aTmp);
    }
    /* original timeout */
    jstring_append_2(insert_sql, SQL_COMMA);
    jstring_append_2(insert_sql, aTmp);
    jstring_append_2(insert_sql, SQL_COMMA);
    jstring_append_2(insert_sql, bReplica == TRUE ? "1" : "0");
    jstring_append_2(insert_sql, SQL_COMMA);
    jstring_append_2(insert_sql, entry->replicate == TRUE ? "1" : "0");

    jstring_append_2(insert_sql, SQL_COMMA);
    SQL_VALUE(insert_sql, jSrcPeerid);
    jstring_append_2(insert_sql, SQL_RIGHT_PAREN);

    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(insert_sql));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "db_id: %d %s -- Save srdi Index: %s \n", dbSpace->conn->log_id, dbSpace->id,
                    jstring_get_string(insert_sql));

    JXTA_OBJECT_RELEASE(insert_sql);
    JXTA_OBJECT_RELEASE(dbSpace);
    JXTA_OBJECT_RELEASE(jAlias);
    JXTA_OBJECT_RELEASE(jSeq);
    return status;
}
#ifdef UNUSED_VWF
static Jxta_status cm_advertisement_remove(DBSpace * dbSpace, const char *nameSpace, const char *key)
{
    Jxta_status status = JXTA_SUCCESS;
    JString *where = jstring_new_0();
    JString *jKey = jstring_new_0();

    jstring_append_2(jKey, key);

    cm_sql_correct_space(nameSpace, where);

    jstring_append_2(where, SQL_AND);
    jstring_append_2(where, CM_COL_AdvId);
    jstring_append_2(where, SQL_EQUAL);
    SQL_VALUE(where, jKey);

    status = cm_sql_delete_with_where(dbSpace, CM_TBL_ELEM_ATTRIBUTES, where);
    if (status != JXTA_SUCCESS)
        goto finish;

    status = cm_sql_delete_with_where(dbSpace, CM_TBL_ADVERTISEMENTS, where);
    if (status != JXTA_SUCCESS)
        goto finish;

    status = cm_sql_delete_with_where(dbSpace, CM_TBL_SRDI, where);
    if (status != JXTA_SUCCESS)
        goto finish;

  finish:
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "db_id: %d %s -- Unable to remove Advertisement rc=%i \n %s \n",
                        dbSpace->conn->log_id, dbSpace->id, status, jstring_get_string(where));
    }
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jKey);
    return status;

}
#endif
static Jxta_status cm_item_update(DBSpace * dbSpace, const char *table, JString * jNameSpace, JString * jKey,
                                  JString * jElemAttr, JString * jVal, JString * jRange, Jxta_expiration_time timeOutForMe,
                                  Jxta_expiration_time timeOutForOthers)
{
    int nrows = 0;
    int rv = 0;
    const char *sqlCmd = NULL;
    Jxta_status status;
    char aTime[64];
    JString *update_sql = jstring_new_0();

    jstring_append_2(update_sql, SQL_UPDATE);
    jstring_append_2(update_sql, table);
    jstring_append_2(update_sql, SQL_SET CM_COL_Value SQL_EQUAL);
    SQL_VALUE(update_sql, jVal);
    jstring_append_2(update_sql, SQL_COMMA CM_COL_NumValue SQL_EQUAL);
    SQL_NUMERIC_VALUE(update_sql, jVal);
    jstring_append_2(update_sql, SQL_COMMA CM_COL_NumRange SQL_EQUAL);
    SQL_VALUE(update_sql, jRange);
    jstring_append_2(update_sql, SQL_COMMA CM_COL_TimeOut SQL_EQUAL);

    memset(aTime, 0, 64);

    if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, lifetime_get(jpr_time_now(), timeOutForMe)) != 0) {
        jstring_append_2(update_sql, aTime);
    } else {
        jstring_append_2(update_sql, "0");
    }

    jstring_append_2(update_sql, SQL_COMMA CM_COL_TimeOutForOthers SQL_EQUAL);

    memset(aTime, 0, 64);

    if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, timeOutForOthers) != 0) {
        jstring_append_2(update_sql, aTime);
    } else {
        jstring_append_2(update_sql, "0");
    }
    jstring_append_2(update_sql, SQL_WHERE CM_COL_AdvId SQL_EQUAL);
    SQL_VALUE(update_sql, jKey);
    jstring_append_2(update_sql, SQL_AND CM_COL_NameSpace SQL_EQUAL);
    SQL_VALUE(update_sql, jNameSpace);
    jstring_append_2(update_sql, SQL_AND CM_COL_Name SQL_EQUAL);
    SQL_VALUE(update_sql, jElemAttr);
    jstring_append_2(update_sql, SQL_AND CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(update_sql, dbSpace->jId);
    jstring_append_2(update_sql, SQL_END_SEMI);
    sqlCmd = jstring_get_string(update_sql);
    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, sqlCmd);

    status = (Jxta_status) rv;
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Couldn't update rc=%i \n %s \n", dbSpace->conn->log_id,
                        dbSpace->id, rv, jstring_get_string(update_sql));
    }

    JXTA_OBJECT_RELEASE(update_sql);
    return status;
}

static Jxta_status cm_srdi_item_update(DBSpace * dbSpace, const char *table, JString * jHandler, JString * jPeerid,
                                       JString * jKey, JString * jAttribute, JString * jValue, JString * jRange,
                                       Jxta_expiration_time timeOutForMe)
{
    int nrows;
    int rv;
    Jxta_status status;
    JString *update_sql = jstring_new_0();
    char aTime[64];

    if (timeOutForMe > 0) {
        jstring_append_2(update_sql, SQL_UPDATE);
        jstring_append_2(update_sql, table);
        jstring_append_2(update_sql, SQL_SET CM_COL_TimeOut SQL_EQUAL);
        memset(aTime, 0, 64);
        if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, lifetime_get(jpr_time_now(), timeOutForMe)) != 0) {
            jstring_append_2(update_sql, aTime);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to format time for me - set to default timeout \n");
            apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, lifetime_get(jpr_time_now(), DEFAULT_EXPIRATION));
            jstring_append_2(update_sql, aTime);
        }
        if (NULL != jValue) {
            jstring_append_2(update_sql, SQL_COMMA CM_COL_Value SQL_EQUAL);
            SQL_VALUE(update_sql, jValue);
            jstring_append_2(update_sql, SQL_COMMA CM_COL_NumValue SQL_EQUAL);
            SQL_NUMERIC_VALUE(update_sql, jValue);
        }
        jstring_append_2(update_sql, SQL_COMMA CM_COL_NumRange SQL_EQUAL);
        SQL_VALUE(update_sql, jRange);
    } else {
        jstring_append_2(update_sql, SQL_DELETE);
        jstring_append_2(update_sql, table);
    }
    jstring_append_2(update_sql, SQL_WHERE CM_COL_AdvId SQL_EQUAL);
    SQL_VALUE(update_sql, jKey);
    jstring_append_2(update_sql, SQL_AND CM_COL_Handler SQL_EQUAL);
    SQL_VALUE(update_sql, jHandler);
    jstring_append_2(update_sql, SQL_AND CM_COL_Name SQL_EQUAL);
    SQL_VALUE(update_sql, jAttribute);
    jstring_append_2(update_sql, SQL_AND CM_COL_Peerid SQL_EQUAL);
    SQL_VALUE(update_sql, jPeerid);
    jstring_append_2(update_sql, SQL_AND CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(update_sql, dbSpace->jId);
    jstring_append_2(update_sql, SQL_END_SEMI);

    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(update_sql));

    status = (Jxta_status) rv;
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Couldn't update rc=%i\n %s \n", dbSpace->conn->log_id,
                        dbSpace->id, rv, jstring_get_string(update_sql));
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d %s -- Updated %d - %s \n", dbSpace->conn->log_id, dbSpace->id,
                        nrows, jstring_get_string(update_sql));
    }

    JXTA_OBJECT_RELEASE(update_sql);
    return rv;
}

Jxta_status cm_update_delta_entries(Jxta_cm * self, JString *jPeerid, JString *jSourcePeerid, JString * jHandler, Jxta_vector * entries, Jxta_expiration_time next_update)
{
    Jxta_status status = JXTA_SUCCESS;
    unsigned int i;
    unsigned int v_size;

    v_size = jxta_vector_size(entries);

    for (i=0; i< v_size; i++) {
        Jxta_SRDIEntryElement *entry;
        status = jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != status) {
            continue;
        }
        status = cm_update_delta_entry(self, jPeerid, jSourcePeerid, jHandler, entry, next_update);
        JXTA_OBJECT_RELEASE(entry);

        if (JXTA_SUCCESS != status) {
            break;
        }
    }

    return status;
}

static JString ** tokenize_string_j(JString *string_j, char separator)
{
    int str_length;
    char *str_ptr = NULL;
    char *str_ptr_save = NULL;
    char *str_start = NULL;
    JString **str_array = NULL;
    JString **res;
    int str_array_length = 0;

    str_length = jstring_length(string_j);
    str_ptr = calloc(str_length + 1, 1);
    str_ptr_save = str_ptr;
    memmove(str_ptr, jstring_get_string(string_j),str_length);
    str_start = str_ptr;
    if ('\0' != *str_ptr) {
        str_array_length = 1;
    }
    while (*str_ptr != '\0') {
        if (separator == *str_ptr) {
            str_array_length++;
            *str_ptr = '\0';
        }
        str_ptr++;
    }

    str_array = calloc(str_array_length + 1, sizeof(JString*));
    res = str_array;
    while (str_array_length--) {
        *str_array = jstring_new_2(str_start);
        str_start += jstring_length(*str_array) + 1;
        str_array++;
    }
    free(str_ptr_save);
    *str_array = NULL;
    return res;
}

Jxta_status cm_update_delta_entry(Jxta_cm * self, JString * jPeerid, JString * jSourceid, JString * jHandler, Jxta_SRDIEntryElement * entry, Jxta_expiration_time next_update)
{
    Jxta_status status = JXTA_SUCCESS;
    DBSpace * dbSpace;
    JString * where;
    char aTmp[64];

    where = jstring_new_0();
    dbSpace = JXTA_OBJECT_SHARE(self->bestChoiceDB);

    jstring_append_2(where, CM_COL_Handler SQL_EQUAL);
    SQL_VALUE(where, jHandler);
    jstring_append_2(where, SQL_AND CM_COL_Peerid SQL_EQUAL);
    SQL_VALUE(where, jPeerid);
    jstring_append_2(where, SQL_AND CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(where, self->jGroupID_string);
    jstring_append_2(where, SQL_AND CM_COL_SourcePeerid SQL_EQUAL);
    SQL_VALUE(where, jSourceid);

    if (entry->seqNumber > 0) {
        memset(aTmp, 0, sizeof(aTmp));
        jstring_append_2(where, SQL_AND CM_COL_SeqNumber SQL_EQUAL);
        apr_snprintf(aTmp, sizeof(aTmp), "%" APR_INT64_T_FMT, entry->seqNumber);
        jstring_append_2(where, aTmp);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Update sequence entry - %s \n", jstring_get_string(where));

    } else if (entry->advId != NULL) {
        jstring_append_2(where, SQL_AND CM_COL_AdvId SQL_EQUAL);
        SQL_VALUE(where, entry->advId);
        if (entry->sn_cs_values) {
            JString **sn_ptr = NULL;
            JString *where_seq = NULL;
            JString **sn_ptr_save = NULL;
            where_seq = jstring_new_0();
            sn_ptr = tokenize_string_j(entry->sn_cs_values, ',');
            sn_ptr_save = sn_ptr;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Update sequence entries - %s \n", jstring_get_string(entry->sn_cs_values));
            while (*sn_ptr) {
                jstring_reset(where_seq, NULL);
                jstring_append_1(where_seq, where);
                jstring_append_2(where_seq, SQL_AND CM_COL_SeqNumber SQL_EQUAL);
                jstring_append_1(where_seq, *sn_ptr);
                JXTA_OBJECT_RELEASE(*sn_ptr);
                status = cm_sql_delta_entry_update(dbSpace, jPeerid, entry, where_seq, next_update);
                if (JXTA_SUCCESS != status) {
                    goto FINAL_EXIT;
                }
                sn_ptr++;
            }
            JXTA_OBJECT_RELEASE(where_seq);
            free(sn_ptr_save);
        }
    }

    if (NULL == entry->sn_cs_values) {
        status = cm_sql_delta_entry_update(dbSpace, jPeerid, entry, where, next_update);
    }

FINAL_EXIT:

    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(dbSpace);
    return status;
}

static Jxta_status cm_sql_delta_entry_update(DBSpace * dbSpace, JString * jPeerid, Jxta_SRDIEntryElement * entry,
                                         JString * where, Jpr_interval_time next_update)
{

    Jxta_status status;
    JString *columns = NULL;
    char aTmp[64];
    Jpr_interval_time actual_update = 0;
    Jpr_interval_time nnow;
    Jpr_interval_time timeout_time;

    nnow = jpr_time_now();

    columns = jstring_new_0();

    jstring_append_2(columns, CM_COL_TimeOut SQL_EQUAL);
    timeout_time = lifetime_get(nnow, entry->expiration);

    if (0 == next_update) {
        actual_update = 0;
    } else {
        actual_update = next_update * ((float)(100-entry->delta_window)/100);
        actual_update = lifetime_get(nnow, actual_update);
    }

    memset(aTmp, 0, sizeof(aTmp));
    apr_snprintf(aTmp, sizeof(aTmp), "%" APR_INT64_T_FMT, timeout_time);
    jstring_append_2(columns, aTmp);


    apr_snprintf(aTmp, sizeof(aTmp), "%" APR_INT64_T_FMT, actual_update);
    jstring_append_2(columns, SQL_COMMA);
    jstring_append_2(columns, CM_COL_NextUpdate SQL_EQUAL);
    jstring_append_2(columns, aTmp);

    status = cm_sql_update_with_where(dbSpace, CM_TBL_SRDI_DELTA, columns, where);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d %s -- Updated columns: %s where - %s \n", dbSpace->conn->log_id,
                        dbSpace->id, jstring_get_string(columns), jstring_get_string(where));

    JXTA_OBJECT_RELEASE(columns);
    return status;
}

static Jxta_status cm_item_insert(DBSpace * dbSpace, const char *table, const char *nameSpace, const char *key,
                                  const char *elemAttr, const char *val, const char *range, Jxta_expiration_time timeOutForMe,
                                  Jxta_expiration_time timeOutForOthers)
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
    JString *jGroupID = jstring_new_2(dbSpace->id);
    Jxta_expiration_time expTime = 0;
    char aTime[64];

    status = cm_item_found(dbSpace, table, nameSpace, jstring_get_string(jKey), elemAttr, &expTime, NULL);

    if (status == JXTA_SUCCESS) {
        if (timeOutForMe > expTime) {
            expTime = timeOutForMe;
        }
        status = cm_item_update(dbSpace, table, jNameSpace, jKey, jElemAttr, jVal, jRange, expTime, timeOutForOthers);
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

    if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, lifetime_get(jpr_time_now(), timeOutForMe)) != 0) {
        jstring_append_2(insert_sql, aTime);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to format time for me - set to default timeout \n");
        apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, lifetime_get(jpr_time_now(), DEFAULT_EXPIRATION));
        jstring_append_2(insert_sql, aTime);
    }

    jstring_append_2(insert_sql, SQL_COMMA);
    memset(aTime, 0, 64);

    if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, timeOutForOthers) != 0) {
        jstring_append_2(insert_sql, aTime);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to format time for others - set to default timeout \n");
        apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, DEFAULT_EXPIRATION);
        jstring_append_2(insert_sql, aTime);
    }
    jstring_append_2(insert_sql, SQL_COMMA);
    SQL_VALUE(insert_sql, jGroupID);
    jstring_append_2(insert_sql, SQL_RIGHT_PAREN);

    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(insert_sql));

    status = (Jxta_status) rv;

  finish:
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Couldn't insert %s  rv=%i status=%i\n",
                        dbSpace->conn->log_id, dbSpace->id, jstring_get_string(insert_sql), rv, status);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d %s -- Saved %s  rv=%i status=%i\n", dbSpace->conn->log_id,
                        dbSpace->id, jstring_get_string(insert_sql), rv, status);
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
    JXTA_OBJECT_RELEASE(jGroupID);
    return status;
}

static Jxta_status cm_item_delete(DBSpace * dbSpace, const char *table, const char *advId)
{
    Jxta_status status;
    int rv = 0;
    int nrows = 0;
    JString *statement = jstring_new_0();
    JString *jAdvId = jstring_new_0();

    jstring_append_2(jAdvId, advId);
    jstring_append_2(statement, SQL_DELETE);
    jstring_append_2(statement, table);
    jstring_append_2(statement, SQL_WHERE CM_COL_AdvId SQL_EQUAL);
    SQL_VALUE(statement, jAdvId);
    jstring_append_2(statement, SQL_AND CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(statement, dbSpace->jId);
    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(statement));

    status = (Jxta_status) rv;
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "db_id: %d %s -- Couldn't delete %s  rc=%i\n", dbSpace->conn->log_id,
                        dbSpace->id, jstring_get_string(statement), rv);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d %s -- Deleted %i  %s\n", dbSpace->conn->log_id, dbSpace->id,
                        nrows, jstring_get_string(statement));
    }

    JXTA_OBJECT_RELEASE(statement);
    JXTA_OBJECT_RELEASE(jAdvId);
    return status;
}

static Jxta_status cm_sql_delete_with_where(DBSpace * dbSpace, const char *table, JString * where)
{
    int rv = 0;
    int nrows = 0;
    Jxta_status status;
    JString *statement = jstring_new_0();

    jstring_append_2(statement, SQL_DELETE);
    jstring_append_2(statement, table);
    jstring_append_2(statement, SQL_WHERE);
    jstring_append_2(statement, CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(statement, dbSpace->jId);

    if (where != NULL) {
        jstring_append_2(statement, SQL_AND);
        jstring_append_1(statement, where);
    }

    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(statement));

    status = (Jxta_status) rv;
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "db_id: %d %s -- Couldn't delete rc=%i\n %s \n", dbSpace->conn->log_id,
                        dbSpace->id, rv, jstring_get_string(statement));
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d %s -- Deleted %i rows  %s\n", dbSpace->conn->log_id,
                        dbSpace->id, nrows, jstring_get_string(statement));
    }
    JXTA_OBJECT_RELEASE(statement);
    return status;

}

static Jxta_status cm_sql_update_with_where(DBSpace * dbSpace, const char *table, JString * columns, JString * where)
{
    int rv = 0;
    int nrows = 0;
    Jxta_status status;
    JString *statement = jstring_new_0();

    jstring_append_2(statement, SQL_UPDATE);
    jstring_append_2(statement, table);
    jstring_append_2(statement, SQL_SET);
    jstring_append_1(statement, columns);
    jstring_append_2(statement, SQL_WHERE CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(statement, dbSpace->jId);
    if (where != NULL) {
        jstring_append_2(statement, SQL_AND);
        jstring_append_1(statement, where);
    }

    rv = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(statement));

    status = (Jxta_status) rv;
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Couldn't update rc=%i\n %s \n", dbSpace->conn->log_id,
                        dbSpace->id, rv, jstring_get_string(statement));
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d %s -- Updated %i rows  %s\n", dbSpace->conn->log_id,
                        dbSpace->id, nrows, jstring_get_string(statement));
    }
    JXTA_OBJECT_RELEASE(statement);
    return status;

}

static Jxta_status cm_expired_records_remove(DBSpace * dbSpace, const char *folder_name, Jxta_boolean all)
{
    Jxta_status status = JXTA_SUCCESS;
    JString *jTime = jstring_new_0();
    JString *where = jstring_new_0();

    cm_sql_correct_space(folder_name, where);

    jstring_append_2(where, SQL_AND);
    cm_sql_append_timeout_check_less(where);
    jstring_append_2(where, SQL_AND CM_COL_GroupID SQL_EQUAL);
    SQL_VALUE(where, dbSpace->jId);
    status = cm_sql_delete_with_where(dbSpace, CM_TBL_ELEM_ATTRIBUTES, where);
    if (status != JXTA_SUCCESS)
        goto finish;
    status = cm_sql_delete_with_where(dbSpace, CM_TBL_SRDI, where);
    if (status != JXTA_SUCCESS)
        goto finish;
    status = cm_sql_delete_with_where(dbSpace, CM_TBL_REPLICA, where);
    if (status != JXTA_SUCCESS)
        goto finish;
    status = cm_sql_delete_with_where(dbSpace, CM_TBL_ADVERTISEMENTS, where);
    if (status != JXTA_SUCCESS)
        goto finish;

    if (all) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "db_id: %d Removing from bestChoice tables\n", dbSpace->conn->log_id);
        status = cm_sql_delete_with_where(dbSpace, CM_TBL_SRDI_INDEX, where);
        if (status != JXTA_SUCCESS)
            goto finish;
        status = cm_sql_delete_with_where(dbSpace, CM_TBL_SRDI_DELTA, where);
        if (status != JXTA_SUCCESS)
            goto finish;
    }

  finish:
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "db_id: %d %s -- Couldn't delete rc=%i\n %s \n", dbSpace->conn->log_id,
                        dbSpace->id, status, jstring_get_string(where));
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "db_id: %d %s -- Deleted ALL -- %s\n", dbSpace->conn->log_id,
                        dbSpace->id, jstring_get_string(where));
    }
    JXTA_OBJECT_RELEASE(where);
    JXTA_OBJECT_RELEASE(jTime);
    return status;
}

static Jxta_status cm_item_found(DBSpace * dbSpace, const char *table, const char *nameSpace, const char *advId,
                                 const char *elemAttr, Jxta_expiration_time * timeout, Jxta_expiration_time * exp)
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
    const char *timeout_column = NULL;
    const char *expiration_column = NULL;

    jstring_append_2(columns, CM_COL_TimeOut);
    if (NULL != exp) {
        jstring_append_2(columns, SQL_COMMA CM_COL_TimeOutForOthers);
    }
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
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Unable to create apr_pool: %d\n", dbSpace->conn->log_id,
                        dbSpace->id, aprs);
        status = JXTA_FAILED;
        goto finish;
    }

    rv = cm_sql_select(dbSpace, pool, table, &res, columns, where, NULL, TRUE);

    if (rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- cm_item_found Select failed: %s %i\n",
                        dbSpace->conn->log_id, dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
        status = JXTA_FAILED;
        goto finish;
    }
    for (rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);
         rv == 0; rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1)) {
        timeout_column = apr_dbd_get_entry(dbSpace->conn->driver, row, 0);
        /* if we are getting the expiration also */
        if (NULL != exp) {
            expiration_column = apr_dbd_get_entry(dbSpace->conn->driver, row, 1);
        }
        sscanf(timeout_column, jstring_get_string(jInt64_for_format), &expTemp);
        if (expTemp < jpr_time_now()) {
            *timeout = 0;
        } else {
            *timeout = (expTemp - jpr_time_now());
        }
        if (NULL != exp) {
            sscanf(expiration_column, jstring_get_string(jInt64_for_format), &expTemp);
            *exp = expTemp;
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

static Jxta_status cm_srdi_item_found(DBSpace * dbSpace, const char *table, JString * NameSpace, JString * Handler,
                                      JString * Peerid, JString * Key, JString * Attribute)
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
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- Unable to create apr_pool: %d\n", dbSpace->conn->log_id,
                        dbSpace->id, aprs);
        status = JXTA_FAILED;
        goto finish;
    }

    jstring_append_2(where, SQL_AND);
    cm_sql_append_timeout_check_greater(where);

    rv = cm_sql_select(dbSpace, pool, table, &res, columns, where, NULL, TRUE);

    if (rv) {

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- cm_srdi_item_found Select failed: %s %i\n",
                        dbSpace->conn->log_id, dbSpace->id, apr_dbd_error(dbSpace->conn->driver, dbSpace->conn->sql, rv), rv);
        status = JXTA_FAILED;
        goto finish;
    }
    for (rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1);
         rv == 0; rv = apr_dbd_get_row(dbSpace->conn->driver, pool, res, &row, -1)) {
        int column_count = apr_dbd_num_cols(dbSpace->conn->driver, res);
        status = JXTA_SUCCESS;
        for (n = 0; n < column_count; ++n) {
            entry = apr_dbd_get_entry(dbSpace->conn->driver, row, n);
            if (entry == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "db_id: %d %s -- (null) entry in cm_item_found\n",
                                dbSpace->conn->log_id, dbSpace->id);
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

static Jxta_status cm_sql_select(DBSpace * dbSpace, apr_pool_t * pool, const char *table, apr_dbd_results_t ** res,
                                 JString * columns, JString * where, JString * sort, Jxta_boolean bCheckGroup)
{
    int rv = 0;
    Jxta_status status = JXTA_SUCCESS;
    Jxta_boolean hasWhere = FALSE;
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
        hasWhere = TRUE;
    }
    if (bCheckGroup) {
        jstring_append_2(statement, hasWhere ? SQL_AND : SQL_WHERE);
        jstring_append_2(statement, CM_COL_GroupID SQL_EQUAL);
        SQL_VALUE(statement, dbSpace->jId);
    }
    if (NULL != sort && jstring_length(sort) > 0) {
        jstring_append_1(statement, sort);
    }

    rv = apr_dbd_select(dbSpace->conn->driver, pool, dbSpace->conn->sql, res, jstring_get_string(statement), 0);
    if (rv == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d %s -- %i tuples %s\n", dbSpace->conn->log_id, dbSpace->id,
                        apr_dbd_num_tuples(dbSpace->conn->driver, *res), jstring_get_string(statement));
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- error cm_sql_select %i   %s\n", dbSpace->conn->log_id,
                        dbSpace->id, rv, jstring_get_string(statement));
    }

    status = rv;
    JXTA_OBJECT_RELEASE(statement);
    return status;
}

static void cm_sql_pragma(DBSpace * dbSpace, const char *pragma)
{
    int ret;
    int nrows;
    JString *cmd;

    cmd = jstring_new_0();

    jstring_append_2(cmd, SQL_PRAGMA);
    jstring_append_2(cmd, pragma);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "db_id: %d SQLite ----- %s \n", dbSpace->conn->log_id,
                    jstring_get_string(cmd));

    ret = apr_dbd_query(dbSpace->conn->driver, dbSpace->conn->sql, &nrows, jstring_get_string(cmd));

    JXTA_OBJECT_RELEASE(cmd);
    return;
}

static Jxta_status cm_sql_select_join_generic(DBSpace * dbSpace, apr_pool_t * pool, JString * jJoin, JString * jSort,
                                              apr_dbd_results_t ** res, JString * where, Jxta_boolean bCheckGroup)
{
    int rv = 0;
    Jxta_status status;
    JString *statement = jstring_new_0();
    jstring_append_2(statement, SQL_SELECT);
    jstring_append_1(statement, jJoin);
    jstring_append_2(statement, SQL_WHERE);

    if (where != NULL && jstring_length(where) > 0) {
        jstring_append_2(statement, SQL_LEFT_PAREN);
        jstring_append_1(statement, where);
        jstring_append_2(statement, SQL_RIGHT_PAREN);
        jstring_append_2(statement, SQL_AND);
    }
    jstring_append_2(statement, CM_COL_SRC SQL_DOT);
    cm_sql_append_timeout_check_greater(statement);

    if (bCheckGroup) {
        jstring_append_2(statement, SQL_AND CM_COL_SRC SQL_DOT CM_COL_GroupID SQL_EQUAL);
        SQL_VALUE(statement, dbSpace->jId);
    }
    jstring_append_1(statement, jSort);

    jstring_append_2(statement, SQL_END_SEMI);

    rv = apr_dbd_select(dbSpace->conn->driver, pool, dbSpace->conn->sql, res, jstring_get_string(statement), 0);
    if (rv == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "db_id: %d %s -- %i tuples %s\n", dbSpace->conn->log_id, dbSpace->id,
                        apr_dbd_num_tuples(dbSpace->conn->driver, *res), jstring_get_string(statement));
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "db_id: %d %s -- error cm_sql_select %i   %s\n", dbSpace->conn->log_id,
                        dbSpace->id, rv, jstring_get_string(statement));
    }

    status = rv;
    JXTA_OBJECT_RELEASE(statement);
    return status;
}

static Jxta_status cm_sql_select_join(DBSpace * dbSpace, apr_pool_t * pool, apr_dbd_results_t ** res, JString * where)
{
    Jxta_status status;
    JString *jGroup = NULL;
    JString *jJoin = NULL;
    jJoin = jstring_new_0();
    jstring_append_2(jJoin, CM_COL_SRC SQL_DOT CM_COL_AdvId SQL_COMMA CM_COL_Advert);
    jstring_append_2(jJoin, SQL_FROM CM_TBL_ELEM_ATTRIBUTES_SRC SQL_JOIN);
    jstring_append_2(jJoin, CM_TBL_ADVERTISEMENTS_JOIN SQL_ON);
    jstring_append_2(jJoin, CM_COL_SRC SQL_DOT CM_COL_NameSpace SQL_EQUAL CM_COL_JOIN SQL_DOT CM_COL_NameSpace);
    jstring_append_2(jJoin, SQL_AND CM_COL_SRC SQL_DOT CM_COL_AdvId SQL_EQUAL CM_COL_JOIN SQL_DOT CM_COL_AdvId);
    jstring_append_2(jJoin, SQL_AND CM_COL_SRC SQL_DOT CM_COL_GroupID SQL_EQUAL CM_COL_JOIN SQL_DOT CM_COL_GroupID);

    jGroup = jstring_new_2(SQL_GROUP CM_COL_Advert);
    status = cm_sql_select_join_generic(dbSpace, pool, jJoin, jGroup, res, where, TRUE);
    JXTA_OBJECT_RELEASE(jGroup);
    JXTA_OBJECT_RELEASE(jJoin);
    return status;
}

static DBSpace *cm_dbSpace_get(Jxta_cm * self, const char *pAddressSpace)
{
    DBSpace *dbSpace = NULL;
    JString *jASName = NULL;
    int j;
    Jxta_status status;
    for (j = 0; j < jxta_vector_size(self->dbSpaces); j++) {
        status = jxta_vector_get_object_at(self->dbSpaces, JXTA_OBJECT_PPTR(&dbSpace), j);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Had a problem getting a dbspace -- %i\n", status);
            break;
        }
        jASName = jxta_cache_config_addr_get_name(dbSpace->jas);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "db_id: %d Checking %s  against %s \n", dbSpace->conn->log_id,
                        jstring_get_string(jASName), pAddressSpace);
        if (!strcmp(jstring_get_string(jASName), pAddressSpace)) {
            JXTA_OBJECT_RELEASE(jASName);
            return dbSpace;
        }
        JXTA_OBJECT_RELEASE(dbSpace);
        dbSpace = NULL;
        JXTA_OBJECT_RELEASE(jASName);
        jASName = NULL;
    }
    if (dbSpace)
        JXTA_OBJECT_RELEASE(dbSpace);
    if (jASName)
        JXTA_OBJECT_RELEASE(jASName);
    return NULL;
}

static void cm_prefix_split_type_and_check_wc(const char *pAdvType, char prefix[], const char **type, Jxta_boolean * wildcard,
                                              Jxta_boolean * wildCardPrefix)
{
    const char *ns;
    unsigned int i;
    i = 0;
    if (!strcmp(pAdvType, "Adv")) {
        ns = "jxta:*";
    } else {
        ns = pAdvType;
    }
    if ('*' == *ns) {
        *wildCardPrefix = TRUE;
        ns++;
    }
    while (*ns) {
        if ('*' == *ns) {
            *wildcard = TRUE;
        } else if (':' == *ns) {
            if ('*' != *(ns + 1)) {
                *type = (char *) ns + 1;
                prefix[i++] = '\0';
                break;
            } else {
                *wildcard = TRUE;
                prefix[i++] = '\0';
                break;
            }
        }
        prefix[i++] = *ns;
        ns++;
    }
}

static Jxta_status cm_dbSpaces_find(Jxta_cm * self, JString * jName, Jxta_boolean * nsWildCardMatch,
                                    Jxta_hashtable ** dbsReturned, DBSpace ** wildCardDB)
{
    Jxta_status status = JXTA_SUCCESS;
    DBSpace *dbRet = NULL;
    if (NULL != jName) {
        const char *keyCheck;
        keyCheck = jstring_get_string(jName);
        if (*nsWildCardMatch) {
            if (NULL != *wildCardDB) {
                JXTA_OBJECT_RELEASE(*wildCardDB);
            }
            *wildCardDB = cm_dbSpace_get(self, keyCheck);
        } else {
            dbRet = cm_dbSpace_get(self, keyCheck);
        }
    }
    if (NULL != dbRet) {
        DBSpace *dbCheck = NULL;
        const char *keyCheck = NULL;
        JString *jASName = NULL;
        jASName = jxta_cache_config_addr_get_name(dbRet->jas);
        keyCheck = jstring_get_string(jASName);
        status = jxta_hashtable_get(*dbsReturned, keyCheck, strlen(keyCheck), JXTA_OBJECT_PPTR(&dbCheck));
        if (JXTA_SUCCESS != status) {
            jxta_hashtable_put(*dbsReturned, keyCheck, strlen(keyCheck), (Jxta_object *) dbRet);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s [%pp] has been added to the dbsReturned hash\n", keyCheck,
                            dbRet);
        }
        JXTA_OBJECT_RELEASE(jASName);
        if (dbCheck)
            JXTA_OBJECT_RELEASE(dbCheck);
        JXTA_OBJECT_RELEASE(dbRet);
    }
    return status;
}

static DBSpace **cm_dbSpaces_get_priv(Jxta_cm * self, const char *pAdvType, Jxta_boolean bestChoice)
{
    unsigned int i;
    Jxta_status status;
    DBSpace **dbArray = NULL;
    DBSpace *wildCardDB = NULL;
    Jxta_boolean nsWildCardMatch = FALSE;
    Jxta_hashtable *dbsReturned;
    Jxta_vector *returnVector;
    Jxta_nameSpace *jns = NULL;
    Jxta_resolved_adv_type *resAdvType = NULL;
    char prefix[32];
    const char *type = NULL;
    Jxta_boolean wildCardPrefix = FALSE;
    Jxta_boolean wildcard = FALSE;
    prefix[0] = '\0';

    cm_prefix_split_type_and_check_wc(pAdvType, prefix, &type, &wildcard, &wildCardPrefix);

    /* quick, get 'em all */
    if (wildcard && wildCardPrefix) {
        dbArray = calloc(1, sizeof(DBSpace *) * (jxta_vector_size(self->dbSpaces) + 1));
        for (i = 0; i < jxta_vector_size(self->dbSpaces); i++) {
            jxta_vector_get_object_at(self->dbSpaces, JXTA_OBJECT_PPTR(&dbArray[i]), i);
        }
        dbArray[i] = NULL;
        return dbArray;
    }
    /* check the adv types already resolved if it's not a wild card search */
    if (!wildcard && !wildCardPrefix) {
        status = jxta_hashtable_get(self->resolvedAdvs, pAdvType, strlen(pAdvType), JXTA_OBJECT_PPTR(&resAdvType));
        if (JXTA_SUCCESS == status) {
            DBSpace **save;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Found a resolved type -- %s \n", pAdvType);
            /* for performance  return right away */
            dbArray = calloc(2, sizeof(DBSpace *));
            save = dbArray;
            *(dbArray++) = JXTA_OBJECT_SHARE(resAdvType->dbSpace);
            *(dbArray) = NULL;
            JXTA_OBJECT_RELEASE(resAdvType);
            return save;
        }
    }
    dbsReturned = jxta_hashtable_new(3);

    /* wildcard for specific prefix */
    if (!wildCardPrefix && wildcard) {
        int j;
        Jxta_vector *resolved = NULL;
        resolved = jxta_hashtable_values_get(self->resolvedAdvs);
        for (j = 0; j < jxta_vector_size(resolved); j++) {
            Jxta_boolean bDummy;
            DBSpace *dbGet;
            const char *split_type;
            char split_prefix[32];
            split_prefix[0] = '\0';
            status = jxta_vector_get_object_at(resolved, JXTA_OBJECT_PPTR(&resAdvType), j);
            if (JXTA_SUCCESS != status) {
                continue;
            }
            cm_prefix_split_type_and_check_wc((const char *) resAdvType->adv, split_prefix, &split_type, &bDummy, &bDummy);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Checking a resolved type -- %s for %s\n", split_prefix, prefix);
            if (!strcmp(split_prefix, prefix)) {
                const char *keyCheck = NULL;
                JString *jASName = NULL;
                jASName = jxta_cache_config_addr_get_name(resAdvType->dbSpace->jas);
                keyCheck = jstring_get_string(jASName);
                status = jxta_hashtable_get(dbsReturned, keyCheck, strlen(keyCheck), JXTA_OBJECT_PPTR(&dbGet));
                if (JXTA_SUCCESS == status) {
                    JXTA_OBJECT_RELEASE(dbGet);
                } else {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "found it and adding db_id: %d to dbsReturned\n",
                                    resAdvType->dbSpace->conn->log_id);
                    jxta_hashtable_put(dbsReturned, keyCheck, strlen(keyCheck), (Jxta_object *) resAdvType->dbSpace);
                }
                JXTA_OBJECT_RELEASE(jASName);
            }
            JXTA_OBJECT_RELEASE(resAdvType);
        }
        if (resolved) {
            JXTA_OBJECT_RELEASE(resolved);
        }
    }
    /* match the adv type with a DBSpace to registered name spaces */
    for (i = 0; i < jxta_vector_size(self->nameSpaces); i++) {
        const char *advType = NULL;
        JString *jPrefix = NULL;
        JString *jType = NULL;
        JString *jAdvType = NULL;
        JString *jName = NULL;
        const char *nsPrefix;
        const char *nsType;
        Jxta_boolean isContinue = FALSE;
        nsWildCardMatch = FALSE;
        status = jxta_vector_get_object_at(self->nameSpaces, JXTA_OBJECT_PPTR(&jns), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Had a problem getting a namespace -- %i\n", status);
            continue;
        }
        jName = jxta_cache_config_ns_get_addressSpace_string(jns);
        jPrefix = jxta_cache_config_ns_get_prefix(jns);
        nsPrefix = jstring_get_string(jPrefix);
        jType = jxta_cache_config_ns_get_adv_type(jns);
        nsType = jstring_get_string(jType);
        if (!wildCardPrefix) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "nsPrefix - %s prefix - %s \n", nsPrefix, &prefix[0]);
            if (strcmp(nsPrefix, &prefix[0])) {
                isContinue = TRUE;
            } else if (jxta_cache_config_ns_is_type_wildcard(jns)) {
                nsWildCardMatch = TRUE;
            }
        }
        if (!nsWildCardMatch && !isContinue) {
            if (!wildcard) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "nsType - %s type - %s \n", nsType, type);
                if (strcmp(nsType, type)) {
                    isContinue = TRUE;
                }
            } else if (!wildCardPrefix) {
                if (strcmp(nsPrefix, &prefix[0])) {
                    isContinue = TRUE;
                }
            }
        }
        JXTA_OBJECT_RELEASE(jType);
        JXTA_OBJECT_RELEASE(jPrefix);
        if (isContinue) {
            JXTA_OBJECT_RELEASE(jns);
            JXTA_OBJECT_RELEASE(jName);
            jName = NULL;
            continue;
        }
        jAdvType = jxta_cache_config_ns_get_ns(jns);
        advType = jstring_get_string(jAdvType);

        status = cm_dbSpaces_find(self, jName, &nsWildCardMatch, &dbsReturned, &wildCardDB);
        JXTA_OBJECT_RELEASE(jName);
        JXTA_OBJECT_RELEASE(jAdvType);
        JXTA_OBJECT_RELEASE(jns);
    }
    returnVector = jxta_hashtable_values_get(dbsReturned);

    if (0 == jxta_vector_size(returnVector)) {
        if (wildCardDB) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "wildCard DB has been returned\n");
            jxta_vector_add_object_last(returnVector, (Jxta_object *) wildCardDB);
        } else if (bestChoice) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "bestChoice DB has been returned\n");
            jxta_vector_add_object_last(returnVector, (Jxta_object *) self->bestChoiceDB);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "default DB has been returned\n");
            jxta_vector_add_object_last(returnVector, (Jxta_object *) self->defaultDB);
        }
    }
    dbArray = calloc(jxta_vector_size(returnVector) + 1, sizeof(DBSpace *));
    for (i = 0; i < jxta_vector_size(returnVector); i++) {
        status = jxta_vector_get_object_at(returnVector, JXTA_OBJECT_PPTR(&dbArray[i]), i);
    }
    dbArray[i] = NULL;
    JXTA_OBJECT_RELEASE(returnVector);
    if (wildCardDB)
        JXTA_OBJECT_RELEASE(wildCardDB);
    if (dbsReturned)
        JXTA_OBJECT_RELEASE(dbsReturned);
    return dbArray;
}

static DBSpace **cm_dbSpaces_get(Jxta_cm * self, const char *pAdvType, Jxta_credential ** scope, Jxta_boolean bestChoice)
{
    DBSpace **dbSpaces = NULL;
    int i;
    if (NULL == scope) {
        dbSpaces = cm_dbSpaces_get_priv(self, pAdvType, bestChoice);
    } else {
        for (i = 0; i < jxta_vector_size(global_cm_registry); i++) {
            Jxta_cm *tmp_cm;
            DBSpace **tmp;
            JString *jGid;
            Jxta_credential **scopeSave;
            jxta_vector_get_object_at(global_cm_registry, JXTA_OBJECT_PPTR(&tmp_cm), i);
            scopeSave = scope;
            tmp = NULL;
            while (*scope) {
                Jxta_id *jId;
                jxta_credential_get_peergroupid(*scope, &jId);
                jxta_id_get_uniqueportion(jId, &jGid);
                JXTA_OBJECT_RELEASE(jId);
                if (0 == jstring_equals(tmp_cm->jGroupID_string, jGid)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "getting dbSpaces from group: %s\n",
                                    jstring_get_string(jGid));
                    tmp = cm_dbSpaces_get_priv(tmp_cm, pAdvType, bestChoice);
                    JXTA_OBJECT_RELEASE(jGid);
                    break;
                }
                JXTA_OBJECT_RELEASE(jGid);
                scope++;
            }
            scope = scopeSave;
            dbSpaces = (DBSpace **) cm_arrays_concatenate((Jxta_object **) dbSpaces, (Jxta_object **) tmp);
            if (tmp)
                free(tmp);
            JXTA_OBJECT_RELEASE(tmp_cm);
        }
        if (NULL == dbSpaces) {
            dbSpaces = cm_dbSpaces_get_priv(self, pAdvType, bestChoice);
        }
    }
    return dbSpaces;
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

    if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, tNow) != 0) {
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

    if (apr_snprintf(aTime, 64, "%" APR_INT64_T_FMT, tNow) != 0) {
        jstring_append_2(cmd, aTime);
    } else {
        jstring_append_2(cmd, "0");
    }
}

Jxta_boolean cm_sql_escape_and_wc_value(JString * jStr, Jxta_boolean replace)
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

void cm_sql_numeric_quote(JString * jDest, JString * jStr, Jxta_boolean isNumeric)
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

JXTA_DECLARE(Jxta_cm *) jxta_cm_new(const char *home_directory, Jxta_id * group_id, Jxta_PID * localPeerId)
{
    JXTA_DEPRECATED_API();
    return cm_new(home_directory, group_id, NULL, localPeerId, NULL);
}

/* remove an advertisement given the group_id, type and advertisement name */
JXTA_DECLARE(Jxta_status) jxta_cm_remove_advertisement(Jxta_cm * self, const char *folder_name, char *primary_key)
{
    JXTA_DEPRECATED_API();
    return cm_remove_advertisement(self, folder_name, primary_key);
}

/* return a pointer to all advertisements in folder folder_name */
/* a NULL pointer means the end of the list */
JXTA_DECLARE(char **) jxta_cm_get_primary_keys(Jxta_cm * self, char *folder_name)
{
    JXTA_DEPRECATED_API();
    return cm_sql_get_primary_keys(self, folder_name, CM_TBL_ADVERTISEMENTS, NULL, NULL, -1);
}

/* save advertisement */
JXTA_DECLARE(Jxta_status) jxta_cm_save(Jxta_cm * self, const char *folder_name, char *primary_key,
                                       Jxta_advertisement * adv, Jxta_expiration_time timeOutForMe,
                                       Jxta_expiration_time timeOutForOthers)
{
    JXTA_DEPRECATED_API();
    return cm_save(self, folder_name, primary_key, adv, timeOutForMe, timeOutForOthers, TRUE);
}

JXTA_DECLARE(Jxta_status) jxta_cm_save_srdi(Jxta_cm * self, JString * handler, JString * peerid, JString * primaryKey,
                                            Jxta_SRDIEntryElement * entry)
{
    JXTA_DEPRECATED_API();
    return cm_save_srdi(self, handler, peerid, primaryKey, entry);
}

JXTA_DECLARE(Jxta_status) jxta_cm_save_replica(Jxta_cm * self, JString * handler, JString * peerid, JString * primaryKey,
                                               Jxta_SRDIEntryElement * entry)
{
    DBSpace **dbSpaces = NULL;
    DBSpace **dbSpacesSave = NULL;
    Jxta_status status = JXTA_SUCCESS;
    JXTA_DEPRECATED_API();
    dbSpaces = cm_dbSpaces_get(self, jstring_get_string(entry->nameSpace), NULL, FALSE);
    dbSpacesSave = dbSpaces;
    if (*dbSpaces) {
        status = cm_srdi_element_save(*dbSpaces, handler, peerid, entry, TRUE);
        if (JXTA_SUCCESS == status) {
            cm_name_space_register(self, *dbSpaces, jstring_get_string(entry->nameSpace));
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve dbSpace for %s name space\n",
                        jstring_get_string(entry->nameSpace));
    }
    while (*dbSpaces) {
        JXTA_OBJECT_RELEASE(*dbSpaces);
        dbSpaces++;
    }
    free(dbSpacesSave);
    return status;
}

/* search for advertisements with specific (attribute,value) pairs */
/* a NULL pointer means the end of the list */
JXTA_DECLARE(char **) jxta_cm_search(Jxta_cm * self, char *folder_name, const char *aattribute, const char *value, int n_adv)
{
    JXTA_DEPRECATED_API();
    return cm_search(self, folder_name, aattribute, value, FALSE, n_adv);
}

/* search for advertisements with specific (attribute,value) pairs */
/* a NULL pointer means the end of the list */
JXTA_DECLARE(char **) jxta_cm_search_srdi(Jxta_cm * self, char *folder_name, const char *aattribute, const char *value, int n_adv)
{
    JXTA_DEPRECATED_API();
    return cm_search_srdi(self, folder_name, aattribute, value, n_adv);
}

JXTA_DECLARE(Jxta_advertisement **) jxta_cm_sql_query(Jxta_cm * self, const char *folder_name, JString * where)
{
    JXTA_DEPRECATED_API();
    return cm_sql_query(self, folder_name, where);
}

JXTA_DECLARE(Jxta_vector *) jxta_cm_query_replica(Jxta_cm * self, JString * nameSpace, Jxta_vector * queries)
{
    JXTA_DEPRECATED_API();
    return cm_query_replica(self, nameSpace, queries);
}

/* restore the original advertisement from the cache */
JXTA_DECLARE(Jxta_status) jxta_cm_restore_bytes(Jxta_cm * self, char *folder_name, char *primary_key, JString ** bytes)
{
    JXTA_DEPRECATED_API();
    return cm_restore_bytes(self, folder_name, primary_key, bytes);
}

JXTA_DECLARE(Jxta_status) jxta_cm_get_others_time(Jxta_cm * self, char *folder_name, char *primary_key,
                                                  Jxta_expiration_time * ttime)
{
    JXTA_DEPRECATED_API();
    return cm_get_time(self, folder_name, primary_key, ttime, CM_COL_TimeOutForOthers);
}

/* Get the expiration time of an advertisement */
JXTA_DECLARE(Jxta_status) jxta_cm_get_expiration_time(Jxta_cm * self, char *folder_name, char *primary_key,
                                                      Jxta_expiration_time * ttime)
{
    JXTA_DEPRECATED_API();
    return cm_get_time(self, folder_name, primary_key, ttime, CM_COL_TimeOut);
}

/* delete files where the expiration time is less than curr_expiration */
JXTA_DECLARE(Jxta_status) jxta_cm_remove_expired_records(Jxta_cm * self)
{
    JXTA_DEPRECATED_API();
    return cm_remove_expired_records(self);
}

/* create a folder with type <type> and index keywords <keys> */
JXTA_DECLARE(Jxta_status) jxta_cm_create_folder(Jxta_cm * self, char *folder_name, const char *keys[])
{
    JXTA_DEPRECATED_API();
    return cm_create_folder(self, folder_name, keys);
}

JXTA_DECLARE(Jxta_status) jxta_cm_create_adv_indexes(Jxta_cm * self, char *folder_name, Jxta_vector * jv)
{
    JXTA_DEPRECATED_API();
    return cm_create_adv_indexes(self, folder_name, jv);
}

/*
 * hash a document
 */
JXTA_DECLARE(unsigned long) jxta_cm_hash(const char *key, size_t ksz)
{
    JXTA_DEPRECATED_API();
    return cm_hash(key, ksz);
}

/* temporary workaround for closing the database.
 * The cm will be freed automatically when group stoping realy works
 */

JXTA_DECLARE(void) jxta_cm_close(Jxta_cm * self)
{
    JXTA_DEPRECATED_API();
    cm_close(self);
}

/*
 * obtain all CM SRDI delta entries for a particular folder
 */
JXTA_DECLARE(Jxta_vector *) jxta_cm_get_srdi_delta_entries(Jxta_cm * self, JString * folder_name)
{
    JXTA_DEPRECATED_API();
    return cm_get_srdi_delta_entries(self, folder_name);
}

JXTA_DECLARE(void) jxta_cm_set_delta(Jxta_cm * self, Jxta_boolean recordDelta)
{
    JXTA_DEPRECATED_API();
    cm_set_delta(self, recordDelta);
}

/*
 * obtain all CM SRDI entries for a particular folder
 */
JXTA_DECLARE(Jxta_vector *) jxta_cm_get_srdi_entries(Jxta_cm * self, JString * folder_name)
{
    JXTA_DEPRECATED_API();
    return cm_create_srdi_entries(self, folder_name, NULL, NULL);
}

JXTA_DECLARE(void) jxta_cm_remove_srdi_entries(Jxta_cm * self, JString * peerid)
{
    JXTA_DEPRECATED_API();
    cm_remove_srdi_entries(self, peerid, NULL);
}

JXTA_DECLARE(Jxta_boolean) jxta_sql_escape_and_wc_value(JString * jStr, Jxta_boolean replace)
{
    JXTA_DEPRECATED_API();
    return cm_sql_escape_and_wc_value(jStr, replace);
}

JXTA_DECLARE(void) jxta_sql_numeric_quote(JString * jDest, JString * jStr, Jxta_boolean isNumeric)
{
    JXTA_DEPRECATED_API();
    cm_sql_numeric_quote(jDest, jStr, isNumeric);
}


/* vi: set ts=4 sw=4 tw=130 et: */
