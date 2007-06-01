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
 * $Id: jxta_rq.c,v 1.82.2.1 2007/01/02 18:15:39 slowhog Exp $
 */

static const char *__log_cat = "RSLVQuery";

#include <stdio.h>
#include <string.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_rq.h"
#include "jxta_xml_util.h"
#include "jxta_routea.h"
#include "jxta_apa.h"
#include "jxta_apr.h"

/**
 * Defines invalid query id
 */
const int JXTA_INVALID_QUERY_ID = -1;

/** Each of these corresponds to a tag in the
 * xml ad.
 */
enum tokentype {
    Null_,
    ResolverQuery_,
    Credential_,
    SrcPeerID_,
    SrcRoute_,
    HandlerName_,
    QueryID_,
    Query_,
    HopCount_
};

/** This is the representation of the
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _ResolverQuery {
    Jxta_advertisement jxta_advertisement;
    JString *Credential;
    Jxta_id *SrcPeerID;
    Jxta_RouteAdvertisement *route;
    JString *HandlerName;
    long QueryID;
    JString *Query;
    long HopCount;
    const Jxta_qos *qos;
};

/**
 * frees the ResolverQuery object
 * @param me the resolver query object to free
 */
static void resolver_query_delete(Jxta_object * me);
ResolverQuery * resolver_query_construct(ResolverQuery * myself);
static Jxta_status validate_msg(ResolverQuery * myself);

/** Handler functions.  Each of these is responsible for
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleResolverQuery(void *me, const XML_Char * cd, int len)
{
    ResolverQuery *myself = (ResolverQuery *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:ResolverQuery> : [%pp]\n", myself);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:ResolverQuery> : [%pp]\n", myself);
    }
}

static void handleCredential(void *userdata, const XML_Char * cd, int len)
{
    ResolverQuery *ad = (ResolverQuery *) userdata;
    jstring_append_0(((struct _ResolverQuery *) ad)->Credential, cd, len);
}

static void handleSrcPeerID(void *me, const XML_Char * cd, int len)
{
    ResolverQuery *msg = (ResolverQuery *) me;

    JXTA_OBJECT_CHECK_VALID(msg);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <SrcPeerID> : [%pp]\n", msg);
    } else {
        Jxta_status res;
        JString *tmp = jstring_new_1(len );
        Jxta_id *pid = NULL;

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);

        res = jxta_id_from_jstring(&pid, tmp);
        if( JXTA_SUCCESS == res ) {
            jxta_resolver_query_set_src_peer_id(msg, pid);
            JXTA_OBJECT_RELEASE(pid);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid PID %s : [%pp]\n", jstring_get_string(tmp), msg);    
        }
        
        JXTA_OBJECT_RELEASE(tmp);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <SrcPeerID> : [%pp]\n", msg);
    }
}

static void handleSrcRoute(void *me, const XML_Char * cd, int len)
{
    ResolverQuery *msg = (ResolverQuery *) me;
    
    JXTA_OBJECT_CHECK_VALID(msg);
    
    if (len == 0) {
        Jxta_RouteAdvertisement *ra;
        
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <SrcPeerRoute> Element [%pp]\n", msg );
        
        ra = jxta_RouteAdvertisement_new();

        jxta_resolver_query_set_src_peer_route(msg, ra);

        jxta_advertisement_set_handlers((Jxta_advertisement *) ra, ((Jxta_advertisement *) msg)->parser, (void *) msg);
        JXTA_OBJECT_RELEASE(ra);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <SrcPeerRoute> Element [%pp]\n", msg );
    }
}

static void handleHandlerName(void *me, const XML_Char * cd, int len)
{
    ResolverQuery *ad = (ResolverQuery *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <HandlerName> : [%pp]\n", ad);
    } else {
        JString *name = jstring_new_1(len );

        jstring_append_0( name, cd, len);
        jstring_trim(name);

        jxta_resolver_query_set_handlername(ad, name);

        JXTA_OBJECT_RELEASE(name);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <HandlerName> : [%pp]\n", ad);
    }
}

static void handleQueryID(void *me, const XML_Char * cd, int len)
{
    ResolverQuery *ad = (ResolverQuery *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <QueryID> : [%pp]\n", ad);
    } else {
        JString *query = jstring_new_1(len );

        jstring_append_0( query, cd, len);
        jstring_trim(query);

        jxta_resolver_query_set_queryid(ad, strtol(jstring_get_string(query), NULL, 0) );

        JXTA_OBJECT_RELEASE(query);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <QueryID> : [%pp]\n", ad);
    }
}

static void handleQuery(void *me, const XML_Char * cd, int len)
{
    ResolverQuery *ad = (ResolverQuery *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Query> : [%pp]\n", ad);
    } else {
        JString *query = jstring_new_1(len );

        jstring_append_0( query, cd, len);
        jstring_trim(query);

        jxta_resolver_query_set_query(ad, query);

        JXTA_OBJECT_RELEASE(query);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Query> : [%pp]\n", ad);
    }
}

static void handleHopCount(void *me, const XML_Char * cd, int len)
{
    ResolverQuery *ad = (ResolverQuery *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <HC> : [%pp]\n", ad);
    } else {
        JString *hops = jstring_new_1(len );

        jstring_append_0( hops, cd, len);
        jstring_trim(hops);

        jxta_resolver_query_set_hopcount(ad, atoi(jstring_get_string(hops)) );

        JXTA_OBJECT_RELEASE(hops);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <HC> : [%pp]\n", ad);
    }
}

static void trim_elements(ResolverQuery * adv)
{
    jstring_trim(adv->Credential);
    jstring_trim(adv->HandlerName);
    jstring_trim(adv->Query);
}

/**
 * The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */


static Jxta_status validate_msg(ResolverQuery * myself) {

    if ( jxta_id_equals(myself->SrcPeerID, jxta_id_nullID)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "src peer id must not be null ID [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    if( NULL == myself->Query ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Query must not be null ID [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
    
    return JXTA_SUCCESS;
}

/**
 * return a JString representation of the advertisement
 * it is the responsibility of the caller to release the JString object
 * @param adv a pointer to the advertisement.
 * @param doc  a pointer to the JString object representing the document.
 * @return Jxta_status 
 */

JXTA_DECLARE(Jxta_status) jxta_resolver_query_get_xml(ResolverQuery * adv, JString ** document)
{
    JString *doc;
    JString *tmps = NULL;
    char *buf;
    int buflen;
    Jxta_status status;

    buflen = 128;
    buf = (char *) calloc(1, buflen);

    trim_elements(adv);
    doc = jstring_new_2("<?xml version=\"1.0\"?>\n");
    jstring_append_2(doc, "<!DOCTYPE jxta:ResolverQuery>");
    jstring_append_2(doc, "<jxta:ResolverQuery>\n");

    if (jstring_length(adv->Credential) != 0) {
        jstring_append_2(doc, "<jxta:Cred>");
        jstring_append_1(doc, adv->Credential);
        jstring_append_2(doc, "</jxta:Cred>\n");
    }

    if (adv->route != NULL) {
        jstring_append_2(doc, "<SrcPeerRoute>");
        jxta_RouteAdvertisement_get_xml(adv->route, &tmps);
        jstring_append_1(doc, tmps);
        JXTA_OBJECT_RELEASE(tmps);
        jstring_append_2(doc, "</SrcPeerRoute>\n");
    }

    jstring_append_2(doc, "<SrcPeerID>");
    jxta_id_to_jstring(adv->SrcPeerID, &tmps);
    jstring_append_1(doc, tmps);
    JXTA_OBJECT_RELEASE(tmps);
    jstring_append_2(doc, "</SrcPeerID>\n");

    jstring_append_2(doc, "<HandlerName>");
    jstring_append_1(doc, adv->HandlerName);
    jstring_append_2(doc, "</HandlerName>\n");

    apr_snprintf(buf, buflen, "%ld\n", adv->QueryID);
    jstring_append_2(doc, "<QueryID>");
    jstring_append_2(doc, buf);
    jstring_append_2(doc, "</QueryID>\n");

    memset(buf, 0, buflen);
    apr_snprintf(buf, buflen, "%ld\n", adv->HopCount);
    jstring_append_2(doc, "<HC>");
    jstring_append_2(doc, buf);
    jstring_append_2(doc, "</HC>\n");

    /* done with buf, free it */
    free(buf);

    jstring_append_2(doc, "<Query>");
    status = jxta_xml_util_encode_jstring(adv->Query, &tmps);
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR , "error encoding the query, return status :%d\n", status);
        JXTA_OBJECT_RELEASE(doc);
        return status;
    }
    jstring_append_1(doc, tmps);
    JXTA_OBJECT_RELEASE(tmps);
    jstring_append_2(doc, "</Query>\n");

    jstring_append_2(doc, "</jxta:ResolverQuery>\n");

    *document = doc;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(JString *) jxta_resolver_query_get_credential(ResolverQuery * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->Credential) {
        jstring_trim(ad->Credential);
        JXTA_OBJECT_SHARE(ad->Credential);
    }
    return ad->Credential;
}

JXTA_DECLARE(void) jxta_resolver_query_set_credential(ResolverQuery * ad, JString * credential)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->Credential) {
        JXTA_OBJECT_RELEASE(ad->Credential);
        ad->Credential = NULL;
    }
    JXTA_OBJECT_SHARE(credential);
    ad->Credential = credential;
}

JXTA_DECLARE(Jxta_id *) jxta_resolver_query_get_src_peer_id(ResolverQuery * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->SrcPeerID) {
        JXTA_OBJECT_SHARE(ad->SrcPeerID);
    }
    return ad->SrcPeerID;
}

JXTA_DECLARE(void) jxta_resolver_query_set_src_peer_id(ResolverQuery * ad, Jxta_id * id)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->SrcPeerID) {
        JXTA_OBJECT_RELEASE(ad->SrcPeerID);
        ad->SrcPeerID = NULL;
    }
    
    ad->SrcPeerID = JXTA_OBJECT_SHARE(id);
}

JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_resolver_query_src_peer_route(ResolverQuery * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->route;
}

JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_resolver_query_get_src_peer_route(ResolverQuery * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->route != NULL) {
        JXTA_OBJECT_SHARE(ad->route);
    }

    return ad->route;
}

JXTA_DECLARE(void) jxta_resolver_query_set_src_peer_route(ResolverQuery * ad, Jxta_RouteAdvertisement * route)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->route != NULL) {
        JXTA_OBJECT_RELEASE(ad->route);
    }

	ad->route = JXTA_OBJECT_SHARE(route);
}

JXTA_DECLARE(void) jxta_resolver_query_get_handlername(ResolverQuery * ad, JString ** str)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->HandlerName) {
        jstring_trim(ad->HandlerName);
        JXTA_OBJECT_SHARE(ad->HandlerName);
    }
    *str = ad->HandlerName;
}

JXTA_DECLARE(void) jxta_resolver_query_set_handlername(ResolverQuery * ad, JString * handlerName)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->HandlerName) {
        JXTA_OBJECT_RELEASE(ad->HandlerName);
        ad->HandlerName = NULL;
    }
    
    ad->HandlerName = JXTA_OBJECT_SHARE(handlerName);
}

JXTA_DECLARE(long) jxta_resolver_query_get_queryid(ResolverQuery * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->QueryID;
}

JXTA_DECLARE(void) jxta_resolver_query_set_queryid(ResolverQuery * ad, long qid)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->QueryID = qid;
}

JXTA_DECLARE(void) jxta_resolver_query_get_query(ResolverQuery * ad, JString ** query)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->Query) {
        jstring_trim(ad->Query);
        JXTA_OBJECT_SHARE(ad->Query);
    }
    
    *query = ad->Query;
}

JXTA_DECLARE(void) jxta_resolver_query_set_query(ResolverQuery * ad, JString * query)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->Query != NULL) {
        JXTA_OBJECT_RELEASE(ad->Query);
        ad->Query = NULL;
    }

    ad->Query = JXTA_OBJECT_SHARE(query);
}

 /**
 *
 */
void jxta_resolver_query_set_hopcount(ResolverQuery * ad, int hopcount)
{
   JXTA_OBJECT_CHECK_VALID(ad);

   ad->HopCount = hopcount;
}

 /**
 *
 */
long jxta_resolver_query_get_hopcount(ResolverQuery * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->HopCount;
}

/** Now, build an array of the keyword structs.  Since
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab ResolverQuery_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:ResolverQuery", ResolverQuery_, *handleResolverQuery, NULL, NULL},
    {"jxta:Cred", Credential_, *handleCredential, NULL, NULL},
    {"SrcPeerID", SrcPeerID_, *handleSrcPeerID, NULL, NULL},
    {"SrcPeerRoute", SrcRoute_, *handleSrcRoute, NULL, NULL},
    {"HandlerName", HandlerName_, *handleHandlerName, NULL, NULL},
    {"QueryID", QueryID_, *handleQueryID, NULL, NULL},
    {"Query", Query_, *handleQuery, NULL, NULL},
    {"HC", HopCount_, *handleHopCount, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

/** Get a new instance of the ad.
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
JXTA_DECLARE(ResolverQuery *) jxta_resolver_query_new(void)
{
    ResolverQuery *myself = (ResolverQuery *) calloc(1, sizeof(ResolverQuery));

    JXTA_OBJECT_INIT(myself, resolver_query_delete, 0);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ResolverQuery NEW [%pp]\n", myself );

    return resolver_query_construct(myself);
}

ResolverQuery * resolver_query_construct(ResolverQuery * myself)
{
    myself = (ResolverQuery *) jxta_advertisement_construct((Jxta_advertisement *) myself,
                                  "jxta:ResolverQuery",
                                  ResolverQuery_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_resolver_query_get_xml,
                                  (JxtaAdvertisementGetIDFunc) NULL,
                                  (JxtaAdvertisementGetIndexFunc) NULL);

    if( NULL != myself ) {
        myself->SrcPeerID = JXTA_OBJECT_SHARE(jxta_id_nullID);
        myself->Credential = jstring_new_0();
        myself->HandlerName = jstring_new_0();
        myself->QueryID = JXTA_INVALID_QUERY_ID;
        myself->Query = NULL;
        myself->route = NULL;
        myself->HopCount = 0;
        myself->qos = NULL;
    }
    
    return myself;
}

Jxta_status resolver_query_create(JString * handlername, JString * qdoc, Jxta_id * src_peerid, Jxta_resolver_query ** rq)
{
    Jxta_resolver_query *msg = jxta_resolver_query_new();

    if( NULL == msg ) {
        return JXTA_NOMEM;
    }

    jxta_resolver_query_set_src_peer_id(msg, src_peerid);
    jstring_append_1(msg->HandlerName, handlername);
    jxta_resolver_query_set_query( msg, qdoc );

    *rq = msg;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(ResolverQuery *) jxta_resolver_query_new_1(JString * handlername, JString * query, Jxta_id * src_pid,
                                                        Jxta_RouteAdvertisement * route)
{
    Jxta_resolver_query *msg = jxta_resolver_query_new();

    if( NULL == msg ) {
        return NULL;
    }

    jxta_resolver_query_set_src_peer_id(msg, src_pid);
    msg->HandlerName = jstring_new_2(jstring_get_string(handlername));
    jxta_resolver_query_set_query( msg, query );

    if (route) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE 
                        "Obsolete constructor usage to include local route advertisement\n");
    }

    return msg;
}

 /** 
  * Shred the memory going out.  Again,
  * if there ever was a segfault (unlikely,
  * of course), the hex value dddddddd will 
  * pop right out as a piece of memory accessed
  * after it was freed...
  */
static void resolver_query_delete(Jxta_object * me)
{
    ResolverQuery * myself = (ResolverQuery * )me;
        
    if (myself->Credential) {
        JXTA_OBJECT_RELEASE(myself->Credential);
    }
    
    if (myself->SrcPeerID) {
        JXTA_OBJECT_RELEASE(myself->SrcPeerID);
    }
    
    if (myself->HandlerName) {
        JXTA_OBJECT_RELEASE(myself->HandlerName);
    }
    
    if (myself->Query) {
        JXTA_OBJECT_RELEASE(myself->Query);
    }
    if (myself->route != NULL) {
        JXTA_OBJECT_RELEASE(myself->route);
    }

    jxta_advertisement_destruct((Jxta_advertisement *) myself);
    
    memset(myself, 0xDD, sizeof(ResolverQuery));
    
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ResolverQuery FREE [%pp]\n", myself );

    free(myself);
}

JXTA_DECLARE(Jxta_status) jxta_resolver_query_parse_charbuffer(ResolverQuery * myself, const char *buf, int len)
{
    Jxta_status res;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    res =  jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_msg(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_resolver_query_parse_file(ResolverQuery * myself, FILE * stream)
{
    Jxta_status res;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_msg(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_resolver_query_attach_qos(Jxta_resolver_query * me, const Jxta_qos * qos)
{
    JXTA_OBJECT_CHECK_VALID(me);

    me->qos = qos;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(const Jxta_qos *) jxta_resolver_query_qos(Jxta_resolver_query * me)
{
    JXTA_OBJECT_CHECK_VALID(me);

    return me->qos;
}

/* vi: set ts=4 sw=4 tw=130 et: */
