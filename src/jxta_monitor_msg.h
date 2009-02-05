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


#ifndef __MONITOR_MSG_H__
#define __MONITOR_MSG_H__

#include "jstring.h"
#include "jxta_advertisement.h"
#include "jxta_vector.h"
#include "jxta_cred.h"
#include "jxta_pa.h"
#include "jxta_peer.h"

typedef struct _jxta_monitor_msg Jxta_monitor_msg;

#include "jxta_monitor_service.h"



#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef enum _monitor_msg_type {
        MON_MONITOR=0,
        MON_REQUEST=1,
        MON_COMMAND =2,
        MON_STATUS = 3
} Jxta_monitor_msg_type;

typedef enum _monitor_msg_cmd {
        CMD_START_MONITOR=0,
        CMD_STOP_MONITOR=1,
        CMD_GET_TYPE=2,
        CMD_STATUS=3
} Jxta_monitor_msg_cmd;

/**
* Create a new monitor message
* @return a monitor message object. NULL if no memory is available
*/
JXTA_DECLARE(Jxta_monitor_msg *) jxta_monitor_msg_new();

/**
 * set the type of message
 * @param myself The monitor message
 * @param type  MON_MONITOR monitor messages
 *              MON_REQUEST request specific types of monitor messages
 *              MON_COMMAND This is a commnand (type specifies command)
 *              MON_STATUS This is a status message
 * @return status - success or failure
*/ 
JXTA_DECLARE(Jxta_status) jxta_monitor_msg_set_type(Jxta_monitor_msg * myself, Jxta_monitor_msg_type type);

/**
 * set the command of message when the type of message is a command
 * @param myself The monitor message
 * @param type  CMD_START_MONITOR - Start the monitor service
 *              CMD_STOP_MONITOR - Stop the monitor service
 *              CMD_GET_TYPE - Get entries of type
 *              CMD_STATUS - Retrieve status of services
 * @return status - success or failure
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_msg_set_command(Jxta_monitor_msg *myself, Jxta_monitor_msg_cmd cmd);

/**
 *  set the group context of this monitor msg 
 * @param myself - The monitor message
 * @param context - Id for the context
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_msg_set_context(Jxta_monitor_msg * myself, const char * context);

/**
 *  get the groupId of this message
 * @param myself The monitor message
 * @param context Pointer for the location to store the context
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_msg_get_context(Jxta_monitor_msg * myself, const char **context);

/**
 * Add monitor entries with a hash of groups that contain a hash of services. Within the service entries are 
 * vectors of entries. This is an optimization the monitor service uses since it maintains an identical 
 * set of hash tables as the message. For adding single entries use jxta_monitor_msg_add_entry.
 * 
 * @param myself - The monitor message
 * @param contexts - This is a hash table that contains entries for each context and within that context are
 * sub_context entries.
 *
**/
JXTA_DECLARE(Jxta_status) jxta_monitor_msg_set_entries(Jxta_monitor_msg * msg, Jxta_hashtable * contexts);

/**
 * add a monitor entry 
 * @param myself The monitor message
 * @param context Context ID
 * @param sub_context Sub context Id
 * @param entry - The monitor entry to add in the message
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_msg_add_entry(Jxta_monitor_msg * myself, const char *context, const char *sub_context, Jxta_monitor_entry * entry);

/**
 * get a vector of the monitor type entries
 * @param myself The monitor message
 * @param type - A filter for type of entries to retrieve from the message
 * @param entries - A location to store a vector of entries from the message
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_msg_get_entries(Jxta_monitor_msg * myself, Jxta_vector ** entries);

/**
 * get all entries from the message
 * 
 * @param myself The monitor message
 * @param types - Location to store vector of entries
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_msg_get(Jxta_monitor_msg *myself, Jxta_vector ** entries);

/**
 * clear all entries of "type" entries 
 * @param myself The monitor message
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_msg_clear(Jxta_monitor_msg * myself, const char * type);

/**
 * add a msg type to retrieve 
 * @param myself The monitor message
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_msg_request_add(Jxta_monitor_msg * myself, const char *type);

/**
 * Helper function to retrieve text describing the "type"
**/
JXTA_DECLARE(const char *) jxta_monitor_msg_type_text(Jxta_monitor_msg * me);

/**
 * Helper function to retrieve text of the "command"
**/
JXTA_DECLARE(const char *) jxta_monitor_msg_cmd_text(Jxta_monitor_msg * me);

JXTA_DECLARE(Jxta_credential*) jxta_monitor_msg_get_credential(Jxta_monitor_msg * me);
JXTA_DECLARE(void) jxta_monitor_msg_set_credential(Jxta_monitor_msg * me, Jxta_credential* credential);

JXTA_DECLARE(Jxta_PA *) jxta_monitor_msg_get_peer_adv(Jxta_monitor_msg * me);
JXTA_DECLARE(void) jxta_monitor_msg_set_peer_adv(Jxta_monitor_msg * me, Jxta_PA *peer_adv);

JXTA_DECLARE(Jxta_status) jxta_monitor_msg_get_xml(Jxta_monitor_msg *, JString ** xml);
JXTA_DECLARE(Jxta_status) jxta_monitor_msg_parse_charbuffer(Jxta_monitor_msg *, const char *, int len);
JXTA_DECLARE(Jxta_status) jxta_monitor_msg_parse_file(Jxta_monitor_msg *, FILE * stream);

JXTA_DECLARE(Jxta_id*) jxta_monitor_msg_get_peer_id(Jxta_monitor_msg * me);
JXTA_DECLARE(void) jxta_monitor_msg_set_peer_id(Jxta_monitor_msg * me, Jxta_id* id);


/**
* Message Element Name we use for monitor messages.
*/
JXTA_DECLARE_DATA const char JXTA_MONITOR_MSG_ELEMENT_NAME[];

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __MONITOR_MSG_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
