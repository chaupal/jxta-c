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
 * $Id: jxta_resolver_config_adv.c 188 2007-12-21 17:08:49Z vfinley $
 */

static const char *const __log_cat = "RslvrCfgAdv";

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_resolver_config_adv.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"
#include "jxta_apr.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
    Null_,
    Jxta_ResolverConfigAdvertisement_,
    addr_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_ResolverConfigAdvertisement {
    Jxta_advertisement jxta_advertisement;
    Jxta_boolean always_propagate;
};

    /* Forward decl. of un-exported function */
static void jxta_ResolverConfigAdvertisement_delete(Jxta_object *);



/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
void handleJxta_ResolverConfigAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    Jxta_ResolverConfigAdvertisement *ad = (Jxta_ResolverConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Begining parse of jxta:ResolverConfig\n");

    while (atts && *atts) {
        if (0 == strcmp(*atts, "type")) {
            /* just silently skip it. */
        } else if (0 == strcmp(*atts, "alwaysPropagate")) {
            ad->always_propagate = (0 == strncasecmp(atts[1], "true", 4)) ? TRUE : FALSE;
        }
        atts += 2;
    }
}

JXTA_DECLARE(void) jxta_ResolverConfigAdvertisement_set_always_propagate(Jxta_ResolverConfigAdvertisement * adv, Jxta_boolean propagate)
{
    adv->always_propagate = propagate;
}

JXTA_DECLARE(Jxta_boolean) jxta_ResolverConfigAdvertisement_get_always_propagate(Jxta_ResolverConfigAdvertisement * adv)
{
    return adv->always_propagate;
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */

static const Kwdtab Jxta_ResolverConfigAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:ResolverConfig", Jxta_ResolverConfigAdvertisement_, *handleJxta_ResolverConfigAdvertisement, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_ResolverConfigAdvertisement_get_xml(Jxta_ResolverConfigAdvertisement * ad, JString ** result)
{
    JString *string = jstring_new_0();
    jstring_append_2(string, "<!-- JXTA Resolver Configuration Advertisement -->\n");
    jstring_append_2(string, "<jxta:ResolverConfig xmlns:jxta=\"http://jxta.org\" type=\"jxta:ResolverConfig\"\n");
    jstring_append_2(string, " alwaysPropagate=\"");
    jstring_append_2(string, (ad->always_propagate == TRUE) ? "true" : "false");
    jstring_append_2(string, "\"");
    jstring_append_2(string, ">\n");
    jstring_append_2(string, "</jxta:ResolverConfig>\n");

    *result = string;
    return JXTA_SUCCESS;
}


Jxta_ResolverConfigAdvertisement *jxta_ResolverConfigAdvertisement_construct(Jxta_ResolverConfigAdvertisement * self)
{
    self = (Jxta_ResolverConfigAdvertisement *)
        jxta_advertisement_construct((Jxta_advertisement *) self,
                                     "jxta:ResolverConfig",
                                     Jxta_ResolverConfigAdvertisement_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_ResolverConfigAdvertisement_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != self) {
        self->always_propagate = FALSE;
    }

    return self;
}

void jxta_ResolverConfigAdvertisement_destruct(Jxta_ResolverConfigAdvertisement * self)
{
    jxta_advertisement_destruct((Jxta_advertisement *) self);

}

/** 
 *   Get a new instance of the ad. 
 **/
JXTA_DECLARE(Jxta_ResolverConfigAdvertisement *) jxta_ResolverConfigAdvertisement_new(void)
{
    Jxta_ResolverConfigAdvertisement *ad =
        (Jxta_ResolverConfigAdvertisement *) calloc(1, sizeof(Jxta_ResolverConfigAdvertisement));

    JXTA_OBJECT_INIT(ad, jxta_ResolverConfigAdvertisement_delete, 0);

    return jxta_ResolverConfigAdvertisement_construct(ad);
}

static void jxta_ResolverConfigAdvertisement_delete(Jxta_object * ad)
{
    jxta_ResolverConfigAdvertisement_destruct((Jxta_ResolverConfigAdvertisement *) ad);

    memset(ad, 0xDD, sizeof(Jxta_ResolverConfigAdvertisement));
    free(ad);
}

JXTA_DECLARE(void) jxta_ResolverConfigAdvertisement_parse_charbuffer(Jxta_ResolverConfigAdvertisement * ad, const char *buf,
                                                                      size_t len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_ResolverConfigAdvertisement_parse_file(Jxta_ResolverConfigAdvertisement * ad, FILE * stream)
{

    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

/* vim: set ts=4 sw=4 et tw=130: */
