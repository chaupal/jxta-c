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

static const char *const __log_cat = "CacheCfgAdv";

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_cache_config_adv.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"
#include "jxta_apr.h"

/* defaults */

#define AS_DBENGINE_DEFAULT "sqlite3"
#define AS_DBNAME_DEFAULT "groupID"

#define XACTION_THRESHOLD_DEFAULT 50

/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
    Null_,
    Jxta_CacheConfigAdvertisement_,
    AddressSpace_,
    NameSpace_,
};


struct jxta_CacheConfigAdvertisement {
    Jxta_advertisement jxta_advertisement;
    Jxta_vector *addressSpaces;
    Jxta_vector *nameSpaces;
    Jxta_addressSpace *defaultAS;
    Jxta_boolean sharedDB;
    int thd_to_add;
    int thd_max;
};

struct jxta_addressSpace {
    JXTA_OBJECT_HANDLE;
    Jxta_boolean ephemeral;
    JString *name;
    JString *DBName;
    JString *DBEngine;
    Jxta_time_diff persistInterval;
    JString *persistName;
    JString *QoS;
    JString *location;
    JString *connectString;
    Jxta_boolean highPerformance;
    Jxta_boolean autoVacuum;
    Jxta_boolean isDefault;
    Jxta_boolean isDelta;
    int xactionThreshold;
};

struct jxta_nameSpace {
    JXTA_OBJECT_HANDLE;
    JString *ns;
    JString *prefix;
    JString *advType;
    JString *jAddressSpace;
    Jxta_addressSpace *addressSpace;
    Jxta_boolean type_is_wildcard;
};

    /* Forward decl. of un-exported function */
static void jxta_CacheConfigAdvertisement_delete(Jxta_object *);


static void freeAddressSpace(Jxta_object * obj)
{
    Jxta_addressSpace *as = (Jxta_addressSpace *) obj;
    if (as->name)
        JXTA_OBJECT_RELEASE(as->name);
    if (as->DBName)
        JXTA_OBJECT_RELEASE(as->DBName);
    if (as->DBEngine)
        JXTA_OBJECT_RELEASE(as->DBEngine);
    if (as->persistName)
        JXTA_OBJECT_RELEASE(as->persistName);
    if (as->QoS)
        JXTA_OBJECT_RELEASE(as->QoS);
    if (as->location)
        JXTA_OBJECT_RELEASE(as->location);
    if (as->connectString)
        JXTA_OBJECT_RELEASE(as->connectString);
    free((void *) obj);
}

static void freeNameSpace(Jxta_object * obj)
{
    Jxta_nameSpace *ns = (Jxta_nameSpace *) obj;
    if (ns->ns)
        JXTA_OBJECT_RELEASE(ns->ns);
    if (ns->jAddressSpace)
        JXTA_OBJECT_RELEASE(ns->jAddressSpace);
    if (ns->addressSpace)
        JXTA_OBJECT_RELEASE(ns->addressSpace);
    if (ns->prefix)
        JXTA_OBJECT_RELEASE(ns->prefix);
    if (ns->advType)
        JXTA_OBJECT_RELEASE(ns->advType);
    free((void *) obj);
}

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
void handleJxta_CacheConfigAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    Jxta_CacheConfigAdvertisement *ad = (Jxta_CacheConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Begining parse of jxta:CacheConfig\n");
    while (atts && *atts) {
        if (0 == strcmp(*atts, "sharedDB")) {
            ad->sharedDB = 0 == strcmp(atts[1], "yes");
        } else if (0 == strcmp(*atts, "threads")) {
            if (0 == strcmp(atts[1], "default")) {
                ad->thd_to_add = -1;
            } else {
                ad->thd_to_add = atoi(atts[1]);
            }
        } else if (0 == strcmp(*atts, "threads_max")) {
            if (0 == strcmp(atts[1], "default")) {
                ad->thd_max = -1;
            } else {
                ad->thd_max = atoi(atts[1]);
            }
        }
        atts+=2;
    }
}

static void handleAddressSpace(void *userdata, const XML_Char * cd, int len)
{
    Jxta_CacheConfigAdvertisement *ad = (Jxta_CacheConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;
    Jxta_addressSpace *jads = NULL;
    if (0 == len)
        return;

    jads = calloc(1, sizeof(Jxta_addressSpace));
    JXTA_OBJECT_INIT(jads, freeAddressSpace, 0);
    jads->ephemeral = FALSE;
    jads->isDefault = FALSE;
    jads->isDelta = FALSE;
    jads->DBEngine = jstring_new_2(AS_DBENGINE_DEFAULT);
    jads->DBName = jstring_new_2(AS_DBNAME_DEFAULT);
    while (atts && *atts) {
        if (0 == strcmp(*atts, "type")) {
            jads->ephemeral = 0 == strcmp(atts[1], "ephemeral");
        } else if (0 == strcmp(*atts, "highPerformance")) {
            jads->highPerformance = 0 == strcmp(atts[1], "yes");
        } else if (0 == strcmp(*atts, "autoVacuum")) {
            jads->autoVacuum = 0 == strcmp(atts[1], "yes");
        } else if (0 == strcmp(*atts, "default")) {
            jads->isDefault = 0 == strcmp(atts[1], "yes");
        } else if (0 == strcmp(*atts, "delta")) {
            jads->isDelta = 0 == strcmp(atts[1], "yes");
        } else if (0 == strcmp(*atts, "name")) {
            jads->name = jstring_new_2(atts[1]);
        } else if (0 == strcmp(*atts, "DBName")) {
            jstring_reset(jads->DBName, NULL);
            jstring_append_2(jads->DBName, atts[1]);
        } else if (0 == strcmp(*atts, "DBEngine")) {
            jstring_reset(jads->DBEngine, NULL);
            jstring_append_2(jads->DBEngine, atts[1]);
        } else if (0 == strcmp(*atts, "persistInterval")) {
            jads->persistInterval = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "persistName")) {
            jads->persistName = jstring_new_2(atts[1]);
        } else if (0 == strcmp(*atts, "QoS")) {
            jads->QoS = jstring_new_2(atts[1]);
        } else if (0 == strcmp(*atts, "location")) {
            jads->location = jstring_new_2(atts[1]);
        } else if (0 == strcmp(*atts, "connectString")) {
            jads->connectString = jstring_new_2(atts[1]);
        } else if (0 == strcmp(*atts, "xactionThreshold")) {
            jads->xactionThreshold = atoi(atts[1]);
        }
        atts += 2;
    }
    if (NULL == jads->name) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Address Space must have a unique name - ignored \n");
        JXTA_OBJECT_RELEASE(jads);
        return;
    }
    /* add this to the address space collection */
    if (jxta_cache_config_addr_is_default(jads) && NULL == ad->defaultAS) {
        ad->defaultAS = JXTA_OBJECT_SHARE(jads);
    }
    jxta_vector_add_object_last(ad->addressSpaces, (Jxta_object *) jads);
    JXTA_OBJECT_RELEASE(jads);
}


static void handleNameSpace(void *userdata, const XML_Char * cd, int len)
{
    Jxta_CacheConfigAdvertisement *ad = (Jxta_CacheConfigAdvertisement *) userdata;
    Jxta_nameSpace *jns = NULL;
    const char **atts = ((Jxta_advertisement *) ad)->atts;
    if (0 == len)
        return;
    jns = calloc(1, sizeof(Jxta_nameSpace));
    if (NULL == jns)
        return;
    JXTA_OBJECT_INIT(jns, freeNameSpace, 0);
    while (atts && *atts) {
        if (0 == strcmp(*atts, "addressSpace")) {
            jns->jAddressSpace = jstring_new_2(atts[1]);
        } else if (0 == strcmp(*atts, "adv")) {
            char attPrefix[64];
            const char *nsParse;
            int i = 0;
            nsParse = atts[1];
            jns->ns = jstring_new_2(atts[1]);
            while (':' != *nsParse) {
                attPrefix[i++] = *nsParse;
                attPrefix[i] = '\0';
                nsParse++;
            }
            jns->prefix = jstring_new_2(attPrefix);
            nsParse++;
            if ('*' == *nsParse)
                jns->type_is_wildcard = TRUE;
            jns->advType = jstring_new_2(nsParse);
        }
        atts += 2;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "att prefix - %s  att type - %s\n", jstring_get_string(jns->prefix),
                    jstring_get_string(jns->advType));
    jxta_vector_add_object_last(ad->nameSpaces, (Jxta_object *) jns);
    JXTA_OBJECT_RELEASE(jns);
}

JXTA_DECLARE(JString *) jxta_cache_config_addr_get_name(Jxta_addressSpace * jads)
{
    if (jads->name)
        JXTA_OBJECT_SHARE(jads->name);
    return jads->name;
}

JXTA_DECLARE(Jxta_vector *) jxta_cache_config_get_addressSpaces(Jxta_CacheConfigAdvertisement * adv)
{
    if (adv->addressSpaces)
        JXTA_OBJECT_SHARE(adv->addressSpaces);
    return adv->addressSpaces;
}

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_single_db(Jxta_CacheConfigAdvertisement * adv)
{
    return adv->sharedDB;
}

int cache_config_threads_needed(Jxta_CacheConfigAdvertisement * me)
{
    return (-1 == me->thd_to_add) ? 1 : me->thd_to_add;
}

int cache_config_threads_maximum(Jxta_CacheConfigAdvertisement * me)
{
    return (-1 == me->thd_max) ? 10 : me->thd_max;
}

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_ephemeral(Jxta_addressSpace * jads)
{
    return jads->ephemeral;
}

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_highPerformance(Jxta_addressSpace * jads)
{
    return jads->highPerformance;
}

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_auto_vacuum(Jxta_addressSpace * jads)
{
    return jads->autoVacuum;
}

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_default(Jxta_addressSpace * jads)
{
    return jads->isDefault;
}

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_delta(Jxta_addressSpace * jads)
{
    return jads->isDelta;
}

JXTA_DECLARE(JString *) jxta_cache_config_addr_get_DBName(Jxta_addressSpace * jads)
{
    if (jads->DBName)
        JXTA_OBJECT_SHARE(jads->DBName);
    return jads->DBName;
}

JXTA_DECLARE(JString *) jxta_cache_config_addr_get_DBEngine(Jxta_addressSpace * jads)
{
    if (jads->DBEngine)
        JXTA_OBJECT_SHARE(jads->DBEngine);
    return jads->DBEngine;
}

JXTA_DECLARE(Jxta_time_diff) jxta_cache_config_get_persistInterval(Jxta_addressSpace * jads)
{
    return jads->persistInterval;
}

JXTA_DECLARE(JString *) jxta_cache_config_addr_get_persistName(Jxta_addressSpace * jads)
{
    if (jads->persistName)
        JXTA_OBJECT_SHARE(jads->persistName);
    return jads->persistName;
}

JXTA_DECLARE(JString *) jxta_cache_config_addr_get_QoS(Jxta_addressSpace * jads)
{
    if (jads->QoS)
        JXTA_OBJECT_SHARE(jads->QoS);
    return jads->QoS;
}

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_local(Jxta_addressSpace * jads)
{
    return strcmp(jstring_get_string(jads->location), "local");
}

JXTA_DECLARE(JString *) jxta_cache_config_addr_get_connectString(Jxta_addressSpace * jads)
{
    if (jads->connectString)
        JXTA_OBJECT_SHARE(jads->connectString);
    return jads->connectString;
}

JXTA_DECLARE(int) jxta_cache_config_addr_get_xaction_threshold(Jxta_addressSpace * jads)
{
    return jads->xactionThreshold;
}

JXTA_DECLARE(Jxta_vector *) jxta_cache_config_get_nameSpaces(Jxta_CacheConfigAdvertisement * adv)
{
    unsigned int i;
    unsigned int j;
    Jxta_nameSpace *jns = NULL;
    /* stuff the address space in the proper name space structs */
    for (i = 0; i < jxta_vector_size(adv->nameSpaces); i++) {
        if (JXTA_SUCCESS != jxta_vector_get_object_at(adv->nameSpaces, JXTA_OBJECT_PPTR(&jns), i))
            continue;
        if (NULL != jns->addressSpace) {
            JXTA_OBJECT_RELEASE(jns);
            jns = NULL;
            continue;
        }
        for (j = 0; j < jxta_vector_size(adv->addressSpaces); j++) {
            Jxta_addressSpace *jas = NULL;
            if (JXTA_SUCCESS != jxta_vector_get_object_at(adv->addressSpaces, JXTA_OBJECT_PPTR(&jas), j))
                continue;
            if (!strcmp(jstring_get_string(jns->jAddressSpace), jstring_get_string(jas->name))) {
                if (NULL != jns->addressSpace) {
                    JXTA_OBJECT_RELEASE(jas);
                    break;
                }
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Found -- %s for %s\n", jstring_get_string(jas->name)
                                , jstring_get_string(jns->ns));
                jns->addressSpace = jas;
                break;
            }
            JXTA_OBJECT_RELEASE(jas);
        }
        if (jns)
            JXTA_OBJECT_RELEASE(jns);
    }
    return JXTA_OBJECT_SHARE(adv->nameSpaces);
}

JXTA_DECLARE(JString *) jxta_cache_config_ns_get_ns(Jxta_nameSpace * jns)
{
    if (jns->ns)
        JXTA_OBJECT_SHARE(jns->ns);
    return jns->ns;
}

JXTA_DECLARE(JString *) jxta_cache_config_ns_get_prefix(Jxta_nameSpace * jns)
{
    if (jns->prefix)
        JXTA_OBJECT_SHARE(jns->prefix);
    return jns->prefix;
}

JXTA_DECLARE(JString *) jxta_cache_config_ns_get_adv_type(Jxta_nameSpace * jns)
{
    if (jns->advType)
        JXTA_OBJECT_SHARE(jns->advType);
    return jns->advType;
}

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_ns_is_type_wildcard(Jxta_nameSpace * jns)
{
    return jns->type_is_wildcard;
}

JXTA_DECLARE(JString *) jxta_cache_config_ns_get_addressSpace_string(Jxta_nameSpace * jns)
{
    if (jns->jAddressSpace)
        JXTA_OBJECT_SHARE(jns->jAddressSpace);
    return jns->jAddressSpace;
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */

static const Kwdtab Jxta_CacheConfigAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:CacheConfig", Jxta_CacheConfigAdvertisement_, *handleJxta_CacheConfigAdvertisement, NULL, NULL},
    {"AddressSpace", AddressSpace_, *handleAddressSpace, NULL, NULL},
    {"NameSpace", NameSpace_, *handleNameSpace, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

static Jxta_status JXTA_STDCALL CacheConfigAdvertisement_get_xml(Jxta_CacheConfigAdvertisement * ad, JString ** result)
{
    char tmpbuf[256];
    unsigned int i;
    Jxta_vector *addSpaces;
    Jxta_vector *nameSpaces;
    JString *string = jstring_new_0();
    jstring_append_2(string, "<!-- JXTA Cache Configuration Advertisement -->\n");
    jstring_append_2(string, "<jxta:CacheConfig xmlns:jxta=\"http://jxta.org\" type=\"jxta:CacheConfig\"");
    jstring_append_2(string, "\n        sharedDB=\"");
    jstring_append_2(string, ad->sharedDB ? "yes" : "no");
    jstring_append_2(string, "\"");
    if (-1 != ad->thd_to_add) {
        apr_snprintf(tmpbuf, sizeof(tmpbuf), " threads=\"%d\"", ad->thd_to_add);
        jstring_append_2(string, tmpbuf);
    } else {
        jstring_append_2(string, " threads=\"default\"");
    }
    if (-1 != ad->thd_max) {
        apr_snprintf(tmpbuf, sizeof(tmpbuf), " threads_max=\"%d\"", ad->thd_max);
        jstring_append_2(string, tmpbuf);
    } else {
        jstring_append_2(string, " threads_max=\"default\"");
    }
    jstring_append_2(string,">\n");
    addSpaces = jxta_cache_config_get_addressSpaces(ad);
    for (i = 0; i < jxta_vector_size(addSpaces); i++) {
        Jxta_addressSpace *jads;
        if (JXTA_SUCCESS != jxta_vector_get_object_at(addSpaces, JXTA_OBJECT_PPTR(&jads), i)) {
            break;
        }
        jstring_append_2(string, "\n    <AddressSpace ");
        jstring_append_2(string, " name=\"");
        jstring_append_1(string, jads->name);
        jstring_append_2(string, "\"");
        jstring_append_2(string, "\n        type=\"");
        jstring_append_2(string, jads->ephemeral ? "ephemeral" : "persistent");
        jstring_append_2(string, "\"");
        jstring_append_2(string, "\n        highPerformance=\"");
        jstring_append_2(string, jads->highPerformance ? "yes" : "no");
        jstring_append_2(string, "\"");
        jstring_append_2(string, "\n        autoVacuum=\"");
        jstring_append_2(string, jads->autoVacuum ? "yes" : "no");
        jstring_append_2(string, "\"");
        jstring_append_2(string, "\n        default=\"");
        jstring_append_2(string, jads->isDefault ? "yes" : "no");
        jstring_append_2(string, "\"");
        jstring_append_2(string, "\n        delta=\"");
        jstring_append_2(string, jads->isDelta ? "yes" : "no");
        jstring_append_2(string, "\"");
        jstring_append_2(string, "\n        DBName=\"");
        jstring_append_1(string, jads->DBName);
        jstring_append_2(string, "\"");
        jstring_append_2(string, "\n        DBEngine=\"");
        jstring_append_1(string, jads->DBEngine);
        jstring_append_2(string, "\"");
        if (jads->persistInterval > 0) {
            jstring_append_2(string, "\n        persistInterval=\"");
            apr_snprintf(tmpbuf, sizeof(tmpbuf), "%" APR_INT64_T_FMT, jads->persistInterval);
            jstring_append_2(string, tmpbuf);
            jstring_append_2(string, "\"");
        }
        if (jads->persistName) {
            jstring_append_2(string, "\n        persistName=\"");
            jstring_append_1(string, jads->persistName);
            jstring_append_2(string, "\"");
        }
        if (jads->QoS) {
            jstring_append_2(string, "\n        QoS=\"");
            jstring_append_1(string, jads->QoS);
            jstring_append_2(string, "\"");
        }
        if (jads->location) {
            jstring_append_2(string, "\n        location=\"");
            jstring_append_1(string, jads->location);
            jstring_append_2(string, "\"");
        }
        if (jads->connectString) {
            jstring_append_2(string, "\n        connectString=\"");
            jstring_append_1(string, jads->connectString);
            jstring_append_2(string, "\"");
        }
        if (-1 != jads->xactionThreshold) {
            jstring_append_2(string, "\n        xactionThreshold=\"");
            apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", jads->xactionThreshold);
            jstring_append_2(string, tmpbuf);
            jstring_append_2(string, "\"");
        }
        jstring_append_2(string, "\n    />\n");
        JXTA_OBJECT_RELEASE(jads);
    }
    JXTA_OBJECT_RELEASE(addSpaces);
    nameSpaces = jxta_cache_config_get_nameSpaces(ad);
    for (i = 0; i < jxta_vector_size(nameSpaces); i++) {
        Jxta_nameSpace *jnsp = NULL;
        if (JXTA_SUCCESS != jxta_vector_get_object_at(nameSpaces, JXTA_OBJECT_PPTR(&jnsp), i)) {
            break;
        }
        jstring_append_2(string, "\n    <NameSpace adv=\"");
        jstring_append_1(string, jnsp->ns);
        jstring_append_2(string, "\"");
        jstring_append_2(string, "\n        addressSpace=\"");
        jstring_append_1(string, jnsp->jAddressSpace);
        jstring_append_2(string, "\"\n    />");
        JXTA_OBJECT_RELEASE(jnsp);
    }
    JXTA_OBJECT_RELEASE(nameSpaces);
    jstring_append_2(string, "\n</jxta:CacheConfig>\n");
    *result = string;
    return JXTA_SUCCESS;
}

Jxta_CacheConfigAdvertisement *jxta_CacheConfigAdvertisement_construct(Jxta_CacheConfigAdvertisement * self)
{
    self = (Jxta_CacheConfigAdvertisement *)
        jxta_advertisement_construct((Jxta_advertisement *) self,
                                     "jxta:CacheConfig",
                                     Jxta_CacheConfigAdvertisement_tags,
                                     (JxtaAdvertisementGetXMLFunc) CacheConfigAdvertisement_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != self) {
        self->addressSpaces = jxta_vector_new(1);
        self->nameSpaces = jxta_vector_new(1);
    }

    return self;
}

void jxta_CacheConfigAdvertisement_destruct(Jxta_CacheConfigAdvertisement * self)
{
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Free Cache Config Advertisement\n");
    if (NULL != self->addressSpaces)
        JXTA_OBJECT_RELEASE(self->addressSpaces);
    if (NULL != self->nameSpaces)
        JXTA_OBJECT_RELEASE(self->nameSpaces);
    if (NULL != self->defaultAS)
        JXTA_OBJECT_RELEASE(self->defaultAS);
    jxta_advertisement_destruct((Jxta_advertisement *) self);

}

/** 
 *   Get a new instance of the ad. 
 **/
JXTA_DECLARE(Jxta_CacheConfigAdvertisement *) jxta_CacheConfigAdvertisement_new(void)
{
    Jxta_CacheConfigAdvertisement *ad = (Jxta_CacheConfigAdvertisement *) calloc(1, sizeof(Jxta_CacheConfigAdvertisement));

    JXTA_OBJECT_INIT(ad, jxta_CacheConfigAdvertisement_delete, 0);

    return jxta_CacheConfigAdvertisement_construct(ad);
}

JXTA_DECLARE(Jxta_status) jxta_CacheConfigAdvertisement_create_default(Jxta_CacheConfigAdvertisement * adv, Jxta_boolean legacy)
{
    Jxta_status status;
    Jxta_addressSpace *ads = NULL;
    Jxta_nameSpace *nsp = NULL;
    adv->sharedDB = FALSE;
    adv->thd_to_add = -1;
    adv->thd_max = -1;
    ads = calloc(1, sizeof(Jxta_addressSpace));
    JXTA_OBJECT_INIT(ads, freeAddressSpace, 0);
    ads->name = jstring_new_2("defaultAddressSpace");
    ads->ephemeral = FALSE;
    ads->highPerformance = FALSE;
    ads->autoVacuum = FALSE;
    ads->isDefault = TRUE;
    ads->isDelta = TRUE;
    ads->DBName = jstring_new_2("groupID");
    ads->DBEngine = jstring_new_2("sqlite3");
    ads->persistInterval = 0;
    ads->persistName = NULL;
    ads->QoS = NULL;
    ads->location = jstring_new_2("local");
    ads->connectString = NULL;
    ads->xactionThreshold = XACTION_THRESHOLD_DEFAULT;
    status = jxta_vector_add_object_last(adv->addressSpaces, (Jxta_object *) ads);
    JXTA_OBJECT_RELEASE(ads);

    if (!legacy) {
        ads = calloc(1, sizeof(Jxta_addressSpace));
        JXTA_OBJECT_INIT(ads, freeAddressSpace, 0);
        ads->name = jstring_new_2("highPerformanceAddressSpace");
        ads->ephemeral = FALSE;
        ads->highPerformance = TRUE;
        ads->isDefault = FALSE;
        ads->DBName = jstring_new_2("highPerf");
        ads->DBEngine = jstring_new_2("sqlite3");
        ads->persistInterval = 0;
        ads->persistName = NULL;
        ads->QoS = NULL;
        ads->location = jstring_new_2("local");
        ads->connectString = NULL;
        ads->xactionThreshold = XACTION_THRESHOLD_DEFAULT;
        status = jxta_vector_add_object_last(adv->addressSpaces, (Jxta_object *) ads);
        JXTA_OBJECT_RELEASE(ads);

        ads = calloc(1, sizeof(Jxta_addressSpace));
        JXTA_OBJECT_INIT(ads, freeAddressSpace, 0);
        ads->name = jstring_new_2("ephemeralAddressSpace");
        ads->ephemeral = TRUE;
        ads->highPerformance = TRUE;
        ads->isDefault = FALSE;
        ads->DBName = jstring_new_2("groupID");
        ads->DBEngine = jstring_new_2("sqlite3");
        ads->persistInterval = 0;
        ads->persistName = NULL;
        ads->QoS = NULL;
        ads->location = jstring_new_2("local");
        ads->connectString = NULL;
        ads->xactionThreshold = XACTION_THRESHOLD_DEFAULT;
        status = jxta_vector_add_object_last(adv->addressSpaces, (Jxta_object *) ads);
        JXTA_OBJECT_RELEASE(ads);

    }
    nsp = calloc(1, sizeof(Jxta_nameSpace));
    JXTA_OBJECT_INIT(nsp, freeNameSpace, 0);
    nsp->ns = jstring_new_2("jxta:*");
    nsp->prefix = jstring_new_2("jxta");
    nsp->advType = jstring_new_2("*");
    nsp->type_is_wildcard = TRUE;
    nsp->jAddressSpace = jstring_new_2("defaultAddressSpace");
    status = jxta_vector_add_object_last(adv->nameSpaces, (Jxta_object *) nsp);
    JXTA_OBJECT_RELEASE(nsp);

    nsp = calloc(1, sizeof(Jxta_nameSpace));
    JXTA_OBJECT_INIT(nsp, freeNameSpace, 0);
    nsp->ns = jstring_new_2("jxta:RA");
    nsp->prefix = jstring_new_2("jxta");
    nsp->advType = jstring_new_2("RA");
    nsp->type_is_wildcard = FALSE;
    nsp->jAddressSpace = jstring_new_2("ephemeralAddressSpace");
    status = jxta_vector_add_object_last(adv->nameSpaces, (Jxta_object *) nsp);
    JXTA_OBJECT_RELEASE(nsp);

    nsp = calloc(1, sizeof(Jxta_nameSpace));
    JXTA_OBJECT_INIT(nsp, freeNameSpace, 0);
    nsp->ns = jstring_new_2("jxta:PipeAdvertisement");
    nsp->prefix = jstring_new_2("jxta");
    nsp->advType = jstring_new_2("PipeAdvertisement");
    nsp->type_is_wildcard = FALSE;
    nsp->jAddressSpace = jstring_new_2("highPerformanceAddressSpace");
    status = jxta_vector_add_object_last(adv->nameSpaces, (Jxta_object *) nsp);
    JXTA_OBJECT_RELEASE(nsp);

    nsp = calloc(1, sizeof(Jxta_nameSpace));
    JXTA_OBJECT_INIT(nsp, freeNameSpace, 0);
    nsp->ns = jstring_new_2("demo:TestAdvertisement");
    nsp->prefix = jstring_new_2("demo");
    nsp->advType = jstring_new_2("TestAdvertisement");
    nsp->type_is_wildcard = FALSE;
    nsp->jAddressSpace = jstring_new_2("highPerformanceAddressSpace");
    status = jxta_vector_add_object_last(adv->nameSpaces, (Jxta_object *) nsp);
    JXTA_OBJECT_RELEASE(nsp);

    return status;
}

static void jxta_CacheConfigAdvertisement_delete(Jxta_object * ad)
{
    jxta_CacheConfigAdvertisement_destruct((Jxta_CacheConfigAdvertisement *) ad);

    memset(ad, 0xDD, sizeof(Jxta_CacheConfigAdvertisement));
    free(ad);
}

/* vim: set ts=4 sw=4 et tw=130: */
