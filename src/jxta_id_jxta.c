/*
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_id_jxta.c,v 1.16 2005/01/10 17:19:22 brent Exp $
 */


#include <stddef.h>
#include <sys/types.h>
#include <string.h>

#include "jxta.h"
#include "jxta_id.h"
#include "jxta_objecthashtable.h"

#include "jxta_id_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
}
#endif

#ifdef WIN32
#pragma warning( once : 4100 )
#endif

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_id * translateToWellKnown( Jxta_id* jid );
static Jxta_status newPeergroupid1( Jxta_id** pg );
static Jxta_status newPeergroupid2( Jxta_id** pg, unsigned char const * seed, size_t len );
static Jxta_status newPeerid1( Jxta_id** peer, Jxta_id* pg );
static Jxta_status newPeerid2( Jxta_id** peer, Jxta_id* pg, unsigned char const * seed, size_t len );
static Jxta_status newCodatid1( Jxta_id** codat, Jxta_id* pg );
static Jxta_status newCodatid2( Jxta_id** codat, Jxta_id* pg, ReadFunc read_func, void* stream );
static Jxta_status newPipeid1( Jxta_id** pipe, Jxta_id* pg );
static Jxta_status newPipeid2( Jxta_id** pipe, Jxta_id* pg, unsigned char const * seed, size_t len );
static Jxta_status newModuleclassid1( Jxta_id** mcid );
static Jxta_status newModuleclassid2( Jxta_id** mcid, Jxta_id* base );
static Jxta_status newModulespecid( Jxta_id** msid, Jxta_id* mcid );
static Jxta_status newFromString( Jxta_id** id, JString* jid );
static Jxta_status getUniqueportion( Jxta_id * jid, JString** uniq );
static void doDelete( Jxta_object * jid );
static Jxta_boolean equals( Jxta_id * jid1, Jxta_id * jid2 );
static unsigned int hashcode (Jxta_id* id);

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
extern JXTAIDFormat jxta_format;

_jxta_id_jxta jxta_nullID = {
                                {
                                    JXTA_OBJECT_STATIC_INIT,
                                    &jxta_format
                                },
                                "Null"
                            };

Jxta_id * jxta_id_nullID = (Jxta_id *) &jxta_nullID;

_jxta_id_jxta jxta_worldNetPeerGroupID = {
            {
                JXTA_OBJECT_STATIC_INIT,
                &jxta_format
            },
            "WorldGroup"
        };

Jxta_id * jxta_id_worldNetPeerGroupID = (Jxta_id *) &jxta_worldNetPeerGroupID;

_jxta_id_jxta jxta_defaultNetPeerGroupID = {
            {
                JXTA_OBJECT_STATIC_INIT,
                &jxta_format
            },
            "NetGroup"
        };

Jxta_id * jxta_id_defaultNetPeerGroupID = (Jxta_id *) &jxta_defaultNetPeerGroupID;

static _jxta_id_jxta * wellKnownIds[] =
    { &jxta_nullID, &jxta_worldNetPeerGroupID, &jxta_defaultNetPeerGroupID, NULL };

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_id *
translateToWellKnown( Jxta_id* jid ) {
    _jxta_id_jxta ** eachWellKnown = wellKnownIds;

    while( NULL != *eachWellKnown ) {
        if( equals( (Jxta_id*) *eachWellKnown, jid ) )
            return (Jxta_id*) *eachWellKnown;

        eachWellKnown++;
    }

    return jid;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
newPeergroupid1( Jxta_id** pg ) {
    return JXTA_NOTIMP;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
newPeergroupid2( Jxta_id** pg, unsigned char const * seed, size_t len  ) {
    return JXTA_NOTIMP;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
newPeerid1( Jxta_id** peerid, Jxta_id * pg ) {
    return JXTA_NOTIMP;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
newPeerid2( Jxta_id** peerid, Jxta_id * pg, unsigned char const * seed, size_t len ) {
    return JXTA_NOTIMP;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
newCodatid1( Jxta_id** codat, Jxta_id * pg ) {
    return JXTA_NOTIMP;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
newCodatid2( Jxta_id** codat, Jxta_id * pg, ReadFunc read_func, void* stream ) {
    return JXTA_NOTIMP;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
newPipeid1( Jxta_id** pipe, Jxta_id * pg ) {
    return JXTA_NOTIMP;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
newPipeid2( Jxta_id** pipe, Jxta_id * pg, unsigned char const * seed, size_t len ) {
    return JXTA_NOTIMP;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
newModuleclassid1( Jxta_id** mcid ) {
    return JXTA_NOTIMP;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
newModuleclassid2( Jxta_id** mcid, Jxta_id * base ) {
    return JXTA_NOTIMP;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
newModulespecid( Jxta_id** msid, Jxta_id * mcid ) {
    return JXTA_NOTIMP;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
newFromString( Jxta_id** id, JString * jid ) {
    const char * unique;
    Jxta_status res;

    /*  alloc, smear and init   */
    _jxta_id_jxta * me = malloc( sizeof( _jxta_id_jxta ) );

    if( NULL == me )
        return JXTA_NOMEM;

    memset( me, 0xdb, sizeof( _jxta_id_jxta ) );

    JXTA_OBJECT_INIT(me, doDelete, 0);
    me->common.formatter = &jxta_format;

    unique = jstring_get_string(jid);
    if( NULL == unique ) {
        res = JXTA_INVALID_ARGUMENT;
        goto Common_Exit;
    }

    unique = strchr( unique, '-' );
    if( NULL == unique ) {
        res = JXTA_INVALID_ARGUMENT;
        goto Common_Exit;
    }

    unique++;
    me->uniquevalue = strdup( unique );

    *id = translateToWellKnown( (Jxta_id*) me );

    /*
     * Maybe '*id' is the same object as 'me', maybe not. In either case, we're
     * now returning a reference to result and not to 'me'.
     * If they're one and the same, the following operation has no effect.
     */
    JXTA_OBJECT_SHARE( *id );

    res = JXTA_SUCCESS;

Common_Exit:
    JXTA_OBJECT_RELEASE( me );

    return res;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status
getUniqueportion( Jxta_id * jid, JString ** string ) {

    JString * result;

    result = jstring_new_1( strlen(jxta_format.fmt) + 1 + strlen(((_jxta_id_jxta*)jid)->uniquevalue) );

    /*  unique value   */
    jstring_append_2( result, jxta_format.fmt );
    jstring_append_2( result, "-" );
    jstring_append_2( result, ((_jxta_id_jxta*)jid)->uniquevalue );

    *string = result;

    return JXTA_SUCCESS;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean
equals( Jxta_id * jid1, Jxta_id * jid2 ) {
    _jxta_id_jxta* ujid1 = (_jxta_id_jxta*)jid1;
    _jxta_id_jxta* ujid2 = (_jxta_id_jxta*)jid2;

    if( ujid1 == ujid2 )
        return TRUE;

    if( !JXTA_OBJECT_CHECK_VALID (ujid1) )
        return FALSE;

    if( !JXTA_OBJECT_CHECK_VALID (ujid2) )
        return FALSE;

    if (ujid1->common.formatter != ujid2->common.formatter )
        return FALSE;

    /* FIXME 20020327 bondolo@jxta.org : This needs to be case insensitive */

    return (0 == strcmp( ujid1->uniquevalue, ujid2->uniquevalue) ) ? TRUE : FALSE;
}

/******************************************************************************/
/* the free function is never called on static objects, so we can still have  */
/* a real delete method for the few possible cases were we do use dynamic IDs */
/* of this format.                                                            */
/******************************************************************************/
static void
doDelete( Jxta_object * jid ) {
    _jxta_id_jxta* ujid = (_jxta_id_jxta*)jid;

    if( NULL == jid )
        return;

    free( (void*) ujid->uniquevalue );

    memset( (void*) jid, 0xdd, sizeof( _jxta_id_jxta ) );

    free( (void*) jid );
}

/************************************************************************
 **
 *************************************************************************/
static unsigned int
hashcode (Jxta_id* id) {
    _jxta_id_jxta* ujid = (_jxta_id_jxta*)id;
    unsigned int code;

    if( !JXTA_OBJECT_CHECK_VALID (ujid) )
        return 0;

    code = jxta_objecthashtable_simplehash( ujid->uniquevalue, strlen(ujid->uniquevalue) );

    return code;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/

JXTAIDFormat jxta_format = {
                               "jxta",

                               newPeergroupid1,
                               newPeergroupid2,
                               newPeerid1,
                               newPeerid2,
                               newCodatid1,
                               newCodatid2,
                               newPipeid1,
                               newPipeid2,
                               newModuleclassid1,
                               newModuleclassid2,
                               newModulespecid,
                               newFromString,
                               getUniqueportion,
                               equals,
                               hashcode
                           };

#ifdef __cplusplus
}
#endif
