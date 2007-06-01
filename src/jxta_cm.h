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
 * $Id: jxta_cm.h,v 1.16 2005/11/23 15:50:32 slowhog Exp $
 */
#ifndef JXTA_CM_H__
#define JXTA_CM_H__

#include "jxta_advertisement.h"
#include "jxta_query.h"
#include "jxta_srdi.h"


#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
/***/ struct _jxta_cm;
typedef struct _jxta_cm Jxta_cm;

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


#define CM_COL_SRC " src"
#define CM_COL_JOIN " jn"

/* table colum values */

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
#define CM_COL_TimeOutForOthers "TimeOutOthers"
#define CM_COL_Advert "Advert"

/**
 * Create a new Jxta_cm object.
 *
 * @param home_directory The directory where to store the files.
 * @param group_id The ID of the group for which this Jxta_cm works.
 * @return Jxta_cm* (A ptr to) the newly created Jxta_cm object.
 */
JXTA_DECLARE(Jxta_cm *) jxta_cm_new(const char *home_directory, Jxta_id * group_id, Jxta_PID * localPeerId);


/**
 * Create a new folder.
 * This operation is idem-potent.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param keys secondary keys; a null terminated array of char*.
 * @return Jxta_status JXTA_SUCCESS if the folder was created ok.
 */
JXTA_DECLARE(Jxta_status) jxta_cm_create_folder(Jxta_cm * self, char *folder_name, const char *keys[]);

/**
 * Create secondary indexes for the specified keys.
 * 
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param adv_descriptor - This is the name space and advertisement type
 *                         "namespace:advertisement type"
 * @param keys the element/attribute pairs that define the indexes of the advetisement
 * @return Jxta_status JXTA_SUCCESS if the schema was created ok.
*/
JXTA_DECLARE(Jxta_status) jxta_cm_create_adv_indexes(Jxta_cm * self, char *folder_name, Jxta_vector * keys);

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
JXTA_DECLARE(Jxta_status) jxta_cm_remove_advertisement(Jxta_cm * cm, const char *folder_name, char *primary_key);

/**
 * Return a pointer to each primary key of all advertisements in the given
 * folder.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @return char** A null terminated array of char*. If the folder does
 * not exist, NULL is returned.
 */
JXTA_DECLARE(char **) jxta_cm_get_primary_keys(Jxta_cm * cm, char *folder_name);

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
 */
JXTA_DECLARE(Jxta_status)
    jxta_cm_save(Jxta_cm * cm, const char *folder_name, char *primary_key,
             Jxta_advertisement * adv, Jxta_expiration_time timeOutForMe, Jxta_expiration_time timeOutForOthers);
/**
 * Save the SRDI entry in the given folder.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param primary_key the primary key with which to associate that advertisement.
 * @param SRDI element - Entry should contain the advid, attr, value and timeout
 */
JXTA_DECLARE(Jxta_status)
    jxta_cm_save_srdi(Jxta_cm * self, JString * handler, JString * peerid, JString * primaryKey, Jxta_SRDIEntryElement * entry);
/**
 * Save the Replica entry in the given folder 
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param primary_key the primary key with which to associate that advertisement.
 * @param SRDI element - Entry should contain the advid, attr, value and timeout
 */

JXTA_DECLARE(Jxta_status)
    jxta_cm_save_replica(Jxta_cm * self, JString * handler, JString * peerid, JString * primaryKey, Jxta_SRDIEntryElement * entry);

/**
 * Search for advertisements with specific (attribute,value) pairs
 * What is returned is a null terminated list of the primary keys of
 * items that match.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param attribute the selector attribute
 * @param value the selector value
 * @param n_adv The maximum number of advs to retrieve.
 * @return A null terminated list of char* each pointing at the primary key
 * of one matching advertisement.
 */
JXTA_DECLARE(char **) jxta_cm_search(Jxta_cm * cm, char *folder_name, const char *attribute, const char *value, int n_adv);

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
JXTA_DECLARE(char **) jxta_cm_query(Jxta_cm * cm, char *folder_name, const char *query, int n_adv);

/**
 * Search for advertisements with a raw SQL query WHERE string.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param where - A SQL string that is concatenated to a WHERE clause.
 * @return A null terminated list of Jxta_advertisement* each pointing at an instance 
 * a matching advertisement
 */
JXTA_DECLARE(Jxta_advertisement **) jxta_cm_sql_query(Jxta_cm * self, const char *folder_name, JString * where);

/**
 * Search for entries in the SRDI with a raw SQL query WHERE string.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param where - A WHERE clause SQL string.
 * @return A null terminated list of Jxta_advertisement* each pointing
 * a composite advertiswement of the entries ordered by the advid.
 */
JXTA_DECLARE(Jxta_object **) jxta_cm_sql_query_srdi(Jxta_cm * self, const char *folder_name, JString * where);

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
JXTA_DECLARE(Jxta_vector *) jxta_cm_query_replica(Jxta_cm * self, JString * nameSpace, Jxta_vector * queries);

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
JXTA_DECLARE(Jxta_status) jxta_cm_restore_bytes(Jxta_cm * cm, char *folder_name, char *primary_key, JString ** bytes);

/**
 * Get the expiration time of an advertisement.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param primary_key the primary key with which to associate that advertisement.
 * @param time The expiration time of that advertisement.
 * @return Jxta_status JXTA_SUCCESS if the advertisement exists and has an expiration time. An error
 * code otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_cm_get_expiration_time(Jxta_cm * cm, char *folder_name, char *primary_key,
                                                      Jxta_expiration_time * time);
/**
 * Get the timeoutforothers expiration time of an advertisement.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param primary_key the primary key with which to associate that advertisement.
 * @param time The timeoutforothers expiration time of that advertisement.
 * @return Jxta_status JXTA_SUCCESS if the advertisement exists and has an expiration time. An error
 * code otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_cm_get_others_time(Jxta_cm * self, char *folder_name, char *primary_key,
                                                  Jxta_expiration_time * time);

/**
 * delete files which life-time has ended.
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @return Jxta_status JXTA_SUCCESS. (errors ?).
 */
JXTA_DECLARE(Jxta_status) jxta_cm_remove_expired_records(Jxta_cm * cm);

/**
 * Generate a hash key for the given buffer.
 * @param str The address of the data.
 * @paran len The length of the data.
 * @return unsigned long The resulting hash key.
 */
JXTA_DECLARE(unsigned long) jxta_cm_hash(const char *str, size_t len);


/* temporary workaround for closing the database.
 * The cm will be freed automatically when group stoping realy works
 */
JXTA_DECLARE(void) jxta_cm_close(Jxta_cm * cm);

/*
 * get the SRDI index entry for the provided folder
 *
 * @param Jxta_cm instance
 * @param folder name
 *
 * @return a vector od SRDIElementEntry
 */
JXTA_DECLARE(Jxta_vector *) jxta_cm_get_srdi_entries(Jxta_cm * self, JString * folder_name);

/**
 * Remove the SRDI entries for the specified peer
 * 
 * @param Jxta_cm instance
 * @param const char * id - Peerid to remove
 * 
 *
**/
JXTA_DECLARE(void) jxta_cm_remove_srdi_entries(Jxta_cm * self, JString * peerid);

/*
 * get the new SRDI index entry for the provided folder since
 * we last pushed them
 *
 * @param Jxta_cm instance
 * @param folder name
 *
 * @return a vector od SRDIElementEntry
 */
JXTA_DECLARE(Jxta_vector *) jxta_cm_get_srdi_delta_entries(Jxta_cm * self, JString * folder_name);
/**
 * Remove the SRDI entries for the specified peer
 * 
 * @param Jxta_cm instance
 * @param const char * id - Peerid to remove
 * 
 *
**/
JXTA_DECLARE(void) jxta_cm_remove_srdi_entries(Jxta_cm * self, JString * peerid);
/**
 * Function to create the proper quotes for a string in a SQL statement
 * 
 * @param jDest - JString of the destination
 * @param jStr - String to examine
 * @param isNumeric - Is this a numeric string
 *
 * 
**/
JXTA_DECLARE(void) jxta_sql_numeric_quote(JString * jDest, JString * jStr, Jxta_boolean isNumeric);
/**
 * Set the delta record option in the CM. Deltas are created for publishing to the SRDI
 * of the connected rendezvous.
 * 
 * @param Jxta_cm instance
 * @param Jxta_boolean recordDelta - TRUE - record FALSE - stop recording
**/
JXTA_DECLARE(void) jxta_cm_set_delta(Jxta_cm * self, Jxta_boolean recordDelta);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_CM_H__ */

/* vi: set ts=4 sw=4 tw=130 et: */
