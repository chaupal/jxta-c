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


#ifndef __Jxta_SRDIMessage_H__
#define __Jxta_SRDIMessage_H__

#include "jxta_object.h"
#include "jxta_id.h"
#include "jstring.h"
#include "jxta_vector.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

#ifdef WIN32
#define JXTA_SEQUENCE_NUMBER_FMT "%I64u"
typedef unsigned __int64 Jxta_sequence_number;
#else
#define JXTA_SEQUENCE_NUMBER_FMT "%llu"
typedef unsigned long long Jxta_sequence_number;

#endif
typedef struct _Jxta_SRDIMessage Jxta_SRDIMessage;
typedef struct _jxta_EntryElement Jxta_SRDIEntryElement;

/* Expiration is an attribute that will have to be handled for
 * each entry.
 */

struct _jxta_EntryElement {
    JXTA_OBJECT_HANDLE;
    Jxta_expiration_time expiration;
    char *db_alias;
    JString *key;
    JString *value;
    JString *nameSpace;
    JString *advId;
    JString *range;
    JString *sn_cs_values;
    Jxta_boolean expanded;
    Jxta_sequence_number seqNumber;
    Jxta_boolean resend;
    Jxta_expiration_time timeout;
    Jxta_expiration_time next_update_time;
    int delta_window;
    Jxta_boolean cache_this;
    Jxta_boolean replicate;
    Jxta_boolean duplicate;
    Jxta_boolean dup_target;
    Jxta_boolean dup_fwd;
    Jxta_boolean fwd;
    Jxta_boolean fwd_this;
    JString *fwd_peerid;
    JString *rep_peerid;
    JString *dup_peerid;
    Jxta_vector *radius_peers;
};

/**
 * Allocate a new srdi message.
 *
 * @param void takes no arguments.
 *
 * @return Pointer to srdi message.
 */
JXTA_DECLARE(Jxta_SRDIMessage *) jxta_srdi_message_new(void);

JXTA_DECLARE(Jxta_SRDIMessage *) jxta_srdi_message_new_1(int TTL, Jxta_id * peerid, char *primarykey, Jxta_vector * entries);

JXTA_DECLARE(Jxta_SRDIMessage *) jxta_srdi_message_new_2(int ttl, Jxta_id * peerid, Jxta_id * src_peerid, char *primarykey, Jxta_vector * entries);

JXTA_DECLARE(Jxta_SRDIMessage *) jxta_srdi_message_new_3(int ttl, Jxta_id * peerid, Jxta_id * src_peerid, char *primarykey, Jxta_vector * entries);

/**
 * Constructs a representation of a srdi message in
 * xml format.
 *
 * @param Jxta_SRDIMessage * pointer to srdi message
 * @param JString ** address of pointer to JString that 
 *        accumulates xml representation of srdi message.
 *
 * @return Jxta_status 
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_message_get_xml(Jxta_SRDIMessage *, JString **);


/**
 * Wrapper for jxta_advertisement_parse_charbuffer,
 * when call completes, the xml representation of a 
 * buffer is loaded into a srdi message structure.
 *
 * @param Jxta_SRDIMessage * pointer to the entry
 *        advertisement to contain the data in the xml.
 * @param const char * buffer containing characters 
 *        with xml syntax.
 * @param int len length of character buffer.
 *
 * @return Jxta_status JXTA_SUCCESS if message was parsed and valid.
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_message_parse_charbuffer(Jxta_SRDIMessage *, const char *, int len);



/**
 * Wrapper for jxta_advertisement_parse_file,
 * when call completes, the xml representation of a 
 * data is loaded into a srdi message structure.
 *
 * @param Jxta_SRDIMessage * srdi message
 *        to contain the data in the xml.
 * @param FILE * an input stream.
 *
 * @return Jxta_status JXTA_SUCCESS if message was parsed and valid.
 */

JXTA_DECLARE(Jxta_status) jxta_srdi_message_parse_file(Jxta_SRDIMessage *, FILE * stream);

/**
 * Gets the ttl of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message 
 *
 * @return short set to value of the type.
 */
JXTA_DECLARE(int) jxta_srdi_message_get_ttl(Jxta_SRDIMessage *);

/**
 *
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 * @param int ttl of the message
 */
JXTA_DECLARE(void) jxta_srdi_message_set_ttl(Jxta_SRDIMessage *, int);

/**
 * Decrements the ttl of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 *
 * @return Jxta_status JXTA_SUCCESS on success.
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_message_decrement_ttl(Jxta_SRDIMessage *);

/**
 * Gets the peer advertisement of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 *
 * @return char * containing the peer advertisement.
 */

/**
 * Gets the Attr of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 *
 * @return char * containing the primary key.
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_message_get_primaryKey(Jxta_SRDIMessage *, JString **);


/**
 * Sets the Attr of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 * @param char * containing the primary key.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_message_set_primaryKey(Jxta_SRDIMessage *, JString *);

/**
 * Gets the peerid of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 *
 * @return char * containing the peerID.
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_message_get_peerID(Jxta_SRDIMessage *, Jxta_id **);

/**
 * Sets the peerid of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 * @param Jxta_id * the peer id.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_message_set_peerID(Jxta_SRDIMessage *, Jxta_id *);

/**
 * Gets the source peerid of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 * @param Jxta_id ** a location to store the peerid
 *
 * @return Jxta_status 
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_message_get_SrcPID(Jxta_SRDIMessage * ad, Jxta_id ** peerid);

/**
 * Sets the source peerid of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 * @param Jxta_id * the peer id.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_message_set_SrcPID(Jxta_SRDIMessage * ad, Jxta_id * peerid);

JXTA_DECLARE(Jxta_boolean) jxta_srdi_message_delta_supported(Jxta_SRDIMessage * ad);

JXTA_DECLARE(void) jxta_srdi_message_set_support_delta(Jxta_SRDIMessage * ad, Jxta_boolean support);

/**
 * Gets the Entries of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 * @param entries pointer to vector containing the entries
 *
 * @return Jxta_status 
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_message_get_entries(Jxta_SRDIMessage * ad, Jxta_vector ** entries);

/**
 * Sets the entries vector of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 * @param Jxta_vector * containing the entries.
 *
 * @return void
 */
JXTA_DECLARE(void) jxta_srdi_message_set_entries(Jxta_SRDIMessage *, Jxta_vector * entries);

/**
 * Gets the Resend Entries of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 * @param entries pointer to vector containing the entries
 *
 * @return Jxta_status 
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_message_get_resend_entries(Jxta_SRDIMessage * ad, Jxta_vector ** entries);

/**
 * Sets the resend entries vector of the srdi message.
 *
 * @param Jxta_SRDIMessage * a pointer to the srdi message
 * @param Jxta_vector * containing the entries.
 *
 * @return void
 */
JXTA_DECLARE(void) jxta_srdi_message_set_resend_entries(Jxta_SRDIMessage * ad, Jxta_vector * entries);

/**
 * Creates a new SRDI entry element
 * @return Jxta_SRDIEntryElement 
 */
JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_new_element(void);

JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_element_clone(Jxta_SRDIEntryElement * entry);

/**
 * Creates a new  entry element
 * @param JString secondary key
 * @param JString value
 * @param long the expiration time associated with the entry
 * @return Jxta_SRDIEntryElement 
 */
JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_new_element_1(JString * key, JString * value, JString * nameSpace,
                                                              Jxta_expiration_time expiration);
/**
 * Creates a new  entry element
 * @param JString secondary key
 * @param JString value
 * @param long the expiration time associated with the entry
 * @return Jxta_SRDIEntryElement 
 */
JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_new_element_2(JString * key, JString * value, JString * nameSpace,
                                                              JString * advId, JString * range, Jxta_expiration_time expiration);

JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_new_element_3(JString * key, JString * value, JString * nameSpace,
                                                              JString * advId, JString * jrange, Jxta_expiration_time expiration,
                                                              Jxta_sequence_number seqNumber);

JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_new_element_4(JString * key, JString * value, JString * nameSpace,
                                                              JString * advId, JString * jrange, Jxta_expiration_time expiration,
                                                              Jxta_sequence_number seqNumber, Jxta_boolean replication);

JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_new_element_resend(Jxta_sequence_number seqNumber);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __Jxta_SRDIMessage_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
