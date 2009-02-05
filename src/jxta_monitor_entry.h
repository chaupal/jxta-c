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


#ifndef __MONITOR_ENTRY_H__
#define __MONITOR_ENTRY_H__

#include "jstring.h"
#include "jxta_advertisement.h"
#include "jxta_vector.h"
#include "jxta_cred.h"
#include "jxta_pa.h"
#include "jxta_peer.h"

typedef struct _jxta_monitor_entry Jxta_monitor_entry;
typedef Jxta_advertisement Jxta_monitor_event;

#include "jxta_monitor_service.h"



#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
* Create a new monitor entry
* @return a monitor entry object. NULL if no memory is available
*/
JXTA_DECLARE(Jxta_monitor_entry *) jxta_monitor_entry_new();

/**
* Create a new monitor entry with groupid and serviceid
* @param context Group id to set
* @param sub_context Service ID to set
* @param adv - advertisement for the entry
*
* @return a monitor entry object. NULL if no memory is available
*/
JXTA_DECLARE(Jxta_monitor_entry *) jxta_monitor_entry_new_1(Jxta_id *context, Jxta_id *sub_context, Jxta_advertisement * adv);

/**
 *  set the group context of this monitor msg 
 * @param myself - The monitor entry
 * @param groupid - Id for the context
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_entry_set_context(Jxta_monitor_entry * myself, const char * context);

/**
 *  get the groupId of this entry
 * @param myself The monitor entry
 * @param groupid Pointer for the location to store the context
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_entry_get_context(Jxta_monitor_entry * myself, char **context);

/**
 *  get the service ID of this monitor entry 
 * @param myself - The monitor entry
 * @param service - pointer to the location for Id for the sub_context
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_entry_get_sub_context(Jxta_monitor_entry * myself, char **sub_context);

/**
 *  set the service ID of this entry
 * @param myself The monitor entry
 * @param sub_context the service ID
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_entry_set_sub_context(Jxta_monitor_entry * myself, const char * sub_context);

/**
 *  get the type of this monitor entry 
 * @param myself - The monitor entry
 * @param type - pointer of the location to store the type
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_entry_get_type(Jxta_monitor_entry * myself, char **type);

/**
 *  set the type of this entry
 * @param myself The monitor entry
 * @param type the type
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_entry_set_type(Jxta_monitor_entry * myself, const char * type);

/**
 * set the monitor entry advertisement. 
 * @param myself The monitor entry
 * @param entry - The monitor advertisement to add in the entry
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_entry_set_entry(Jxta_monitor_entry * myself, Jxta_advertisement * adv);
JXTA_DECLARE(Jxta_status) jxta_monitor_entry_get_entry(Jxta_monitor_entry * myself, Jxta_advertisement **adv);

JXTA_DECLARE(Jxta_status) jxta_monitor_entry_get_xml(Jxta_monitor_entry *, JString ** xml);
JXTA_DECLARE(Jxta_status) jxta_monitor_entry_parse_charbuffer(Jxta_monitor_entry *, const char *, int len);
JXTA_DECLARE(Jxta_status) jxta_monitor_entry_parse_file(Jxta_monitor_entry *, FILE * stream);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __MONITOR_ENTRY_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
