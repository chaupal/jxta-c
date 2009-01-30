/*
 * Copyright (c) 2008-2009 Sun Microsystems, Inc.  All rights reserved.
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


#ifndef __Jxta_cm_prepared_priv_H__
#define __Jxta_cm_prepared_priv_H__

#include "jxta_apr.h"
#include <apu.h>
#include <apr_dbd.h>

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _pp_statements Jxta_pp_statements ;

/** number of variables for INSERT statements */

#define INSERT_ELEM_ATTRIBUTE_ITEMS 9
#define INSERT_ADVERTISEMENT_ITEMS 7
#define INSERT_SRDI_REPLICA_ITEMS 13
#define INSERT_SRDI_DELTA_ITEMS 18
#define INSERT_SRDI_INDEX_ITEMS 16

/**
* Update Advertisements
*/
#define UPDATE_ADVERTISEMENTS_ITEMS 6

#define UPDATE_ADVERTISEMENTS_STRING \
                        SQL_UPDATE CM_TBL_ADVERTISEMENTS SQL_SET \
                                CM_COL_Advert SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_TimeOut SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_TimeOutForOthers SQL_EQUAL SQL_VARIABLE \
                        SQL_WHERE \
                                CM_COL_AdvId SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_NameSpace SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE

/**
* UPDATE ElemAttribute
*/
#define UPDATE_ELEM_ATTR_ITEMS 9

#define UPDATE_ELEM_ATTR_STRING \
                        SQL_UPDATE CM_TBL_ELEM_ATTRIBUTES SQL_SET \
                                CM_COL_Value SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA  CM_COL_NumValue SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_NumRange SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_TimeOut SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_TimeOutForOthers SQL_EQUAL SQL_VARIABLE \
                        SQL_WHERE \
                                CM_COL_AdvId SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_NameSpace SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Name SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE
/**
* UPDATE SRDI/Replica timeout/range
*/
#define UPDATE_SRDI_REPLICA_ITEMS 8

#define UPDATE_SRDI_STRING \
                        SQL_UPDATE CM_TBL_SRDI SQL_SET \
                                CM_COL_TimeOut SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_TimeOutForOthers SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_NumRange SQL_EQUAL SQL_VARIABLE \
                        SQL_WHERE \
                                CM_COL_AdvId SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Handler SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Name SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE

#define UPDATE_REPLICA_STRING \
                        SQL_UPDATE CM_TBL_REPLICA SQL_SET \
                                CM_COL_TimeOut SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_TimeOutForOthers SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_NumRange SQL_EQUAL SQL_VARIABLE \
                        SQL_WHERE \
                                CM_COL_AdvId SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Handler SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Name SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE

/**
* UPDATE SRDI/Replica timeout/range/value/num_value
*/
#define UPDATE_SRDI_REPLICA_VALUE_ITEMS 10

#define UPDATE_SRDI_VALUE_STRING \
                        SQL_UPDATE CM_TBL_SRDI SQL_SET \
                                CM_COL_TimeOut SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_TimeOutForOthers SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_NumRange SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_Value SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_NumValue SQL_EQUAL SQL_VARIABLE \
                        SQL_WHERE \
                                CM_COL_AdvId SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Handler SQL_LIKE SQL_VARIABLE \
                                SQL_AND CM_COL_Name SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE

#define UPDATE_REPLICA_VALUE_STRING \
                        SQL_UPDATE CM_TBL_REPLICA SQL_SET \
                                CM_COL_TimeOut SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_TimeOutForOthers SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_NumRange SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_Value SQL_EQUAL SQL_VARIABLE \
                                SQL_COMMA CM_COL_NumValue SQL_EQUAL SQL_VARIABLE \
                        SQL_WHERE \
                                CM_COL_AdvId SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Handler SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Name SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE

/**
* UPDATE SRDI Index entries
*/

#define UPDATE_SRDI_INDEX_ITEMS 4

#define UPDATE_SRDI_INDEX_STRING \
                        SQL_UPDATE CM_TBL_SRDI_INDEX SQL_SET \
                                CM_COL_TimeOut SQL_EQUAL SQL_VARIABLE \
                        SQL_WHERE \
                                CM_COL_SeqNumber SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE


/**
* UPDATE SRDI Index/Replica Peerids
**/

#define UPDATE_SRDI_INDEX_REPLICA_PEERID_ITEMS 5

#define UPDATE_SRDI_INDEX_PEERID_STRING \
                        SQL_UPDATE CM_TBL_SRDI_INDEX SQL_SET \
                                CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                        SQL_WHERE \
                                CM_COL_AdvId SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Name SQL_EQUAL SQL_VARIABLE

#define UPDATE_REPLICA_PEERID_STRING \
                        SQL_UPDATE CM_TBL_REPLICA SQL_SET \
                                CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                        SQL_WHERE \
                                CM_COL_AdvId SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Name SQL_EQUAL SQL_VARIABLE

/**
* DELETE SRDI/Replica entries
*/
#define DELETE_SRDI_REPLICA_ITEMS 5

#define DELETE_SRDI_STRING \
                        SQL_DELETE CM_TBL_SRDI \
                        SQL_WHERE \
                                CM_COL_AdvId SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Handler SQL_LIKE SQL_VARIABLE \
                                SQL_AND CM_COL_Name SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE

#define DELETE_REPLICA_STRING \
                        SQL_DELETE CM_TBL_REPLICA \
                        SQL_WHERE \
                                CM_COL_AdvId SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Handler SQL_LIKE SQL_VARIABLE \
                                SQL_AND CM_COL_Name SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE

/**
* DELETE SRDI index entries
*/
#define DELETE_SRDI_INDEX_ITEMS 3

#define DELETE_SRDI_INDEX_STRING \
                        SQL_DELETE CM_TBL_SRDI_INDEX \
                        SQL_WHERE \
                                CM_COL_SeqNumber SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE

/**
* SELECT SRDI/Replica Entries
*/
#define GET_SRDI_REPLICA_ITEMS

#define GET_SRDI_STRING \
                        SQL_SELECT \
                                CM_COL_Value SQL_COMMA CM_COL_NumRange SQL_COMMA CM_COL_NameSpace \
                        SQL_FROM CM_TBL_SRDI \
                        SQL_WHERE \
                            CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                            SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE \
                            SQL_AND CM_COL_AdvId SQL_EQUAL SQL_VARIABLE \
                            SQL_AND CM_COL_Name SQL_EQUAL SQL_VARIABLE

#define GET_REPLICA_STRING \
                        SQL_SELECT \
                                CM_COL_Value SQL_COMMA CM_COL_NumRange SQL_COMMA CM_COL_NameSpace \
                        SQL_FROM CM_TBL_REPLICA \
                        SQL_WHERE \
                            CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                            SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE \
                            SQL_AND CM_COL_AdvId SQL_EQUAL SQL_VARIABLE \
                            SQL_AND CM_COL_Name SQL_EQUAL SQL_VARIABLE
/**
* SELECT SRDI Index entries
*/
#define GET_SRDI_INDEX_ITEMS 3

#define GET_SRDI_INDEX_STRING \
                        SQL_SELECT \
                                CM_COL_DBAlias SQL_COMMA CM_COL_AdvId SQL_COMMA CM_COL_Name \
                                SQL_COMMA CM_COL_Replica SQL_COMMA CM_COL_CachedLocal \
                                SQL_COMMA CM_COL_Replicate SQL_COMMA CM_COL_Fwd SQL_COMMA CM_COL_Duplicate SQL_COMMA CM_COL_SourcePeerid \
                                SQL_COMMA CM_COL_FwdPeerid SQL_COMMA CM_COL_NameSpace \
                                SQL_FROM CM_TBL_SRDI_INDEX \
                        SQL_WHERE \
                                CM_COL_SeqNumber SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_Peerid SQL_EQUAL SQL_VARIABLE \
                                SQL_AND CM_COL_GroupID SQL_EQUAL SQL_VARIABLE

struct _pp_statements {
    JXTA_OBJECT_HANDLE;

/**
* The INSERT prepared statements are created when the table is created. It creates an INSERT
* SQL statement that takes all columns as variables.
* 
*/
    apr_dbd_prepared_t *insert_elem_attributes;
    JString *insert_elem_attributes_j;

    apr_dbd_prepared_t *insert_advertisements;
    JString *insert_advertisements_j;

    apr_dbd_prepared_t *insert_srdi;
    JString *insert_srdi_j;

    apr_dbd_prepared_t *insert_replica;
    JString *insert_replica_j;

    apr_dbd_prepared_t *insert_srdi_delta;
    JString *insert_srdi_delta_j;

    apr_dbd_prepared_t *insert_srdi_index;
    JString *insert_srdi_index_j;

/**
* The UPDATE prepared statements are created for individual functions. The UPDATE SQL statement
* includes the SET and WHERE clauses and is unique for each prepared statement.
*/
    apr_dbd_prepared_t *update_advertisements;

    apr_dbd_prepared_t *update_elem_attributes;

    apr_dbd_prepared_t *update_srdi;
    apr_dbd_prepared_t *update_srdi_value;

    apr_dbd_prepared_t *update_replica;
    apr_dbd_prepared_t *update_replica_value;
    apr_dbd_prepared_t *update_replica_peerid;

    apr_dbd_prepared_t *update_srdi_index;
    apr_dbd_prepared_t *update_srdi_index_peerid;

/**
* The DELETE prepared statements are created for various functions. The DELETE SQL statement will also
* include a WHERE clause
*/
    apr_dbd_prepared_t *delete_srdi;
    apr_dbd_prepared_t *delete_replica;
    apr_dbd_prepared_t *delete_srdi_index;

/**
* The SELECT prepared statements are created with parameters for the WHERE clause. The columns returned are
* always static and cannot be provided dynamically
*/
    apr_dbd_prepared_t *get_srdi;
    apr_dbd_prepared_t *get_replica;
    apr_dbd_prepared_t *get_srdi_index;
};

#define LOG_PARM_LIST(lps_desc, lps_list, lps_size) \
                    int lps_idx; \
                    JString *lps_str_j; \
                    lps_str_j = jstring_new_0(); \
                    for (lps_idx =0; lps_idx < lps_size; lps_idx++) { \
                        if (jstring_length(lps_str_j) > 0) \
                            jstring_append_2(lps_str_j, ","); \
                        jstring_append_2(lps_str_j, lps_list[lps_idx]); \
                    } \
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "%s %s\n", lps_desc, jstring_get_string(lps_str_j)); \
                    JXTA_OBJECT_RELEASE(lps_str_j);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __Jxta_cm_prepared_priv_H__  */
