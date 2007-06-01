/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights
 * reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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
 * $Id: jxta_transport_welcome_message.c,v 1.6 2006/07/14 21:35:54 bondolo Exp $
 */

static const char *__log_cat = "WELCOME_MSG";

#include <stdlib.h>

#include "jxta_log.h"
#include "jxta_errno.h"

#include "jxta_transport_welcome_message.h"

static const char* GREETING = "JXTAHELLO";
static const char* SPACE = " ";
static const char* CURRENTVERSION = "3.0";
static const char* CRLF = "\r\n";
static const Jxta_welcome_message_version CURRENT_VERSION = JXTA_WELCOME_MESSAGE_VERSION_3_0;

struct _welcome_message {
    JXTA_OBJECT_HANDLE;

    Jxta_endpoint_address *dest_addr;
    Jxta_endpoint_address *public_addr;
    JString *peer_id;
    Jxta_boolean noprop;
    int use_message_version;   
    
    Jxta_welcome_message_version version;
    
    JString *welcome_str;
};

typedef struct _welcome_message _welcome_message_mutable;

static Jxta_status welcome_message_parse(_welcome_message_mutable * me);
static _welcome_message_mutable * welcome_message_new(void);
static void welcome_message_free(Jxta_object * me);

static _welcome_message_mutable * welcome_message_new(void)
{
    _welcome_message_mutable *self = (_welcome_message_mutable *) calloc(1, sizeof(_welcome_message_mutable));
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "welcome message calloc error\n");
        return NULL;
    }

    JXTA_OBJECT_INIT(self, welcome_message_free, NULL);

    return self;
}

JXTA_DECLARE(Jxta_welcome_message *) welcome_message_new_1(Jxta_endpoint_address * dest_addr, Jxta_endpoint_address * public_addr,
                                             JString *peerid, Jxta_boolean noprop, int message_version )
{
    _welcome_message_mutable *myself = welcome_message_new();
    
    JXTA_OBJECT_CHECK_VALID(dest_addr);
    JXTA_OBJECT_CHECK_VALID(public_addr);
    JXTA_OBJECT_CHECK_VALID(peerid);
    
    if( (message_version < 0) || (message_version > 255) ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Illegal message version.\n");
        JXTA_OBJECT_RELEASE(myself);
        return NULL;
    }
    
    if (myself != NULL) {
        char *caddress = NULL;

        myself->dest_addr = JXTA_OBJECT_SHARE(dest_addr);
        myself->public_addr = JXTA_OBJECT_SHARE(public_addr);
        myself->peer_id = JXTA_OBJECT_SHARE(peerid);
        myself->noprop = noprop;
        myself->use_message_version = message_version;
        myself->version = JXTA_WELCOME_MESSAGE_VERSION_3_0;

        myself->welcome_str = jstring_new_0();
        if (myself->welcome_str == NULL) {
            JXTA_OBJECT_RELEASE(myself);
            return NULL;
        }
        jstring_append_2(myself->welcome_str, GREETING);
        jstring_append_2(myself->welcome_str, SPACE);
        jstring_append_2(myself->welcome_str, (caddress = jxta_endpoint_address_to_string(myself->dest_addr)));
        free(caddress);
        jstring_append_2(myself->welcome_str, "");
        jstring_append_2(myself->welcome_str, SPACE);
        jstring_append_2(myself->welcome_str, (caddress = jxta_endpoint_address_to_string(myself->public_addr)));
        free(caddress);
        jstring_append_2(myself->welcome_str, SPACE);
        jstring_append_1(myself->welcome_str, peerid);
        if( JXTA_WELCOME_MESSAGE_VERSION_1_1 == CURRENT_VERSION ) {
            jstring_append_2(myself->welcome_str, SPACE);
            jstring_append_2(myself->welcome_str, myself->noprop ? "0" : "1");
        } if( JXTA_WELCOME_MESSAGE_VERSION_3_0 == CURRENT_VERSION ) {
            char version[4];
                        
            jstring_append_2(myself->welcome_str, SPACE);
            jstring_append_2(myself->welcome_str, myself->noprop ? "0" : "1");
            jstring_append_2(myself->welcome_str, SPACE);
            sprintf( version, "%d", myself->use_message_version );
            jstring_append_2(myself->welcome_str, version );
        }
        jstring_append_2(myself->welcome_str, SPACE);
        jstring_append_2(myself->welcome_str, CURRENTVERSION);
        jstring_append_2(myself->welcome_str, "\r\n");
    }

    return myself;
}

JXTA_DECLARE( Jxta_welcome_message *) welcome_message_new_2(JString * welcome_str)
{
    _welcome_message_mutable *myself;
    const char *line; 
    const char *current;
    const char *next;
    const char *last;
    const char *end;
    JString * temp;

    JXTA_OBJECT_CHECK_VALID(welcome_str);
    line = jstring_get_string(welcome_str);
    current = line;

    myself = welcome_message_new();
    if (myself == NULL) {
        return NULL;
    }

    /* <GREETING> */
    next = strchr( current, ' ' );
    if( (NULL == next) || (0 == (next - current)) || (0 != strncmp( line, GREETING, (next - current))) ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Greeting missing from welcome message : %s\n", line );
        goto Error_Exit;
    }
    current = next + 1;
    
    /* <WELCOMEDEST> */
    next = strchr( current, ' ' );
    if( (NULL == next) || (0 == (next - current)) ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Destination address missing from welcome message : %s\n", line );
        goto Error_Exit;
    }
    temp = jstring_new_1( (next - current) );
    jstring_append_0( temp, current, (next - current) );
    myself->dest_addr = jxta_endpoint_address_new_1(temp);
    JXTA_OBJECT_CHECK_VALID(myself->dest_addr);
    JXTA_OBJECT_RELEASE( temp );
    current = next + 1;

    /* <WELCOMESRC> */
    next = strchr( current, ' ' );
    if( (NULL == next) || (0 == (next - current)) ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Source address missing from welcome message : %s\n", line );
        goto Error_Exit;
    }
    temp = jstring_new_1( (next - current) );
    jstring_append_0( temp, current, (next - current) );
    myself->public_addr = jxta_endpoint_address_new_1(temp);
    JXTA_OBJECT_CHECK_VALID(myself->public_addr);
    JXTA_OBJECT_RELEASE( temp );
    current = next + 1;

    /* <WELCOMEPEER> */
    next = strchr( current, ' ' );
    if( (NULL == next) || (0 == (next - current)) ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Peer ID missing from welcome message : %s\n", line );
        goto Error_Exit;
    }
    myself->peer_id = jstring_new_1( (next - current) );
    jstring_append_0( myself->peer_id, current, (next - current) );
    current = next + 1;
    
    /* Handle <VERSION> */
    end = &current[strlen(current)];
    if( end == current ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Truncated welcome message : %s\n", line );
    }
    if( ('\n' != *(end - 1)) || ('\r' != *(end - 2)) ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Truncated welcome message : %s\n", line );
    }
    end -= 2;
    last = strrchr( current, ' ' );
    if( (NULL == last) || ( '0' == *(++last)) ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Missing version from welcome message : %s\n", line );
        goto Error_Exit;        
    }
        
    /* Version "1.1" */
    if( 0 == strncmp("1.1", last, (end - last)) ) {
        /* <NOPROP> */
        next = strchr( current, ' ' );
        if( (NULL == next) || (1 != (next - current)) ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Propagate flag missing from welcome message : %s\n", line );
            goto Error_Exit;
        }
        switch( *current ) {
            case '0' : 
                myself->noprop = FALSE;
                break;

            case '1' :
                myself->noprop = TRUE;
                break;

            default :
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid propagate flag missing from welcome message : %s\n", line );
                goto Common_Exit;            
        }
        current = next + 1;
        
        if( current != last ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Extra tokens in 1.1 welcome message : %s\n", line );
            goto Common_Exit;            
        }
        
        myself->use_message_version = 0;

        myself->version = JXTA_WELCOME_MESSAGE_VERSION_1_1;
    } else if( 0 == strncmp("3.0", last, (end - last)) ) {
        /* <NOPROP> */
        next = strchr( current, ' ' );
        if( (NULL == next) || (1 != (next - current)) ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Propagate flag missing from welcome message : %s\n", line );
            goto Error_Exit;
        }
        switch( *current ) {
            case '0' : 
                myself->noprop = FALSE;
                break;

            case '1' :
                myself->noprop = TRUE;
                break;

            default :
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid propagate flag missing from welcome message : %s\n", line );
                goto Common_Exit;            
        }
        current = next + 1;
                
        /* <MSGVERS> */
        next = strchr( current, ' ' );
        if( (NULL == next) || (0 == (next - current)) || ((next - current) > 3) ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Message version missing from welcome message : %s\n", line );
            goto Error_Exit;
        } else {
            char version[4];
            
            memmove( version, current, (next - current) );
            version[(next - current)] = '\0';
            myself->use_message_version = atoi(version);
        }
        current = next + 1;

        if( current != last ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Extra tokens in 3.0 welcome message : %s\n", line );
            goto Common_Exit;            
        }
    
        myself->version = JXTA_WELCOME_MESSAGE_VERSION_3_0;
    } else {
        myself->noprop = TRUE;

        myself->use_message_version = 0;

        myself->version = JXTA_WELCOME_MESSAGE_VERSION_UNKNOWN;
    }
   
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Welcome message parsed : %s\n", jstring_get_string(myself->peer_id) );
    
    goto Common_Exit;

Error_Exit :
        JXTA_OBJECT_RELEASE(myself);
        myself = NULL;

Common_Exit :

    return myself;
}

static void welcome_message_free(Jxta_object * me)
{
    _welcome_message_mutable *myself = (_welcome_message_mutable *) me;

    if (myself->dest_addr) {
        JXTA_OBJECT_RELEASE(myself->dest_addr);
        myself->dest_addr = NULL;
        }

    if (myself->public_addr) {
        JXTA_OBJECT_RELEASE(myself->public_addr);
        myself->public_addr = NULL;
        }

    if (myself->peer_id) {
        JXTA_OBJECT_RELEASE(myself->peer_id);
        myself->peer_id = NULL;
        }

    if (myself->welcome_str) {
        JXTA_OBJECT_RELEASE(myself->welcome_str);
        myself->welcome_str = NULL;
        }

    free(myself);
}

JXTA_DECLARE(Jxta_endpoint_address *) welcome_message_get_publicaddr(Jxta_welcome_message * me)
{
    _welcome_message_mutable * myself = (_welcome_message_mutable *) me;

    JXTA_OBJECT_CHECK_VALID(myself);
    
    return JXTA_OBJECT_SHARE(myself->public_addr);
}

JXTA_DECLARE(Jxta_endpoint_address *) welcome_message_get_destaddr(Jxta_welcome_message * me)
{
    _welcome_message_mutable * myself = (_welcome_message_mutable *) me;

    JXTA_OBJECT_CHECK_VALID(myself);
    
    return JXTA_OBJECT_SHARE(myself->dest_addr);
}

JXTA_DECLARE(JString *) welcome_message_get_peerid(Jxta_welcome_message * me)
{
    _welcome_message_mutable * myself = (_welcome_message_mutable *) me;

    JXTA_OBJECT_CHECK_VALID(myself);
    
    return JXTA_OBJECT_SHARE(myself->peer_id);
}

JXTA_DECLARE(Jxta_boolean) welcome_message_get_no_propagate(Jxta_welcome_message * me)
{
    _welcome_message_mutable * myself = (_welcome_message_mutable *) me;

    JXTA_OBJECT_CHECK_VALID(myself);
    
    return myself->noprop;
}

JXTA_DECLARE(int) welcome_message_get_message_version(Jxta_welcome_message * me)
{
    _welcome_message_mutable * myself = (_welcome_message_mutable *) me;

    JXTA_OBJECT_CHECK_VALID(myself);
    
    return myself->use_message_version;
}

JXTA_DECLARE(JString *) welcome_message_get_welcome(Jxta_welcome_message * me)
{
    _welcome_message_mutable * myself = (_welcome_message_mutable *) me;

    JXTA_OBJECT_CHECK_VALID(myself);
    
    return JXTA_OBJECT_SHARE(myself->welcome_str);
}

/* vim: set ts=4 sw=4 tw=130 et: */
