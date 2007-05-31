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
 * $Id: jxta_cm.h,v 1.4 2005/02/22 00:32:44 bondolo Exp $
 */
#ifndef JXTA_CM_H__
#define JXTA_CM_H__

#include "jxta_advertisement.h"


#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
/***/ struct _jxta_cm;
typedef struct _jxta_cm Jxta_cm;

/**
 * Create a new Jxta_cm object.
 *
 * @param home_directory The directory where to store the files.
 * @param group_id The ID of the group for which this Jxta_cm works.
 * @return Jxta_cm* (A ptr to) the newly created Jxta_cm object.
 */
Jxta_cm *jxta_cm_new(const char *home_directory, Jxta_id * group_id);

/**
 * Create a new folder.
 * This operation is idem-potent.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @param keys secondary keys; a null terminated array of char*.
 * @return Jxta_status JXTA_SUCCESS if the folder was created ok.
 */
Jxta_status jxta_cm_create_folder(Jxta_cm * self, char *folder_name, const char *keys[]);

/**
 * Create secondary indexes for the specified keys.
 * 
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param adv_descriptor - This is the name space and advertisement type
 *                         "namespace:advertisement type"
 * @param keys the element/attribute pairs that define the indexes of the advetisement
 * @return Jxta_status JXTA_SUCCESS if the schema was created ok.
*/
Jxta_status jxta_cm_create_adv_indexes(Jxta_cm * self, char *folder_name, Jxta_vector * keys);

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
Jxta_status jxta_cm_remove_advertisement(Jxta_cm * cm, char *folder_name, char *primary_key);

/**
 * Return a pointer to each primary key of all advertisements in the given
 * folder.
 *
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @param folder_name the name of the folder
 * @return char** A null terminated array of char*. If the folder does
 * not exist, NULL is returned.
 */
char **jxta_cm_get_primary_keys(Jxta_cm * cm, char *folder_name);

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
Jxta_status
jxta_cm_save(Jxta_cm * cm, char *folder_name, char *primary_key,
             Jxta_advertisement * adv, Jxta_expiration_time timeOutForMe, Jxta_expiration_time timeOutForOthers);

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
char **jxta_cm_search(Jxta_cm * cm, char *folder_name, const char *attribute, const char *value, int n_adv);


char **jxta_cm_query(Jxta_cm * cm, char *folder_name, const char *query, int n_adv);



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
Jxta_status jxta_cm_restore_bytes(Jxta_cm * cm, char *folder_name, char *primary_key, JString ** bytes);

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
Jxta_status jxta_cm_get_expiration_time(Jxta_cm * cm, char *folder_name, char *primary_key, Jxta_expiration_time * time);

/**
 * delete files which life-time has ended.
 * @param Jxta_cm (A ptr to) the cm object to apply the operation to
 * @return Jxta_status JXTA_SUCCESS. (errors ?).
 */
Jxta_status jxta_cm_remove_expired_records(Jxta_cm * cm);

/**
 * Generate a hash key for the given buffer.
 * @param str The address of the data.
 * @paran len The length of the data.
 * @return unsigned long The resulting hash key.
 */
unsigned long jxta_cm_hash(const char *str, size_t len);


/* temporary workaround for closing the database.
 * The cm will be freed automatically when group stoping realy works
 */
void jxta_cm_close(Jxta_cm * cm);

/*
 * get the SRDI index entry for the provided folder
 *
 * @param Jxta_cm instance
 * @param folder name
 *
 * @return a vector od SRDIElementEntry
 */
Jxta_vector *jxta_cm_get_srdi_entries(Jxta_cm * self, JString * folder_name);

/*
 * get the new SRDI index entry for the provided folder since
 * we last pushed them
 *
 * @param Jxta_cm instance
 * @param folder name
 *
 * @return a vector od SRDIElementEntry
 */
Jxta_vector *jxta_cm_get_srdi_delta_entries(Jxta_cm * self, JString * folder_name);

#ifdef __cplusplus
}
#endif



#endif /* JXTA_CM_H__ */
