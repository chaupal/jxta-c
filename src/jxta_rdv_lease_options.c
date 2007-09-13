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

static const char *const __log_cat = "RdvLeaseOptions";

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_rdv_lease_options.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */ 
enum tokentype {
    Null_,
    Rdv_Lease_Options_,
};

struct _Jxta_rdv_lease_options {
    Jxta_advertisement jxta_advertisement;

    unsigned int suitability;
    unsigned int willingness;
};

    /* Forward decl. of un-exported function */
static void rdv_lease_options_delete(Jxta_object *);
static void handle_rdv_lease_options(void *me, const XML_Char * cd, int len);
static Jxta_status validate_options(Jxta_rdv_lease_options * myself);


JXTA_DECLARE(unsigned int) jxta_rdv_lease_options_get_suitability(Jxta_rdv_lease_options * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->suitability;
}

JXTA_DECLARE(void) jxta_rdv_lease_options_set_suitability(Jxta_rdv_lease_options * ad, unsigned int suitability )
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->suitability = suitability;
}

JXTA_DECLARE(unsigned int) jxta_rdv_lease_options_get_willingness(Jxta_rdv_lease_options * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->willingness;
}

JXTA_DECLARE(void) jxta_rdv_lease_options_set_willingness(Jxta_rdv_lease_options * ad, unsigned int willingness )
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->willingness = willingness;
}

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handle_rdv_lease_options(void *me, const XML_Char * cd, int len)
{
    Jxta_rdv_lease_options *myself = (Jxta_rdv_lease_options *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:RdvLeaseOptions> : [%pp]\n", myself);

    while (atts && *atts) {
        if (0 == strcmp(*atts, "type")) {
            /* just silently skip it. */
        } else if (0 == strcmp(*atts, "xmlns:jxta")) {
            /* just silently skip it. */
        } else if (0 == strcmp(*atts, "suitability")) {
                myself->suitability = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "willingness")) {
                myself->willingness = atoi(atts[1]);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
        }

        atts += 2;
    }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:RdvLeaseOptions> : [%pp]\n", myself);
    }
}

static Jxta_status validate_options(Jxta_rdv_lease_options * myself) 
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if( myself->suitability > 100 ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Suitability must be <= 100 [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    if( myself->willingness > 100 ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Willingness must be <= 100 [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
    
    return JXTA_SUCCESS;
}


/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */

static const Kwdtab JXTA_RDV_LEASE_OPTIONS_TAGS[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:RdvLeaseOptions", Rdv_Lease_Options_, *handle_rdv_lease_options, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};



JXTA_DECLARE(Jxta_status) jxta_rdv_lease_options_get_xml(Jxta_rdv_lease_options * myself, JString ** xml)
{
    Jxta_status res;
    JString *string;
    char tmpbuf[256];   /* We use this buffer to store a string representation of a int */

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    res = validate_options(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    }
    
    string = jstring_new_0();

    jstring_append_2(string, "<Option type=\"jxta:RdvLeaseOptions\"");

    if ( 0 != myself->suitability ) {
        jstring_append_2(string, " suitability=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", myself->suitability);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if ( 0 != myself->willingness ) {
        jstring_append_2(string, " willingness=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", myself->willingness);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    jstring_append_2(string, "/>\n");

    *xml = string;
    
    return JXTA_SUCCESS;
}


Jxta_rdv_lease_options *rdv_lease_options_construct(Jxta_rdv_lease_options * self)
{
    self = (Jxta_rdv_lease_options *)
        jxta_advertisement_construct((Jxta_advertisement *) self,
                                     "jxta:RdvLeaseOptions",
                                     JXTA_RDV_LEASE_OPTIONS_TAGS,
                                     (JxtaAdvertisementGetXMLFunc) jxta_rdv_lease_options_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL,
                                     (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != self) {
        self->suitability = 0;
        self->willingness = 0;
    }

    return self;
}

void jxta_rdv_lease_options_destruct(Jxta_rdv_lease_options * self)
{
    jxta_advertisement_destruct((Jxta_advertisement *) self);

}

/** 
 *   Get a new instance of the ad. 
 **/
JXTA_DECLARE(Jxta_rdv_lease_options *) jxta_rdv_lease_options_new(void)
{
    Jxta_rdv_lease_options *myself = (Jxta_rdv_lease_options *) calloc(1, sizeof(Jxta_rdv_lease_options));

    JXTA_OBJECT_INIT(myself, rdv_lease_options_delete, 0);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "RdvLeaseOptions NEW [%pp]\n", myself );

    return rdv_lease_options_construct(myself);
}

static void rdv_lease_options_delete(Jxta_object * myself)
{
    jxta_rdv_lease_options_destruct((Jxta_rdv_lease_options *) myself);

    memset(myself, 0xDD, sizeof(Jxta_rdv_lease_options));
    
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "RdvLeaseOptions FREE [%pp]\n", myself );

    free(myself);
}

JXTA_DECLARE(Jxta_status) jxta_rdv_lease_options_parse_charbuffer(Jxta_rdv_lease_options * myself, const char *buf, int len)
{
    Jxta_status res;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    res =  jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_options(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_lease_options_parse_file(Jxta_rdv_lease_options * myself, FILE * stream)
{
    Jxta_status res;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_options(myself);
    }
    
    return res;
}


/* vim: set ts=4 sw=4 et tw=130: */
