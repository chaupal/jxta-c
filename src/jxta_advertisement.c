/* 
 * Copyright (c) 2001-2006 Sun Microsystems, Inc.  All rights reserved.
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
static const char *__log_cat = "ADV";


#include "jxta_log.h"
#include "jxta_errno.h"
#include "jxta_advertisement.h"
#include "jxta_apr.h"
#include "jxta_mia.h"
#include "jxta_pa.h"
#include "jxta_pga.h"
#include "jxta_test_adv.h"
#include "jxta_pipe_adv.h"
#include "jxta_msa.h"
#include "jxta_mca.h"
#include "jxta_tta.h"
#include "jxta_hta.h"
#include "jxta_rq.h"
#include "jxta_dq.h"
#include "jxta_rr.h"
#include "jxta_discovery_config_adv.h"
#include "jxta_endpoint_config_adv.h"
#include "jxta_rdv_config_adv.h"
#include "jxta_srdi_config_adv.h"
#include "jxta_rdv_lease_options.h"
#include "jxta_peerview_monitor_entry.h"
#include "jxta_peerview_option_entry.h"
#include "jxta_rdv_monitor_entry.h"
#include "jxta_relaya.h"
#include "jxta_piperesolver_msg.h"

#define ENHANCED_QUERY_LOG "QueryLog"

#define MAX_ADV_DEPTH 32


typedef struct _jxta_advertisement_new_func Jxta_advertisement_new_func;

/** 
 * Allowing the extensibility of the jxta protocols
 * discovery architecture is enabled by creating delayed
 * execution handlers.  This way, each piece of the 
 * protocol can be handled in a customized way, precisely
 * when and where necessary, not sooner, not later.  This is
 * accomplished by hashing the methods for creating 
 * advertisement handlers, and only invoking these when 
 * it is necessary to transform the advertisement serialized
 * in XML to its representation in memory.  
 */
struct _jxta_advertisement_new_func {
    JXTA_OBJECT_HANDLE;
    JxtaAdvertisementNewFunc jxta_advertisement_new_func;
};

static Jxta_advertisement_new_func *advertisement_new_func_new(const JxtaAdvertisementNewFunc new_func);
static void advertisement_new_func_delete(Jxta_object * obj);
static const char ** advertisement_get_tagnames_1(Jxta_advertisement * ad, Jxta_boolean check_replication);


/* static apr_pool_t * global_ad_pool; */
/**
*   This is a "global" registry for advertisement handlers.
*/
static Jxta_hashtable *global_ad_table = NULL;

/**
*   This is a "global" table for element/attributes that are replicated
*/
static Jxta_hashtable *global_ad_non_replication_table = NULL;

static void advertisement_handle_chardata(void *me, const XML_Char * cd, int len);
static void advertisement_start_element(void *me, const char *ename, const char **atts);
static void advertisement_end_element(void *me, const char *name);

static void advertisement_free(Jxta_object * me);

static JString * advertisement_get_namespace_prefix(const char *namespace);


JXTA_DECLARE(Jxta_advertisement *) jxta_advertisement_initialize(Jxta_advertisement * ad,
                              const char *document_name,
                              const Kwdtab * dispatch_table,
                              JxtaAdvertisementGetXMLFunc get_xml_func,
                              JxtaAdvertisementGetIDFunc get_id_func,
                              JxtaAdvertisementGetIndexFunc get_index_func, 
                              FreeFunc free_func)
{
    JXTA_OBJECT_INIT(ad, (JXTA_OBJECT_FREE_FUNC) free_func, 0);

    return jxta_advertisement_construct(ad, document_name, dispatch_table, get_xml_func, get_id_func, get_index_func);
}

JXTA_DECLARE(Jxta_advertisement *) jxta_advertisement_construct(Jxta_advertisement * ad,
                                                                const char *document_name,
                                                                const Kwdtab * dispatch_table,
                                                                JxtaAdvertisementGetXMLFunc get_xml_func,
                                                                JxtaAdvertisementGetIDFunc get_id_func,
                                                                JxtaAdvertisementGetIndexFunc get_index_func)
{
    ad->document_name = document_name;
    ad->dispatch_table = (Kwdtab *) dispatch_table;
    ad->get_xml = get_xml_func;
    ad->get_id = get_id_func;
    ad->get_indexes = get_index_func;
    ad->local_id = NULL;
    ad->adv_list = jxta_vector_new(0);
    
    /* Active during parsing */
    ad->parent = NULL;
    ad->parser = NULL;
    ad->handler_stk = NULL;
    ad->depth = 0;
    ad->curr_handler = NULL;
    ad->atts = NULL;
    ad->accum = NULL;
    
    return ad;
}

JXTA_DECLARE(void) jxta_advertisement_destruct(Jxta_advertisement * ad)
{
    if (ad->accum != NULL) {
        JXTA_OBJECT_RELEASE(ad->accum);
    }

    if (ad->adv_list != NULL) {
        JXTA_OBJECT_RELEASE(ad->adv_list);
    }

    if (ad->handler_stk != NULL)
        free(ad->handler_stk);

    if (ad->local_id != NULL) {
        free(ad->local_id);
    }
}

void jxta_advertisement_delete(Jxta_advertisement * ad)
{
    jxta_advertisement_destruct(ad);

    /* FIXME 20050406 This function is commonly, but incorrectly used by many advs. */
    /* free(ad); */
}

JXTA_DECLARE(void) jxta_advertisement_set_handlers(Jxta_advertisement * ad, XML_Parser parser, Jxta_advertisement * parent)
{    
    ad->parent = parent;
    ad->parser = parser;

    XML_SetUserData(ad->parser, ad);

    if( NULL == parent ) {
        XML_SetElementHandler(ad->parser, advertisement_start_element, advertisement_end_element);
        XML_SetCharacterDataHandler(ad->parser, advertisement_handle_chardata);
        XML_SetDefaultHandlerExpand(ad->parser, advertisement_handle_chardata);
    }

    ad->handler_stk = (Jxta_advertisement_node_handler *) calloc(MAX_ADV_DEPTH, sizeof(Jxta_advertisement_node_handler));
    ad->depth = 0;
    
    ad->curr_handler = NULL;
    
    ad->accum = jstring_new_0();

    if( NULL != parent ) {
        advertisement_start_element( ad, ad->document_name, parent->atts );
    } else {
        ad->atts = NULL;
    }
}

JXTA_DECLARE(void) jxta_advertisement_clear_handlers(Jxta_advertisement * ad )
{
    XML_SetUserData(ad->parser, ad->parent);
    
    ad->parent = NULL;
    ad->parser = NULL;
    
    free( ad->handler_stk );
    ad->handler_stk = NULL;
    ad->depth = 0;

    ad->atts = NULL;

    JXTA_OBJECT_RELEASE( ad->accum );
    ad->accum = NULL;
}

static void advertisement_handle_chardata(void *me, const XML_Char * cd, int len)
{
    Jxta_advertisement *ad = (Jxta_advertisement *) me;

    if( NULL == me ) {
        return;
    }

    JXTA_OBJECT_CHECK_VALID(ad);

    jstring_append_0(ad->accum, cd, len);
}

JXTA_DECLARE(const char *) jxta_advertisement_get_document_name(Jxta_advertisement * ad)
{
    return ad->document_name;
}

JXTA_DECLARE(char *) jxta_advertisement_get_string(Jxta_advertisement * ad, Jxta_index * ji)
{
    return jxta_advertisement_get_string_1(ad, ji, NULL);
}

JXTA_DECLARE(char *) jxta_advertisement_get_string_1(Jxta_advertisement * ad, Jxta_index * ji, Jxta_kwd_entry_flags * flags)
{
    int i, j, m;
    const Kwdtab *tags_table = ad->dispatch_table;
    char *full_index_name;
    char *element;
    const char *attrib;
    const char *parm;
    int len;

    element = (char *) jstring_get_string(ji->element);
    if (tags_table == NULL) {
        return NULL;
    }
    if (ji->attribute != NULL) {
        attrib = (char *) jstring_get_string(ji->attribute);
        len = strlen(attrib) + strlen(element) + 2;
        full_index_name = (char *) calloc(1, len);
        apr_snprintf(full_index_name, len, "%s %s", element, attrib);
    } else {
        len = strlen(element) + 1;
        full_index_name = (char *) calloc(1, len);
        apr_snprintf(full_index_name, len, "%s", element);
    }
    j = -1;
    m = -1;
    i = 0;
    while (tags_table[i].kwd) {
        if ((!strcmp(tags_table[i].kwd, full_index_name))) {
            j = i;
            break;
        } else if (!strcmp(tags_table[i].kwd, element)) {
            /* save it in case there is no exact match */
            m = i;
        }
        i++;
    }
    free(full_index_name);
    if (j == -1) {
        if (m == -1)
            return NULL;
        i = m;
    }
    if (NULL != flags) *flags = tags_table[i].tokentype;
    if (tags_table[i].get != NULL) {
        return tags_table[i].get(ad);
    } else if (tags_table[i].get_parm != NULL) {
        char *tmp;
        parm = ji->parm;
        tmp = tags_table[i].get_parm(ad, parm);
        return tmp;
    }
    return NULL;
}

/* Free just the initial pointer, i.e., free(tags);
 * The actual data is const and will be freed when the 
 * ad is freed.  Anyone with a better way is invited
 * to change it.
 */
JXTA_DECLARE(const char **) jxta_advertisement_get_tagnames(Jxta_advertisement * ad)
{
    return advertisement_get_tagnames_1(ad, FALSE);
}

static const char ** advertisement_get_tagnames_1(Jxta_advertisement * ad, Jxta_boolean check_replication)
{
    const Kwdtab *tags_table = ad->dispatch_table;
    int i = 1;                  /* The first entry is not interresting */
    int numtags = 1;            /* There's always at least the final NULL ptr. */
    const char **tags;

    if (tags_table == NULL)
        return NULL;

    while (tags_table[i].kwd != NULL) {
        /* Do not count those that do not have get_xxx_string support */
        if (tags_table[i].get != NULL || tags_table[i].get_parm != NULL) {
            ++numtags;
        }
        i++;
    }

    tags = (const char **) calloc(numtags, sizeof(const char *));

    numtags = 0;
    i = 1;
    while (tags_table[i].kwd != NULL) {
        /* Do not return those that do not have get_xxx_string support */
        if (tags_table[i].get != NULL || tags_table[i].get_parm) {
            /* if checking replication return non replicated tags */
            if (check_replication) {
                if ((tags_table[i].tokentype & NO_REPLICATION)) {
                    tags[numtags++] = tags_table[i].kwd;
                }
            } else {
                tags[numtags++] = tags_table[i].kwd;
            }
        }
        ++i;
    }
    tags[numtags] = NULL;

    return tags;
}

static void log_error(XML_Parser parser, const char *document_name)
{
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, 
             "Error Parsing %s : %s at line %d:%d (offset=%d,%d)\n",
             document_name,
             XML_ErrorString(XML_GetErrorCode(parser)),
             XML_GetCurrentLineNumber(parser),
             XML_GetCurrentColumnNumber(parser), 
             XML_GetCurrentByteIndex(parser), 
             XML_GetCurrentByteCount(parser));
}

JXTA_DECLARE(Jxta_status) jxta_advertisement_parse_charbuffer(Jxta_advertisement * ad, const char *buf, size_t len)
{
    Jxta_status res=JXTA_SUCCESS;
    size_t offset = 0;
    XML_Parser parser;
 
    if (!buf || !len) {
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Parsing %s [%pp] from char buffer.\n", ad->document_name, ad );
 
    parser = XML_ParserCreate(NULL);

    jxta_advertisement_set_handlers(ad, parser, (Jxta_advertisement *) NULL);

    while (offset < len) {
        enum XML_Status xml_result = XML_Parse(parser, buf + offset, len - offset, 1);
        
        offset += XML_GetCurrentByteIndex(parser);        

        if( XML_STATUS_OK == xml_result ) {
            res = JXTA_SUCCESS;
            break;
        } else if( XML_STATUS_ERROR == xml_result ) {
        if (XML_GetErrorCode(parser) != XML_ERROR_JUNK_AFTER_DOC_ELEMENT) {
            log_error(parser, ad->document_name);
                res = JXTA_INVALID_ARGUMENT;
                break;
            } else {
                /* Reset the parser for another advertisement */
                XML_ParserReset( parser, NULL );
                continue;
            }
        }
        }

        XML_ParserFree(parser);

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_advertisement_parse_file(Jxta_advertisement * ad, FILE * instream)
{
    char buf[BUFSIZ];           /*  8192 */
    Jxta_status res=JXTA_SUCCESS;
    int done;
    XML_Parser parser;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Parsing [%pp] from file.\n", ad );
    
    parser = XML_ParserCreate(NULL);
    
    jxta_advertisement_set_handlers(ad, parser, (Jxta_advertisement *) NULL);

    do {
        size_t offset = 0;
        size_t len = fread(buf, sizeof(char), BUFSIZ, instream);
        done = len < BUFSIZ ? 1 : 0;

        while (offset < len) {
            enum XML_Status xml_result = XML_Parse(parser, buf + offset, len - offset, done);

            offset += XML_GetCurrentByteIndex(parser);
            
            if( XML_STATUS_OK == xml_result ) {
                res = JXTA_SUCCESS;
                break;
            } else if( XML_STATUS_ERROR == xml_result ) {
                if (XML_GetErrorCode(parser) != XML_ERROR_JUNK_AFTER_DOC_ELEMENT) {
            log_error(parser, ad->document_name);
                    res = JXTA_INVALID_ARGUMENT;
                    break;
                } else {
                    /* Reset the parser for another advertisement */
                    XML_ParserReset( parser, NULL );
                }
            }
        }
    } while (!done);

    XML_ParserFree(parser);

    return res;
}

static Jxta_advertisement_new_func *advertisement_new_func_new(const JxtaAdvertisementNewFunc new_func)
{
    Jxta_advertisement_new_func *myself = (Jxta_advertisement_new_func *) calloc(1, sizeof(Jxta_advertisement_new_func));
    
    if( NULL == myself ) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, advertisement_new_func_delete, 0);

    myself->jxta_advertisement_new_func = new_func;

    return myself;
}

static void advertisement_new_func_delete(Jxta_object * me)
{
    Jxta_advertisement_new_func * myself = (Jxta_advertisement_new_func*) me;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In jxta_ad_new_func_delete\n");

    memset( myself, 0xDD, sizeof(Jxta_advertisement_new_func)); 

    free(myself);
}

/** 
 * @param key const char * for hash key
 *
 * @param ad_new_function Advertisement initialization function: instead of the 
 *        hash value being an initialized jxta_advertisement, its better
 *        to have the value be a pointer to an initialization function
 *        that will return the advertisement.  That way every time 
 *        the parser hits the keyword, it will check here. If that 
 *        tagname is registered, then the function pointer for 
 *        initializing that kind of advertisement is returned from 
 *        the hash table and a new object of that kind of advertisement
 *        is returned from here.  Probably have to return it as a
 *        jxta_advertisement *, and cast it in the calling function.
 */
JXTA_DECLARE(void) jxta_advertisement_register_global_handler(const char *doc_type, const JxtaAdvertisementNewFunc ad_new_function)
{
    Jxta_advertisement_new_func *new_func = advertisement_new_func_new(ad_new_function);
    Jxta_advertisement *new_ad = NULL;
    const char **tags;
    const char **tags_save;
    JString *jPrefix = NULL;
    Jxta_hashtable *non_replicated_entries = NULL;
    jxta_hashtable_put(global_ad_table, doc_type, strlen(doc_type) + 1, (Jxta_object *) new_func);

    JXTA_OBJECT_RELEASE(new_func);

    new_ad = (Jxta_advertisement *) (*ad_new_function) ();

    if( NULL == new_ad ) {
        return;
    }

    tags = advertisement_get_tagnames_1(new_ad, TRUE);
    if (*tags) {
        jPrefix = advertisement_get_namespace_prefix(doc_type);
        if (NULL != jPrefix && jstring_length(jPrefix) > 0) {
            if (JXTA_SUCCESS != jxta_hashtable_get(global_ad_non_replication_table, jstring_get_string(jPrefix),
                                                   jstring_length(jPrefix), JXTA_OBJECT_PPTR(&non_replicated_entries))) {
                non_replicated_entries = jxta_hashtable_new(32);
                jxta_hashtable_put(global_ad_non_replication_table, jstring_get_string(jPrefix)
                                   , jstring_length(jPrefix), (Jxta_object *) non_replicated_entries);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Created new non-replicated entries table for namespace prefix%s\n", 
                                jstring_get_string(jPrefix));
            }
        }
    }
    
    tags_save = tags;
    while (*tags) {
        JString *non_replicated_tag;
        Jxta_boolean putStatus = FALSE;
        non_replicated_tag = jstring_new_0();
        jstring_append_2(non_replicated_tag, *tags);
        putStatus = jxta_hashtable_putnoreplace(non_replicated_entries, jstring_get_string(non_replicated_tag)
                            , jstring_length(non_replicated_tag), (Jxta_object *) non_replicated_tag);
        if (TRUE == putStatus)
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Added tag: %s to non-replicated namespace: %s\n", 
                            *tags, jstring_get_string(jPrefix));
        tags++;

        JXTA_OBJECT_RELEASE(non_replicated_tag);
    }
    free((void *) tags_save);

    JXTA_OBJECT_RELEASE(new_ad);
    if (jPrefix)
        JXTA_OBJECT_RELEASE(jPrefix);
    if (non_replicated_entries)
        JXTA_OBJECT_RELEASE(non_replicated_entries);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "jxta_advertisement_global_register : registered : %s\n", doc_type);
}

JXTA_DECLARE(Jxta_boolean) jxta_advertisement_is_element_replicated(const char *ns, const char *elem_attr)
{
    Jxta_hashtable *non_replicated_entries = NULL;
    Jxta_boolean res = TRUE;
    JString *jPrefix = advertisement_get_namespace_prefix(ns);
    jxta_hashtable_get(global_ad_non_replication_table, jstring_get_string(jPrefix), jstring_length(jPrefix),
                       JXTA_OBJECT_PPTR(&non_replicated_entries));
    if (NULL != non_replicated_entries) {
        res = (JXTA_SUCCESS == jxta_hashtable_contains(non_replicated_entries, elem_attr, strlen(elem_attr))) ? FALSE: TRUE;
        JXTA_OBJECT_RELEASE(non_replicated_entries);
    }
    if (jPrefix)
        JXTA_OBJECT_RELEASE(jPrefix);
    return res;
}

static JString * advertisement_get_namespace_prefix(const char *namespace)
{
    int i = 0;
    JString *jPrefix = NULL;
    for (i = 0; namespace[i] != '\0' && namespace[i] != ':'; i++);
    if (':' == namespace[i]) {
        jPrefix = jstring_new_0();
        jstring_append_0(jPrefix, namespace, i);
    }

    return jPrefix;
}

JXTA_DECLARE(Jxta_status) jxta_advertisement_global_handler(Jxta_advertisement * ad, const char *doc_type, Jxta_advertisement** new_ad)
{
    Jxta_status res;
    Jxta_advertisement_new_func *new_func = NULL;

    res = jxta_hashtable_get(global_ad_table, doc_type, strlen(doc_type) + 1, JXTA_OBJECT_PPTR(&new_func));

    if( (JXTA_SUCCESS == res) && (new_func != NULL) ) {
        JxtaAdvertisementNewFunc ad_new_function = new_func->jxta_advertisement_new_func;

        JXTA_OBJECT_RELEASE(new_func);

        /* Do what a subdocument handler would do: */

        /* create the sub-ad */
        *new_ad = (Jxta_advertisement *) (*ad_new_function) ();

        if( NULL == *new_ad ) {
            return JXTA_NOMEM;
        }

        /*
         * then return to the back end engine
         * (let it know that we found a handler)
         */
        res = JXTA_SUCCESS;
    } else {
        /* let the back end know that no handler was found */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "No Advertisement handler for : %s \n", doc_type);
        res = JXTA_ITEM_NOTFOUND;
    }

    return res;
}

/*
 * @todo jice@jxta.org - 20020204 : Nothing controls that this routine
 * is not called while the global hash is in use by another thread.
 * That may be a correct assumption but we need to define when it is
 * correct to call this routine.
 */
void jxta_advertisement_cleanup(void)
{
    if (global_ad_table != NULL) {
        JXTA_OBJECT_RELEASE(global_ad_table);
    }
    if (global_ad_non_replication_table != NULL) {
        JXTA_OBJECT_RELEASE(global_ad_non_replication_table);
    }
}

static void advertisement_start_element(void *me, const char *ename, const char **atts)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_advertisement *ad = (Jxta_advertisement *) me;

    const char **att;
    const char *typeAttr = NULL;
    int each_element_key;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->depth >= MAX_ADV_DEPTH) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Advertisement too deep. Parsing fails.\n");
        /* Keep counting so that we stay in sync. */
        ++(ad->depth);
        return;
    }
    
    ad->handler_stk[ad->depth++] = ad->curr_handler;
    ad->atts = atts;
    ad->currElement = ename;

    att = atts;
    
    /* Find the "type" attribute, if present */
    while( *att ) {
        if( 0 == strcmp( *att, "type" ) ) {
            typeAttr = *(att + 1);
            break;
        }
        
        att += 2;
    }
    
    if ( NULL == ad->dispatch_table ) {
         /* IF, and only IF, we do not have a tags_table, THEN we're a base class
         * adv handling an unknown batch of documents. Then, we look in the global
         * hash table, for the handler; NOT the tags_table.
         */
        Jxta_advertisement *new_ad = NULL;

        /*
         * If we got a type attribute, try and match it first.
         */
        if (typeAttr != NULL) {
            /* Try the type attribute name first */
            res = jxta_advertisement_global_handler(ad, typeAttr, &new_ad);
        }

        if( NULL == new_ad ) {
            /* try the element name */
            res = jxta_advertisement_global_handler(ad, ename, &new_ad);
        }

        if( res == JXTA_SUCCESS ) {
            /* set new handlers */
            jxta_advertisement_set_handlers(new_ad, ad->parser, ad);

            /* hook it into our list of advs */
            jxta_vector_add_object_last(ad->adv_list, (Jxta_object *) new_ad);
            JXTA_OBJECT_RELEASE(new_ad);
        }

        /* we've done all we could, no matter if a handler was found */
        return;
    }

        att = atts;
        
    /* process attribute handlers */

        while (*att != NULL) {
            /* search for this attribute */
            int meta_tag_len = strlen(ename) + strlen(" ") + strlen(*att) + 4;
            char *meta_tag = (char *) calloc(1, meta_tag_len);
        unsigned int each_kwd;
            
            apr_snprintf(meta_tag, meta_tag_len, "%s %s", ename, *att);

            for(each_kwd = 0; ad->dispatch_table[each_kwd].kwd; each_kwd++ ) {
                if ( 0 == strcmp(ad->dispatch_table[each_kwd].kwd, meta_tag)) {
                    if (ad->dispatch_table[each_kwd].nodeparse != NULL) {
                        char const * attr_value = *(att + 1);
                        
                    ad->dispatch_table[each_kwd].nodeparse(me, attr_value, strlen(attr_value));
                    }
                    break;
                }
            }
            
            free(meta_tag);

            att += 2;
        }

    /* process the element via handlers */ 
    for( each_element_key = 0; each_element_key < 3; each_element_key++ ) {
        const char *element_key=NULL;
        unsigned int each_kwd;

        switch( each_element_key ) {
            case 0 : 
                /* try to find a handler based on type attribute */
                if( NULL != typeAttr ) {
                    element_key = typeAttr;
                } else {
                    /* next! */
                    continue;
    }
            break;

            case 1 :
                /* try to find a handler based on element name */
                element_key = ename;
            break;

            case 2 :
                /* try to find a global handler */
                {
                    Jxta_advertisement *new_ad = NULL;

    /*
     * If we got a type attribute, try and match it first.
     */
    if (typeAttr != NULL) {
                        /* Try the type attribute name first */
                        res = jxta_advertisement_global_handler(ad, typeAttr, &new_ad);
                    }

                    if( NULL == new_ad ) {
                        /* try the element name */
                        res = jxta_advertisement_global_handler(ad, ename, &new_ad);
                    }

                    if( res == JXTA_SUCCESS ) {
                        /* set new handlers */
                        jxta_advertisement_set_handlers(new_ad, ad->parser, ad);

                        /* hook it into our list of advs */
                        jxta_vector_add_object_last(ad->adv_list, (Jxta_object *) new_ad);
                        JXTA_OBJECT_RELEASE(new_ad);
                        return;
                }
            }
            continue;                
    }

    for (each_kwd = 0; ad->dispatch_table[each_kwd].kwd; each_kwd++ ) {
            if ( 0 == strcmp(ad->dispatch_table[each_kwd].kwd, element_key)) {
                ad->curr_handler = ad->dispatch_table[each_kwd].nodeparse;

                /* Call the handler exactly once at start, with a zero-length
                 * string. Existing handlers are already designed to cope with
                 * it. Handlers will ignore it except if the tag marks a complex
                 * sub-element, in which case the handler *needs* to be called
                 * at start to switch user-data. We could let the application
                 * code supply a different handler for start and end, but all
                 * advertisement is already written with a single handler
                 * that's called any number of times. This change just makes the
                 * number of calls predictable, thus allowing the reliable
                 * processing of repeat tags without breaking existing
                 * advertisement code.
                */
               jstring_reset(ad->accum, NULL);
               ad->curr_handler(me, "", 0);

                return;
            }
        }
    }

    ad->curr_handler = NULL;

    /* Include this start element in the accumulator */
    XML_DefaultCurrent(ad->parser);
    
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Unhandled element during <%s> [%pp] parse : %s.\n", ad->document_name, me, ename );
}

/**
*   Complete the element by calling the current handler with the
*   accumulated character data.
    */
static void advertisement_end_element(void *me, const char *ename)
{
    Jxta_advertisement *ad = (Jxta_advertisement *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    ad->depth--;

    if ( ad->depth >= MAX_ADV_DEPTH) {
                return;
    }
    
    if( NULL != ad->curr_handler ) {
        if( 0 == jstring_length(ad->accum) ) {
            /* XXX 20060830 bondolo Because this is such a strange parsing system we have to make sure that there's at least one character at the end. */
            jstring_append_2( ad->accum, " " );
        }
        ad->curr_handler(ad, jstring_get_string(ad->accum), jstring_length(ad->accum) );
        jstring_reset(ad->accum, NULL);
    } else {
        /* Include this end element in the accumulator */
        XML_DefaultCurrent(ad->parser);
    }

    ad->curr_handler = ad->handler_stk[ad->depth];

    if (ad->depth == 0) {
    /* If at the end of a subdocument (any element with subelements is
         * handled as a subdocument), reset the me 
     * for the parents dispatch table.
     */
        Jxta_advertisement *parent = ad->parent;

        jxta_advertisement_clear_handlers(ad);

        if( NULL != parent ) {
            /* The parent has to handle the element too. */
            advertisement_end_element( parent, ename );
        }
    }
}

JXTA_DECLARE(Jxta_status) jxta_advertisement_get_xml(Jxta_advertisement * ad, JString ** js)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->get_xml != NULL) {
        return ad->get_xml(ad, js);
    } else {
        return JXTA_NOTIMP;
    }
}

JXTA_DECLARE(Jxta_id *) jxta_advertisement_get_id(Jxta_advertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->get_id != NULL) {
        return ad->get_id(ad);
    }
    return NULL;
}

JXTA_DECLARE(const char *) jxta_advertisement_get_local_id_string(Jxta_advertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->local_id;
}

JXTA_DECLARE(void) jxta_advertisement_set_local_id_string(Jxta_advertisement * ad, const char *local_id)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->local_id) {
        free(ad->local_id);
        ad->local_id = NULL;
    }
        
    if( NULL != local_id ) {
        ad->local_id = strdup(local_id);
    }
}

JXTA_DECLARE(Jxta_vector *) jxta_advertisement_get_indexes(Jxta_advertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->get_indexes != NULL) {
        return ad->get_indexes(ad);
    }
    return NULL;
}

/**
*   Used instead of "delete" because delete is unfortunately public.
*/
static void advertisement_free(Jxta_object * me)
{
    Jxta_advertisement* myself = (Jxta_advertisement*) me;
    
    jxta_advertisement_destruct(myself);
    
    memset( myself, 0xDD, sizeof(Jxta_advertisement) );
    
    free(myself);
}

JXTA_DECLARE(Jxta_advertisement *) jxta_advertisement_new(void)
{
    Jxta_advertisement *ad = (Jxta_advertisement *) calloc(1, sizeof(Jxta_advertisement));

    jxta_advertisement_initialize(ad, "jxta:any_many", NULL, NULL, NULL, NULL, (FreeFunc) advertisement_free);

    return ad;
}

JXTA_DECLARE(void) jxta_advertisement_get_advs(Jxta_advertisement * ad, Jxta_vector ** advs)
{
    Jxta_vector *result = jxta_vector_new(0);
    
    JXTA_OBJECT_CHECK_VALID(ad);
    
    jxta_vector_addall_objects_last( result, ad->adv_list );
    
    *advs = result;
}

JXTA_DECLARE(void) jxta_advertisement_clear_advs(Jxta_advertisement * ad )
{
    JXTA_OBJECT_CHECK_VALID(ad);
    
    jxta_vector_clear( ad->adv_list );
}

/** 
 * Delete function to destroy the index object
 *
 **/
static void jxta_Advertisement_idx_delete(Jxta_object * jo)
{
    Jxta_index *ji = (Jxta_index *) jo;

    JXTA_OBJECT_RELEASE(ji->element);
    if (ji->attribute != NULL) {
        JXTA_OBJECT_RELEASE(ji->attribute);
    }
    if (ji->range)
        free(ji->range);
    memset(ji, 0xdd, sizeof(Jxta_index));
    free(ji);
}

/**
 * Convenience function for sub classes of advertisement to create an index vector
 **/
JXTA_DECLARE(Jxta_vector *) jxta_advertisement_return_indexes(const char *idx[])
{
    JString *element = NULL;
    JString *attribute = NULL;
    int i = 0;
    Jxta_vector *ireturn;
    
    ireturn = jxta_vector_new(0);
    for (i = 0; idx[i] != NULL; i += 2) {
        Jxta_index *jIndex;

        jIndex = (Jxta_index *) malloc(sizeof(Jxta_index));
        memset(jIndex, '\0', sizeof(Jxta_index));
        JXTA_OBJECT_INIT(jIndex, (JXTA_OBJECT_FREE_FUNC) jxta_Advertisement_idx_delete, 0);
        element = jstring_new_2((char *) idx[i]);
        if (idx[i + 1] != NULL) {
            attribute = jstring_new_2((char *) idx[i + 1]);
        } else {
            attribute = NULL;
        }
        jIndex->element = element;
        jIndex->attribute = attribute;
        jIndex->parm = NULL;
        jxta_vector_add_object_last(ireturn, (Jxta_object *) jIndex);
        JXTA_OBJECT_RELEASE(jIndex);
    }
    return ireturn;
}

void jxta_advertisement_register_global_handlers(void)
{
    global_ad_table = jxta_hashtable_new(32);
    global_ad_non_replication_table = jxta_hashtable_new(32);

    /* Core advertisement types */
    jxta_advertisement_register_global_handler("jxta:MCA", (JxtaAdvertisementNewFunc) jxta_MCA_new);
    jxta_advertisement_register_global_handler("jxta:MSA", (JxtaAdvertisementNewFunc) jxta_MSA_new);
    jxta_advertisement_register_global_handler("jxta:MIA", (JxtaAdvertisementNewFunc) jxta_MIA_new);
    jxta_advertisement_register_global_handler("jxta:PA", (JxtaAdvertisementNewFunc) jxta_PA_new);
    jxta_advertisement_register_global_handler("jxta:PGA", (JxtaAdvertisementNewFunc) jxta_PGA_new);
    jxta_advertisement_register_global_handler("jxta:RdvAdvertisement", (JxtaAdvertisementNewFunc) jxta_RdvAdvertisement_new);
    jxta_advertisement_register_global_handler("jxta:PipeAdvertisement", (JxtaAdvertisementNewFunc) jxta_pipe_adv_new);
    jxta_advertisement_register_global_handler("jxta:RA", (JxtaAdvertisementNewFunc) jxta_RouteAdvertisement_new);
    jxta_advertisement_register_global_handler("jxta:APA", (JxtaAdvertisementNewFunc) jxta_AccessPointAdvertisement_new);

    /* Core message types */
    jxta_advertisement_register_global_handler("jxta:ResolverQuery", (JxtaAdvertisementNewFunc) jxta_resolver_query_new);
    jxta_advertisement_register_global_handler("jxta:ResolverResponse", (JxtaAdvertisementNewFunc) jxta_resolver_response_new);
    jxta_advertisement_register_global_handler("jxta:DiscoveryQuery", (JxtaAdvertisementNewFunc) jxta_discovery_query_new);
    jxta_advertisement_register_global_handler("jxta:PipeResolver", (JxtaAdvertisementNewFunc) jxta_piperesolver_msg_new);


    /* Config advertisement types */
    jxta_advertisement_register_global_handler("jxta:CacheConfig", (JxtaAdvertisementNewFunc) jxta_CacheConfigAdvertisement_new );
    jxta_advertisement_register_global_handler("jxta:DiscoveryConfig", (JxtaAdvertisementNewFunc) jxta_DiscoveryConfigAdvertisement_new );
    jxta_advertisement_register_global_handler("jxta:ResolverConfig", (JxtaAdvertisementNewFunc) jxta_ResolverConfigAdvertisement_new );
    jxta_advertisement_register_global_handler("jxta:EndPointConfig", (JxtaAdvertisementNewFunc) jxta_EndPointConfigAdvertisement_new );
    jxta_advertisement_register_global_handler("jxta:MonitorConfig", (JxtaAdvertisementNewFunc) jxta_MonitorConfigAdvertisement_new );
    jxta_advertisement_register_global_handler("jxta:RdvConfig", (JxtaAdvertisementNewFunc) jxta_RdvConfigAdvertisement_new );
    jxta_advertisement_register_global_handler("jxta:SrdiConfig", (JxtaAdvertisementNewFunc) jxta_SrdiConfigAdvertisement_new );
    jxta_advertisement_register_global_handler("jxta:TCPTransportAdvertisement", (JxtaAdvertisementNewFunc) jxta_TCPTransportAdvertisement_new );
    jxta_advertisement_register_global_handler("jxta:HTTPTransportAdvertisement", (JxtaAdvertisementNewFunc) jxta_HTTPTransportAdvertisement_new );
    jxta_advertisement_register_global_handler("jxta:RelayAdvertisement", (JxtaAdvertisementNewFunc) jxta_RelayAdvertisement_new );

    /* Miscellaneous advertisement types */
    jxta_advertisement_register_global_handler("jxta:RdvLeaseOptions", (JxtaAdvertisementNewFunc) jxta_rdv_lease_options_new);
    jxta_advertisement_register_global_handler("demo:TestAdvertisement", (JxtaAdvertisementNewFunc) jxta_test_adv_new);
    jxta_advertisement_register_global_handler("jxta:PV3MonEntry", (JxtaAdvertisementNewFunc) jxta_peerview_monitor_entry_new );
    jxta_advertisement_register_global_handler("jxta:RdvMonEntry", (JxtaAdvertisementNewFunc) jxta_rdv_monitor_entry_new );
    jxta_advertisement_register_global_handler("jxta:PV3OptionEntry", (JxtaAdvertisementNewFunc) jxta_peerview_option_entry_new );

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Global ad hash table initialized\n");


    return ;
}


/* vim: set ts=4 sw=4 tw=130 et: */
