/* 
 * Copyright (c) 2002-2006 Sun Microsystems, Inc.  All rights reserved.
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
#ifndef __JXTA_CM_PRIVATE_H__
#define __JXTA_CM_PRIVATE_H__

#include <apr_dbd.h>
#include "jxta_advertisement.h"
#include "jxta_cred.h"
#include "jxta_query.h"
#include "jxta_srdi.h"
#include "jxta_cache_config_adv.h"
#include "jxta_cm.h"
#include "jxta_proffer.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif
/**
* table names for SQL
* the 'as' is created to allow where clauses to be generic 
**/

#define CM_TBL_ELEM_ATTRIBUTES "tblElementAttributes"
#define CM_TBL_ELEM_ATTRIBUTES_SRC "tblElementAttributes as src "
#define CM_TBL_ELEM_ATTRIBUTES_JOIN "tblElementAttributes as jn "

#define CM_TBL_SRDI "tblSRDI"
#define CM_TBL_SRDI_SRC "tblSRDI as src "
#define CM_TBL_SRDI_JOIN "tblSRDI as jn "

#define CM_TBL_REPLICA "tblReplica"
#define CM_TBL_REPLICA_SRC "tblReplica as src "
#define CM_TBL_REPLICA_JOIN "tblReplica as jn "

#define CM_TBL_ADVERTISEMENTS "tblAdvertisement"
#define CM_TBL_ADVERTISEMENTS_SRC "tblAdvertisement as src "
#define CM_TBL_ADVERTISEMENTS_JOIN "tblAdvertisement as jn "

#define CM_TBL_SRDI_DELTA "tblSRDIDelta"
#define CM_TBL_SRDI_DELTA_SRC "tblSRDIDelta as src "
#define CM_TBL_SRDI_DELTA_JOIN "tblSRDIDelta as jn "

#define CM_TBL_SRDI_INDEX "tblSRDIIndex"
#define CM_TBL_SRDI_INDEX_SRC "tblSRDIIndex as src "
#define CM_TBL_SRDI_INDEX_JOIN "tblSRDIIndex as jn "

#define CM_COL_SRC " src"
#define CM_COL_JOIN " jn"

/* table colum values */

#define CM_COL_GroupID "GroupID"
#define CM_COL_NameSpace "NameSpace"
#define CM_COL_AdvType "AdvType"
#define CM_COL_Handler "Handler"
#define CM_COL_Peerid "PeerID"
#define CM_COL_AdvId "AdvId"
#define CM_COL_Name "Name"
#define CM_COL_Value "Value"
#define CM_COL_NumValue "NumValue"
#define CM_COL_NumRange "RangeValue"
#define CM_COL_TimeOut "TimeOut"
#define CM_COL_OriginalTimeout "TimeOutOriginal"
#define CM_COL_TimeOutForOthers "TimeOutOthers"
#define CM_COL_NextUpdate "NextUpdate"
#define CM_COL_DeltaWindow "DeltaWindow"
#define CM_COL_Advert "Advert"
#define CM_COL_SeqNumber "SeqNumber"
#define CM_COL_DBAlias "DBAlias"
#define CM_COL_Replica "Replica"
#define CM_COL_CachedLocal "CachedLocal"
#define CM_COL_Replicate "Replicate"
#define CM_COL_Duplicate "Duplicate"
#define CM_COL_Radius "Radius"
#define CM_COL_DupPeerid "DupPeerid"
#define CM_COL_SourcePeerid "Sourceid"
#define CM_COL_Fwd "Fwd"
#define CM_COL_FwdPeerid "FwdPeerid"
#define CM_COL_RepPeerid "RepPeerid"

typedef struct jxta_cache_entry Jxta_cache_entry;

struct jxta_cache_entry {
    JXTA_OBJECT_HANDLE;
    Jxta_expiration_time lifetime;
    Jxta_expiration_time expiration;
    Jxta_ProfferAdvertisement *profadv;
    JString *profid;
    Jxta_advertisement  * adv;
} ;

typedef struct jxta_replica_entry Jxta_replica_entry;

struct jxta_replica_entry {
    JXTA_OBJECT_HANDLE;
    JString *peer_id;
    JString *adv_id;
    JString *db_alias;
    JString *name;
    JString *dup_id;
};

/**
 * Create a new Jxta_cm object.
 *
 * @param home_directory The directory where to store the files.
 * @param group_id The ID of the group for which this Jxta_cm works.
 * @param config_adv Config advertisement of the Peer
 * @param localPeerId PeerId of this peer
 * @return Jxta_cm* (A ptr to) the newly created Jxta_cm object.
 */
Jxta_cm * cm_new(const char *home_directory, Jxta_id * group_id, Jxta_CacheConfigAdvertisement * conf_adv,
                                      Jxta_PID * localPeerId, apr_thread_pool_t * thread_pool);

/**
 * Create a Jxta_cm object that shares the DB resources created by another CM
 *
 * @param cm CM that contains the resources to share.
 */
Jxta_cm * cm_shared_DB_new(Jxta_cm *cm,  Jxta_id * group_id);

/**
 * Stop providing CM services. This makes the CM unavailable and cancels
 * any scheduled tasks.
 *
 * @param Jxta_cm the CM object
*/
void cm_stop(Jxta_cm *cm);

/**
 * Create a new folder.
 * This operation is idem-potent.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param keys secondary keys; a null terminated array of char*.
 * @return Jxta_status JXTA_SUCCESS if the folder was created ok.
 */
Jxta_status cm_create_folder(Jxta_cm * self, char *folder_name, const char *keys[]);

/**
 * Create secondary indexes for the specified keys.
 * 
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param adv_descriptor - This is the name space and advertisement type
 *                         "namespace:advertisement type"
 * @param keys the element/attribute pairs that define the indexes of the advertisement
 * @return Jxta_status JXTA_SUCCESS if the schema was created ok.
*/
Jxta_status cm_create_adv_indexes(Jxta_cm * self, char *folder_name, Jxta_vector * keys);

/**
 * Remove adv of given primary_key from given folder.
 * This operation will fail if the folder does not exist, but will
 * not complain about removing an item that does not exist.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param primary_key the primary_key
 * @return Jxta_status JXTA_SUCCESS if removed, an error code otherwise
 */
Jxta_status cm_remove_advertisement(Jxta_cm * cm, const char *folder_name, char *primary_key);

/**
 * Remove advertisements of given array of keys from given folder.
 * This operation will fail if the folder does not exist, but will
 * not complain about removing an item that does not exist.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param keys array of keys to remove
 * @return Jxta_status JXTA_SUCCESS if removed, an error code otherwise
 */
Jxta_status cm_remove_advertisements(Jxta_cm * self, const char *folder_name, char **keys);


char **cm_sql_get_primary_keys(Jxta_cm * self, char *folder_name, const char *tableName, JString * jWhere,
                                      JString * jGroup, int row_max);

/**
 * Return a pointer to each primary key of all advertisements in the given
 * folder.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param no_locals if True only return advs with expiration > 0
 * @return char** A null terminated array of char*. If the folder does
 * not exist, NULL is returned.
 */
char ** cm_get_primary_keys(Jxta_cm * cm, char *folder_name, Jxta_boolean no_locals);

/**
 * Save the given advertisement in the given folder.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param primary_key the primary key with which to associate that advertisement.
 * @param adv (A ptr to) the advertisement.
 * @param timeOutForMe The life time of the advertisement in the local (publisher's)
 * cache.
 * @param timeOutForOthers The prescribed expiration time of the advertisement in
 * other peer's cache.
 * @param update Indicates whether an existing advertisement will be updated.
 *                  true - update will be performed.
 *                  false - no update.
 */
Jxta_status cm_save(Jxta_cm * cm, const char *folder_name, char *primary_key,
             Jxta_advertisement * adv, Jxta_expiration_time timeOutForMe, Jxta_expiration_time timeOutForOthers,
             Jxta_boolean update);

/**
 * Save the SRDI entry in the given folder.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param primary_key the primary key with which to associate that advertisement.
 * @param entries Jxta_SRDIEntryElement
 */
Jxta_status cm_save_srdi(Jxta_cm * self, JString * handler, JString * peerid, JString * primaryKey, Jxta_SRDIEntryElement * entry);

/**
 * Save the SRDI entry in the given folder.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param peerid The publishing peer of the entries
 * @param src_peerid The publishing peer of the SRDI entries when the entry is from a replicating peer
 * @param primary_key the primary key with which to associate that advertisement.
 * @param entries vector of Jxta_SRDIEntryElement
 * @param resendEntries vector of SRDI elements to resend
 */
Jxta_status cm_save_srdi_elements(Jxta_cm * self, JString * handler, JString * peerid, JString *src_peerid, JString * primaryKey, Jxta_vector * entries, Jxta_vector **resendEntries);

/**
 * Get a SRDI entry by sequence number and populate the entry.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param jPeerid Peerid of srdi entry
 * @param seqNumber Sequence number assigned from the source peerid.
 * @param entry Location to store a new SRDI Entry element.
 *
 */
Jxta_status cm_get_srdi_with_seq_number(Jxta_cm * me, JString * jPeerid, Jxta_sequence_number seq, Jxta_SRDIEntryElement ** entry);

/**
 * Remove the SRDI entries in the SRDI Delta table for the peer.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param jPeerid Peer ID of the entries
 * @param entries vector of Jxta_SRDIEntryElement
 */
void cm_remove_srdi_delta_entries(Jxta_cm * self, JString * jPeerid, Jxta_vector * entries);

Jxta_status cm_get_delta_entries_for_update(Jxta_cm * self, const char *name, JString *peer, Jxta_hashtable ** entries);

Jxta_status cm_update_delta_entries(Jxta_cm * self, JString *jPeerid, JString *jSourcePeerid, JString * jHandler, Jxta_vector * entries, Jxta_expiration_time next_update);

Jxta_status cm_update_delta_entry(Jxta_cm * self, JString * jPeerid, JString * jSourceid, JString * jHandler, Jxta_SRDIEntryElement * entry, Jxta_expiration_time next_update);

Jxta_status cm_expand_delta_entries(Jxta_cm * self, Jxta_vector * msg_entries, JString * peerid, JString * source_peerid, Jxta_vector ** ret_entries);

Jxta_status cm_remove_delta_entries(Jxta_cm * me, JString *seq_entries);

/**
 * Save the SRDI entry in the SRDI Delta table for the peer.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param jPeerid Peer ID for the entry
 * @param jOrigPeerid Origin Peer Id of the message
 * @param jSourcePeerid Peer ID that is the source for the SRDI message
 * @param jHandler Handler for the entry
 * @param entry Jxta_SRDIEntryElement to save
 * @param within_radius This entry is within the hash radius
 * @param jNewValue New value if there is an entry in the table for the sequence number
 * @param newSequenceNumber New seqeuence number if an entry exists for this entry
 * @param update_srdi If set to true the threshold has been met for updating the srdi entry at the rendezvous
 * @param window The percentage of the expiration used to send SRDI delta updates for duplicates
 *
 * @return Jxta_status 
 */
Jxta_status cm_save_delta_entry(Jxta_cm * me, JString * jPeerid, JString * jSourcePeerid, JString *jOrigPeerid, JString *jAdvPeer, JString * jHandler,
                            Jxta_SRDIEntryElement * entry, Jxta_boolean within_radius, JString ** jNewValue, Jxta_sequence_number * newSeqNumber,
                            JString **jRemovePeerid, JString ** jRemoveSeqNumber, Jxta_boolean * update_srdi, int window);


/**
 * Get the entries with the sequence number in resendEntries. A hashtable is returned keyed by the peerid where entries have been sent.
 * When an entry has been forwarded the keyed peers will contain the peer of the replicating peer.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param peerid_j JString pointer of the peerid string
 * @param resendEntries Jxta_vector of Jxta_SRDIEntryElement
 * @param resend_entries Location to store Jxta_hashtable of Jxta_SRDIEntryElement keyed by the source id with matching sequence numbers 
 * @param reverse_entries Location to store Jxta_hashtable of Jxta_SRDIEntryElement keyed by the replicating peer (Forwarded entries)
 * @return Jxta_status
 */
Jxta_status cm_get_resend_delta_entries(Jxta_cm * me, JString * peerid_j, Jxta_vector * resendEntries, Jxta_hashtable ** resend_entries, Jxta_hashtable ** reverse_entries);

/**
 * Save the Replica entry in the given folder 
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param primary_key the primary key with which to associate that advertisement.
 * @param entries vector of Jxta_SRDIEntryElement
 */
Jxta_status cm_save_replica(Jxta_cm * self, JString * handler, JString * peerid, JString * primaryKey,
                                               Jxta_SRDIEntryElement * entry);
/**
 * Save the Replica entry in the given folder 
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param primary_key the primary key with which to associate that advertisement.
 * @param entries vector of Jxta_SRDIEntryElement
 */

Jxta_status cm_save_replica_elements(Jxta_cm * self, JString * handler, JString * peerid, JString * primaryKey, Jxta_vector * entries, Jxta_vector ** resendEntries);

Jxta_status cm_save_replica_fwd_elements(Jxta_cm * me, JString * handler, JString * peerid, JString *src_peerid, JString * primaryKey,
                                     Jxta_vector * entries, Jxta_vector ** resendEntries);

/**
 * Search for advertisements with specific (attribute,value) pairs
 * What is returned is a null terminated list of the primary keys of
 * items that match.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param aattribute the selector attribute
 * @param value the selector value
 * @param no_locals if true don't return zero expiration entries
 * @param n_adv The maximum number of advs to retrieved
 * @return A null terminated list of char* each pointing at the primary key
 * of one matching advertisement.
 */
char ** cm_search(Jxta_cm * cm, char *folder_name, const char *aattribute, const char *value, Jxta_boolean no_locals, int n_adv);

/**
 * Search for advertisements that match the XPath type query.
 * What is returned is a null terminated list of the primary keys of
 * items that match.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param query - XPath type query
 * @param n_adv The maximum number of advs to retrieve.
 * @return A null terminated list of char* each pointing at the primary key
 * of one matching advertisement.
 */
char ** cm_query(Jxta_cm * cm, char *folder_name, const char *query, int n_adv);

/**
 * Search for advertisements with a raw SQL query WHERE string.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param where - A SQL string that is concatenated to a WHERE clause.
 * @return A null terminated list of Jxta_advertisement* each pointing at an instance 
 * a matching advertisement
 */
Jxta_advertisement ** cm_sql_query(Jxta_cm * self, const char *folder_name, JString * where);

/**
 * Search for advertisements with a raw SQL query WHERE string. Jxta_cache_entries
 * are returned that contain the proffer Adv and the advertisement found.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param jContext - A query Context.
 * @return A null terminated list of Jxta_cache entries
 */
struct jxta_cache_entry ** cm_query_ctx(Jxta_cm * self, Jxta_credential **scope, int threshold, Jxta_query_context * jContext);

/**
 * Search for entries in the SRDI with a raw SQL query WHERE string.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param ns name space for the query
 * @param where - A WHERE clause SQL string.
 * @return A null terminated list of Jxta_cache_entry with advid as meta data.
 */
Jxta_cache_entry ** cm_sql_query_srdi_ns(Jxta_cm * self, const char *ns, JString * where);

/**
 * Search for entries in the Replica name space given a vector of Jxta_query_element entries .
 * The results are filtered based on the least popular entry. When the entries are
 * retrieved they are sorted by peerid and the AND booleans in the predicate
 * elements. The least popular results are returned.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param nameSpace - the namespace (ns:name) to be queried
 * @param queries - A vector of SQL predicates 
 * @return A vector of peerIds that match the entries searched.
 */
Jxta_vector * cm_query_replica(Jxta_cm * self, JString * nameSpace, Jxta_vector * queries);

/**
 * Restore the designated advertisement from storage.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param primary_key the primary key with which to associate that advertisement.
 * @param bytes A location where to store (A ptr to) a jstring containing
 * the data in serialized (xml) form.
 * @return JXTA_SUCCESS if the advertisement was found and retrieved, an error code
 * otherwise.
 */
Jxta_status cm_restore_bytes(Jxta_cm * cm, char *folder_name, char *primary_key, JString ** bytes);

/**
 * Get the expiration time of an advertisement.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param primary_key the primary key with which to associate that advertisement.
 * @param ttime The expiration time of that advertisement.
 * @return Jxta_status JXTA_SUCCESS if the advertisement exists and has an expiration time. An error
 * code otherwise.
 */
Jxta_status cm_get_expiration_time(Jxta_cm * cm, char *folder_name, char *primary_key,
                                                      Jxta_expiration_time * ttime);
/**
 * Get the timeoutforothers expiration time of an advertisement.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param primary_key the primary key with which to associate that advertisement.
 * @param ttime The timeoutforothers expiration time of that advertisement.
 * @return Jxta_status JXTA_SUCCESS if the advertisement exists and has an expiration time. An error
 * code otherwise.
 */
Jxta_status cm_get_others_time(Jxta_cm * self, char *folder_name, char *primary_key,
                                                  Jxta_expiration_time * ttime);

/**
 * delete files which life-time has ended.
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @return Jxta_status JXTA_SUCCESS. (errors ?).
 */
Jxta_status cm_remove_expired_records(Jxta_cm * cm);

/**
 * Generate a hash key for the given buffer.
 * @param str The address of the data.
 * @param len The length of the data.
 * @return unsigned long The resulting hash key.
 */
unsigned long cm_hash(const char *str, size_t len);


/** temporary workaround for closing the database.
 * The cm will be freed automatically when group stopping really works
 */
void cm_close(Jxta_cm * cm);

/**
 * retrun SRDI entries from the element/attributes table
 *
 * @param Jxta_cm instance
 * @param folder name
 * @param jAdvId optional advid - returns only the entries of the advID
 * @param jPeerId optional peerid - returns all entries for a given Peer
 *
 * @return a vector of SRDIElementEntry
 */
Jxta_vector * cm_create_srdi_entries(Jxta_cm * self, JString * folder_name, JString * jAdvId, JString * jPeerId);

/**
 * return SRDI entries from the SRDI table
 *
 * @param Jxta_cm instance
 * @param folder name
 * @param jAdvId optional advid - returns only the entries of the advID
 * @param jPeerId optional peerid - returns all entries for a given Peer
 *
 * @return a vector of SRDIElementEntry
 */
Jxta_vector * cm_get_srdi_entries_1(Jxta_cm * self, JString * folder_name, JString * jAdvId, JString * jPeerId);

/**
 * Remove the SRDI entries for the specified peer
 * 
 * @param Jxta_cm instance
 * @param const char * id - Peerid to remove
 * @param Jxta_vector **srdi_entries - Return of all entries that are removed
 *
**/
void cm_remove_srdi_entries(Jxta_cm * self, JString * peerid, Jxta_vector **srdi_entries);


/**
 * Update SRDI/Replica times to reflect the length of the lease
 *
 * @param Jxta_cm instance
 * @param jPeerid string of peerid to udate
 * @param time limit entries to this time
*/
Jxta_status cm_update_srdi_times(Jxta_cm * me, JString * jPeerid, Jxta_time ttime);

/**
 * get the new SRDI index entry for the provided folder since
 * we last pushed them
 *
 * @param Jxta_cm instance
 * @param folder name
 *
 * @return a vector od SRDIElementEntry
 */
Jxta_vector * cm_get_srdi_delta_entries(Jxta_cm * self, JString * folder_name);

Jxta_boolean cm_sql_escape_and_wc_value(JString * jStr, Jxta_boolean replace);

/**
 * Function to create the proper quotes for a string in a SQL statement
 * 
 * @param jDest - JString of the destination
 * @param jStr - String to examine
 * @param isNumeric - Is this a numeric string
 *
 * 
**/

void cm_sql_numeric_quote(JString * jDest, JString * jStr, Jxta_boolean isNumeric);
/**
 * Set the delta record option in the CM. Deltas are created for publishing to the SRDI
 * of the connected rendezvous.
 * 
 * @param Jxta_cm instance
 * @param Jxta_boolean recordDelta - TRUE - record FALSE - stop recording
**/

void cm_set_delta(Jxta_cm * self, Jxta_boolean recordDelta);

char **cm_sql_get_primary_keys(Jxta_cm * self, char *folder_name, const char *tableName, JString * jWhere,
                                      JString * jGroup, int row_max);

Jxta_status cm_create_adv_indexes(Jxta_cm * self, char *folder_name, Jxta_vector * jv);

void cm_update_replica_forward_peers(Jxta_cm * cm, Jxta_vector * replica_entries, JString *peer_id_j);
Jxta_status cm_get_replica_entries(Jxta_cm * cm, JString *peer_id_j, Jxta_vector **replicas_v);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_CM__PRIVATE_H__ */

/* vi: set ts=4 sw=4 tw=130 et: */
