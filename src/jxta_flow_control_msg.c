/*
 * Copyright (c) 2006 Sun Microsystems, Inc.  All rights reserved.
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
 *    if any, must include the following acknowledgement:
 *       "This product includes software developed by the
 *       Sun Microsystems, Inc. for Project JXTA."
 *    Alternately, this acknowledgement may appear in the software itself,
 *    if and wherever such third-party acknowledgements normally appear.
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
 * $Id: jxta_peerview_address_assign_msg.c 416 2009-11-12 23:38:46Z ExocetRick $
 */

static const char *__log_cat = "FC_MSG";

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_flow_control_msg.h"

struct _jxta_ep_flow_control_msg {
    Jxta_advertisement jxta_advertisement;

    Jxta_id *peer_id;
    Jxta_time time;
    apr_int64_t size;
    int interval;
    int frame;
    int look_ahead;
    int reserve;
    Ts_max_option max_option;
};

enum tokentype {
    Null_,
    Jxta_ep_flow_control_msg_,
    FlowControl_,
};
static void jxta_ep_flow_control_msg_delete(Jxta_object * me);
static void handle_ep_flow_control_msg(void *me, const XML_Char * cd, int len);

static Jxta_status validate_message(Jxta_ep_flow_control_msg * myself);

/** 
 * Now, build an array of the keyword structs.  Since
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, diffusion through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab jxta_ep_flow_control_msg_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:EPFlowControlMessage", Jxta_ep_flow_control_msg_, *handle_ep_flow_control_msg, NULL, NULL},
    {NULL, 0, NULL, NULL, NULL}
};

    /** Get a new instance of the ad.
     */
JXTA_DECLARE(Jxta_ep_flow_control_msg *) jxta_ep_flow_control_msg_new(void)
{
    Jxta_ep_flow_control_msg *myself = (Jxta_ep_flow_control_msg *) calloc(1, sizeof(Jxta_ep_flow_control_msg));

    if (NULL == myself) {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_ep_flow_control_msg NEW [%pp]\n", myself);

    JXTA_OBJECT_INIT(myself, jxta_ep_flow_control_msg_delete, NULL);

    myself = (Jxta_ep_flow_control_msg *) jxta_advertisement_construct((Jxta_advertisement *) myself,
                                                                 "jxta:EPFlowControlMessage",
                                                                 jxta_ep_flow_control_msg_tags,
                                                                 (JxtaAdvertisementGetXMLFunc) jxta_ep_flow_control_msg_get_xml,
                                                                 (JxtaAdvertisementGetIDFunc) NULL,
                                                                 (JxtaAdvertisementGetIndexFunc) NULL);


    return myself;
}

JXTA_DECLARE(Jxta_ep_flow_control_msg *) jxta_ep_flow_control_msg_new_1(int fc_time
                            , apr_int64_t fc_size, int fc_interval, int fc_frame
                            , int fc_look_ahead, int fc_reserve, Ts_max_option max_option){
    Jxta_ep_flow_control_msg * myself;

    myself = jxta_ep_flow_control_msg_new();

    myself->time = fc_time;
    myself->size = fc_size;
    myself->interval = fc_interval;
    myself->frame = fc_frame;
    myself->look_ahead = fc_look_ahead;
    myself->reserve = fc_reserve;
    myself->max_option = TS_MAX_OPTION_FRAME;
    return myself;
}

static void jxta_ep_flow_control_msg_delete(Jxta_object * me)
{
    Jxta_ep_flow_control_msg *myself = (Jxta_ep_flow_control_msg *) me;


    jxta_advertisement_destruct((Jxta_advertisement *) myself);

    memset(myself, 0xdd, sizeof(Jxta_ep_flow_control_msg));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_ep_flow_control_msg FREE [%pp]\n", myself);

    free(myself);
}


JXTA_DECLARE(Jxta_status) jxta_ep_flow_control_msg_parse_charbuffer(Jxta_ep_flow_control_msg * myself, const char *buf, int len)
{
    Jxta_status res = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_ep_flow_control_msg_parse_file(Jxta_ep_flow_control_msg * myself, FILE * stream)
{
    Jxta_status res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

static Jxta_status validate_message(Jxta_ep_flow_control_msg * myself) {


    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_ep_flow_control_msg_get_xml(Jxta_ep_flow_control_msg * myself, JString ** xml)
{
    Jxta_status res;
    JString *string;
    JString *peer_j=NULL;
    char tmp[256];

    JXTA_OBJECT_CHECK_VALID(myself);

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    
    res = validate_message(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    }
    
    string = jstring_new_0();

    jstring_append_2(string, "<jxta:EPFlowControlMessage");
    if (NULL != myself->peer_id) {
        jxta_id_to_jstring(myself->peer_id, &peer_j);
        jstring_append_2(string, " peer_id=\"");
        jstring_append_2(string, jstring_get_string(peer_j));
        jstring_append_2(string, "\"");
    }
    apr_snprintf(tmp, sizeof(tmp), " fc_time=\"%ld\"", (long) myself->time);
    jstring_append_2(string, tmp);

    apr_snprintf(tmp, sizeof(tmp), " fc_size=\"%" APR_INT64_T_FMT "\"", myself->size);
    jstring_append_2(string, tmp);

    apr_snprintf(tmp, sizeof(tmp), " fc_interval=\"%d\"", myself->interval);
    jstring_append_2(string, tmp);
    apr_snprintf(tmp, sizeof(tmp), " fc_frame=\"%d\"", myself->frame);
    jstring_append_2(string, tmp);
    apr_snprintf(tmp, sizeof(tmp), " fc_look_ahead=\"%d\"", myself->look_ahead);
    jstring_append_2(string, tmp);
    apr_snprintf(tmp, sizeof(tmp), " fc_reserve=\"%d\"", myself->reserve);
    jstring_append_2(string, tmp);
    jstring_append_2(string, " fc_max_option=\"");
    jstring_append_2(string, myself->max_option == TS_MAX_OPTION_LOOK_AHEAD ? "look_ahead":"frame");
    jstring_append_2(string, "\"");

    jstring_append_2(string, "/>");


    *xml = string;
    if (peer_j)
        JXTA_OBJECT_RELEASE(peer_j);

    return JXTA_SUCCESS;
}


static void handle_ep_flow_control_msg(void *me, const XML_Char * cd, int len)
{
    Jxta_ep_flow_control_msg *myself = (Jxta_ep_flow_control_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <EPFlowControlMsg> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "peer_id")) {
                if (JXTA_SUCCESS != jxta_id_from_cstr(&myself->peer_id, atts[1])) {
                    myself->peer_id = NULL;
                }
            } else if (0 == strcmp(*atts, "fc_time")) {
                myself->time = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "fc_size")) {
                myself->size = apr_atoi64(atts[1]);
            } else if (0 == strcmp(*atts, "fc_interval")) {
                myself->interval = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "fc_frame")) {
                myself->frame = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "fc_look_ahead")) {
                myself->look_ahead = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "fc_reserve")) {
                myself->reserve = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "fc_max_option")) {
                myself->max_option = strcmp(atts[1],"frame") == 0 ? TS_MAX_OPTION_FRAME:TS_MAX_OPTION_LOOK_AHEAD;
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }
            atts += 2;
        }        
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <EPFlowControlMsg> : [%pp]\n", myself);
    }
}


JXTA_DECLARE(Jxta_status) jxta_ep_flow_control_msg_set_peerid(Jxta_ep_flow_control_msg * myself, Jxta_id * peer_id)
{
    if (myself->peer_id != NULL) {
        JXTA_OBJECT_RELEASE(myself->peer_id);
    }
    myself->peer_id = JXTA_OBJECT_SHARE(peer_id);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_ep_flow_control_msg_get_peerid(Jxta_ep_flow_control_msg * myself, Jxta_id ** peer_id)
{

    if (NULL != myself->peer_id) {
        JXTA_OBJECT_SHARE(myself->peer_id);
    }
    *peer_id = myself->peer_id;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_size(Jxta_ep_flow_control_msg * myself, apr_int64_t size)
{
    myself->size = size;
}

JXTA_DECLARE(apr_int64_t) jxta_ep_flow_control_msg_get_size(Jxta_ep_flow_control_msg * myself)
{
    return myself->size;

}


JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_time(Jxta_ep_flow_control_msg * myself, Jxta_time time)
{
   myself->time = time;
}

JXTA_DECLARE(Jxta_time) jxta_ep_flow_control_msg_get_time(Jxta_ep_flow_control_msg * myself)
{
    return myself->time;

}


JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_interval(Jxta_ep_flow_control_msg * myself, int interval)
{
   myself->interval = interval;

}

JXTA_DECLARE(int) jxta_ep_flow_control_msg_get_interval(Jxta_ep_flow_control_msg * myself)
{
    return myself->interval;

}


JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_frame(Jxta_ep_flow_control_msg * myself, int frame)
{
   myself->frame = frame;

}

JXTA_DECLARE(int) jxta_ep_flow_control_msg_get_frame(Jxta_ep_flow_control_msg * myself)
{
    return myself->frame;

}

JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_reserve(Jxta_ep_flow_control_msg * myself, int reserve)
{
   myself->reserve = reserve;

}

JXTA_DECLARE(int) jxta_ep_flow_control_msg_get_reserve(Jxta_ep_flow_control_msg * myself)
{
    return myself->reserve;

}


JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_look_ahead(Jxta_ep_flow_control_msg * myself, int look_ahead)
{
   myself->look_ahead = look_ahead;

}

JXTA_DECLARE(int) jxta_ep_flow_control_msg_get_look_ahead(Jxta_ep_flow_control_msg * myself)
{
    return myself->look_ahead;

}

JXTA_DECLARE(void) jxta_ep_flow_control_msg_set_max_option(Jxta_ep_flow_control_msg * myself, Ts_max_option max_option)
{
   myself->max_option = max_option;

}

JXTA_DECLARE(Ts_max_option) jxta_ep_flow_control_msg_get_max_option(Jxta_ep_flow_control_msg * myself)
{
    return myself->max_option;

}

/* vim: set ts=4 sw=4 et tw=130: */
