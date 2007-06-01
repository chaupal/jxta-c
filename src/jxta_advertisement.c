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
 * $Id: jxta_advertisement.c,v 1.70.2.4 2005/05/06 10:42:01 slowhog Exp $
 */

static const char *__log_cat = "ADV";

#include <apr_thread_proc.h>

#include "jpr/jpr_threadonce.h"

#include "jxta_log.h"
#include "jxta_errno.h"
#include "jxta_advertisement.h"


#ifdef __cplusplus
extern "C" {
#endif

#if 0
}
#endif
void
jxta_advertisement_initialize(Jxta_advertisement * ad,
                              const char *document_name,
                              const Kwdtab * dispatch_table,
                              JxtaAdvertisementGetXMLFunc get_xml_func,
                              JxtaAdvertisementGetIDFunc get_id_func,
                              JxtaAdvertisementGetIndexFunc get_index_func, FreeFunc free_func)
{

    ad->curr_handler = NULL;
    ad->document_name = document_name;
    ad->jht = jxta_hashtable_new(16);
    ad->dispatch_table = dispatch_table;
    JXTA_OBJECT_INIT(ad, (JXTA_OBJECT_FREE_FUNC) free_func, 0);
    ad->get_xml = get_xml_func;
    ad->get_id = get_id_func;
    ad->get_indexes = get_index_func;

#define MAX_DEPTH 16
    ad->depth = 0;
    ad->accum = jstring_new_0();
    ad->adv_list = NULL;
    ad->handler_stk = malloc(sizeof(Jxta_advertisement_node_handler) * MAX_DEPTH);
    memset(ad->handler_stk, 0, sizeof(Jxta_advertisement_node_handler) * MAX_DEPTH);
}


void jxta_advertisement_delete(Jxta_advertisement * ad)
{

    if (ad->accum != NULL) {
        JXTA_OBJECT_RELEASE(ad->accum);
    }
    free(ad->handler_stk);

    if (ad->jht != NULL) {
        JXTA_OBJECT_RELEASE(ad->jht);
    }

    if (ad->adv_list != NULL) {
        JXTA_OBJECT_RELEASE(ad->adv_list);
    }
}

void jxta_advertisement_set_handlers(Jxta_advertisement * ad, XML_Parser parser, void *parent)
{

    if (parent == NULL) {
        ad->parent = (void *) ad;
    } else {
        ad->parent = parent;
    }

    ad->parser = parser;
    XML_SetCharacterDataHandler(ad->parser, jxta_advertisement_handle_chardata);
    XML_SetUserData(ad->parser, ad);
    XML_SetElementHandler(ad->parser, jxta_advertisement_start_element, jxta_advertisement_end_element);
}



void jxta_advertisement_handle_chardata(void *user_data, const XML_Char * cd, int len)
{

    Jxta_advertisement *ad = (Jxta_advertisement *) user_data;
    jstring_append_0(ad->accum, cd, len);
}


#if 0
void select_from_hash_table(Jxta_advertisement * ad, const char *ename)
{

    Jxta_hashtable *jht = ad->jht;
    Jxta_object *val;

    jxta_hashtable_get(jht, ename, strlen(ename), &val);

    if (val != NULL) {
        jxta_advertisement_set_handlers((Jxta_advertisement *) val, ad->parser, ad);
        return;
    }

    jxta_advertisement_global_handler(ad, ename);
}

#endif


const char *jxta_advertisement_get_document_name(Jxta_advertisement * ad)
{
    return ad->document_name;
}


char *jxta_advertisement_get_string(Jxta_advertisement * ad, const char *tagname)
{

    int i;
    const Kwdtab *tags_table = ad->dispatch_table;

    if (tags_table == NULL)
        return NULL;

    i = 0;
    while (tags_table[i].kwd) {
        if ((!strcmp(tags_table[i].kwd, tagname))) {
            /*
             * Check in case someone asked for a key that has no get_xxx_string
             * support. 
             */
            if (tags_table[i].get == NULL)
                return NULL;
            return tags_table[i].get(ad);
        }
        i++;
    }

    return NULL;
}


/* Free just the initial pointer, i.e., free(tags);
 * The actual data is const and will be freed when the 
 * ad is freed.  Anyone with a better way is invited
 * to change it.
 */
const char **jxta_advertisement_get_tagnames(Jxta_advertisement * ad)
{

    const Kwdtab *tags_table = ad->dispatch_table;
    int i = 1;  /* The first entry is not interresting */
    int numtags = 1;    /* There's always at least the final NULL ptr. */
    const char **tags;

    if (tags_table == NULL)
        return NULL;

    while (tags_table[i].kwd != NULL) {
        /* Do not count those that do not have get_xxx_string support */
        if (tags_table[i].get != NULL)
            ++numtags;
        i++;
    }

    tags = (const char **) calloc(numtags, sizeof(const char *));

    numtags = 0;
    i = 1;
    while (tags_table[i].kwd != NULL) {
        /* Do not return those that do not have get_xxx_string support */
        if (tags_table[i].get != NULL)
            tags[numtags++] = tags_table[i].kwd;
        ++i;
    }
    tags[numtags] = NULL;

    return tags;

}



void log_error(XML_Parser parser, const char *document_name)
{
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Error Parsing %s : %s at line %d:%d (offset=%d,%d)\n",
             document_name,
             XML_ErrorString(XML_GetErrorCode(parser)),
             XML_GetCurrentLineNumber(parser),
             XML_GetCurrentColumnNumber(parser), XML_GetCurrentByteIndex(parser), XML_GetCurrentByteCount(parser));
}


/** FIXME: Find a way to merge these two parse functions, or 
 * have them wrap a third function that handles common code.
 */
Jxta_status jxta_advertisement_parse_charbuffer(Jxta_advertisement * ad, const char *buf, size_t len)
{

    XML_Parser parser = XML_ParserCreate(NULL);
    jxta_advertisement_set_handlers(ad, parser, (void *) NULL);

    if (!XML_Parse(parser, buf, len, 1)) {
        log_error(parser, ad->document_name);
        XML_ParserFree(parser);
        return JXTA_INVALID_ARGUMENT;
    }

    XML_ParserFree(parser);

    return JXTA_SUCCESS;
}


Jxta_status jxta_advertisement_parse_file(Jxta_advertisement * ad, FILE * instream)
{

    char buf[BUFSIZ];   /*  8192 */
    int done;

    XML_Parser parser = XML_ParserCreate(NULL);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In jxta_advertisement_parse_file\n");
    jxta_advertisement_set_handlers(ad, parser, (void *) NULL);

    do {
        size_t len = fread(buf, 1, sizeof(buf), instream);
        done = len < sizeof(buf);

        if (!XML_Parse(parser, buf, len, done)) {
            log_error(parser, ad->document_name);
            XML_ParserFree(parser);
            return JXTA_INVALID_ARGUMENT;
        }
    }
    while (!done);

    XML_ParserFree(parser);

    return JXTA_SUCCESS;
}


/**
 * @todo probably don't want to use this, hash tables are 
 * too heavy, so get rid of it.
 */
void jxta_advertisement_register_local_handler(Jxta_advertisement * ad, const char *name, const void *value)
{
/*
  apr_hash_set(ad->handlers, (const void*)name,
	       APR_HASH_KEY_STRING, value);
  
  jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "From jxta_advertisement_register_module_handler, ");
  jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "name being registered: %s\n", name);
*/
}



/* This is a "global" registry for advertisement handlers. */
/* static apr_pool_t * global_ad_pool; */

static Jxta_hashtable *global_ad_table = NULL;
static apr_thread_once_t global_hash_init_ctrl = JPR_THREAD_ONCE_INIT;


void initialize_global_hash(void)
{
    global_ad_table = jxta_hashtable_new(32);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Global ad hash table initialized\n");
}


void jxta_advertisement_new_func_struct_delete(Jxta_advertisement_new_func_struct * new_func_struct)
{

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "In jxta_ad_new_func_delete\n");
    free(new_func_struct);
}


Jxta_advertisement_new_func_struct *jxta_advertisement_new_func_struct_new(const JxtaAdvertisementNewFunc new_func)
{

    Jxta_advertisement_new_func_struct *new_func_struct =
        (Jxta_advertisement_new_func_struct *) malloc(sizeof(Jxta_advertisement_new_func_struct));
    memset(new_func_struct, 0x00, sizeof(Jxta_advertisement_new_func_struct));
    JXTA_OBJECT_INIT(new_func_struct, (JXTA_OBJECT_FREE_FUNC) jxta_advertisement_new_func_struct_delete, 0);
    new_func_struct->jxta_advertisement_new_func = new_func;

    return new_func_struct;
}




/** 
 * @param const char * for hash key
 *
 * @param Advertisement initialization function: instead of the 
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
void jxta_advertisement_register_global_handler(const char *key, const JxtaAdvertisementNewFunc ad_new_function)
{

    Jxta_advertisement_new_func_struct *new_func_struct = jxta_advertisement_new_func_struct_new(ad_new_function);

    apr_thread_once(&global_hash_init_ctrl, initialize_global_hash);


    jxta_hashtable_put(global_ad_table, key, strlen(key), (Jxta_object *) new_func_struct);
    JXTA_OBJECT_RELEASE(new_func_struct);


    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "jxta_advertisement_global_register : key being registered: %s\n", key);
}




int jxta_advertisement_global_handler(Jxta_advertisement * ad, const char *key)
{

    Jxta_advertisement *new_ad;
    Jxta_advertisement_new_func_struct *new_func_struct = NULL;

    apr_thread_once(&global_hash_init_ctrl, initialize_global_hash);

    jxta_hashtable_get(global_ad_table, key, strlen(key), (Jxta_object **) & new_func_struct);

    if (new_func_struct != NULL) {
        Jxta_advertisement *(*new_func) (void);
        new_func = new_func_struct->jxta_advertisement_new_func;

        /* Do what a subdocument handler would do: */

        /* create the sub-ad */
        new_ad = (Jxta_advertisement *) (*new_func) ();

        /* hook it somewhere (our list of advs) */
        jxta_vector_add_object_last(ad->adv_list, (Jxta_object *) new_ad);

        /* set new handlers */
        jxta_advertisement_set_handlers(new_ad, ad->parser, ad);

        JXTA_OBJECT_RELEASE(new_func_struct);
        JXTA_OBJECT_RELEASE(new_ad);

        /*
         * then return to the back end engine
         * (let it know that we found a handler)
         */
        return 1;
    }

    /* temporary, just for debugging */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Element name %s has no handler...\n", key);

    /* let the back end know that no handler was found */
    return 0;
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
}





void jxta_advertisement_start_element(void *userdata, const char *ename, const char **atts)
{
    int i;
    const char **att;

    Jxta_advertisement *ad = (Jxta_advertisement *) userdata;
    const Kwdtab *tags_table = ad->dispatch_table;
    ad->atts = atts;

    if (ad->depth >= MAX_DEPTH) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Advertisement too deep.\n");
        /* Keep counting so that we stay in sync. */
        ++(ad->depth);
        return;
    }

    ad->handler_stk[ad->depth++] = ad->curr_handler;

    att = atts;
    while (*att != NULL) {
        if (!strcmp(*(att++), "type"))
            break;
        ++att;
    }

    /* IF, and only IF, we do not have a tags_table, THEN we're a base class
     * adv handling an unknown batch of documents. Then, we look in the global
     * hash table, for the handler; NOT the tags_table.
     */
    if (tags_table == NULL) {
        /*
         * If we do not have a tags table but we do not have an adv_list
         * either, then we cannot do anything with what we parse.
         */
        if (ad->adv_list == NULL)
            return;

        /*
         * If we got a type attribute, try and match it first.
         */
        if (*att != NULL) {
            /* If a handler is found, we're done. */
            if (jxta_advertisement_global_handler(ad, *att))
                return;
        }

        /* if we're still here, try the element name */
        jxta_advertisement_global_handler(ad, ename);

        /* we've done all we could, nomatter if a handler was found */
        return;
    }

    /*
     * Else, the adv is a specific subclass; it has a tags table.
     */

    /*
     * If we got a type attribute, try and match it first.
     */
    if (*att != NULL) {
        i = 0;
        while (tags_table[i].kwd) {
            if ((!strcmp(tags_table[i].kwd, *att))) {
                ad->curr_handler = tags_table[i].nodeparse;

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
                ad->curr_handler(userdata, "", 0);

                return;
            }
            i++;
        }
    }


    /*
     * If there is no type attr or it means nothing to us, try with the element
     * name.
     */

    i = 0;
    while (tags_table[i].kwd) {
        if ((!strcmp(tags_table[i].kwd, ename))) {
            ad->curr_handler = tags_table[i].nodeparse;

            /* Call the handler exactly once at start, with a zero-length
             * string. Existing handlers are already designed to cope with
             * it. Handlers will ignore it except if the tag marks a complex
             * sub-element, in which case the handler *needs* to be called
             * at start to switch user-data. We could let the application
             * code supply a different handler for start and end, but all
             * advertisement is already written with a single handler
             * that's called any number of times. This change just makes the
             * number of calls predictable, thus allowing the reliable
             * processing of repeat tags without breaking existing advertisement
             * code.
             */
            ad->curr_handler(userdata, "", 0);

            return;
        }
        i++;
    }

    /* Check if the element was
     * registered at run time.
     */
    /* select_from_hash_table(ad,ename); */
}



/** All we get here is an element name.  So we have to 
 * scan the dispatch table for the correct element name,
 * then we can set the curr_handler.  That way the character 
 * handler can use the curr_handler to dispatch the stream 
 * to the correct function.
 */
void jxta_advertisement_end_element(void *userdata, const char *tagname)
{

    Jxta_advertisement *ad = (Jxta_advertisement *) userdata;

    /* If at the end of a subdocument (any element with subelements is
     * handled as a subdocument), reset the userdata 
     * for the parents dispatch table.
     */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "IN END ELEMENT. Doc: %s. Tag: %s\n", ad->document_name, tagname);

    /*
     * What can we do if the tag does not match the document name anyway ?
     * And in some corner cases, the adv initializer may have set it wrong
     * because of the schema subclassing buziness. So, it's better not to check.
     */

    if (ad->depth == 0) {

        Jxta_advertisement *parent = ad->parent;
        XML_SetUserData(parent->parser, parent);

        parent->curr_handler = parent->handler_stk[--(parent->depth)];
        /* We can get rid of the accumulator now, since we'll switch to
         * the parent jxta_advertisement object.
         */
        JXTA_OBJECT_RELEASE(ad->accum);
        ad->accum = NULL;
        free(ad->handler_stk);
        ad->handler_stk = NULL;
    } else {

        if (--(ad->depth) >= MAX_DEPTH)
            return;

        /* This is the end of a simple tag (inside a document).
         * Call the adv-specific handler with the complete data.
         * Note that the complete data may be a zero-length string.
         * To make it easier for adv-specific handlers to handler empty tags
         * that matter (there are some) we make sure that an empty
         * end-tag call has some distinguishing quality compared to a start-tag
         * call. To remain compatible with the current expectations of code with
         * regard to empty tags, we'll pass just a blank. That way, new code
         * that needs to may recognize empty tags, while older code will see no
         * difference (still being called any number of times with possibly only
         * spaces).
         * Since most code currently incorrectly assumes that data is either
         * zero-length after stripping or complete, most code will now be
         * correct without modification. Code (currently broken) that may
         * deal with empty tags that can repeat, has to be fixed as follows:
         * 1 - always ignore zero length data - this is the start.
         * 2 - strip non-zero length data, what's left is the value even if
         *     there's nothing left.
         */
        if (ad->curr_handler != NULL) {
            if (jstring_length(ad->accum) == 0) {
                ad->curr_handler(userdata, " ", 1);
            } else {
                char *result;
                jstring_reset(ad->accum, &result);
                ad->curr_handler(userdata, result, strlen(result));
                free(result);
            }
        }
        ad->curr_handler = ad->handler_stk[ad->depth];

    }
}


void jxta_advertisement_get_xml(Jxta_advertisement * ad, JString ** js)
{
    if (ad->get_xml != NULL) {
        ad->get_xml(ad, js);
    }
}


Jxta_id *jxta_advertisement_get_id(Jxta_advertisement * ad)
{

    if (ad->get_id != NULL) {
        return ad->get_id(ad);
    }
    return NULL;
}

Jxta_vector *jxta_advertisement_get_indexes(Jxta_advertisement * ad)
{

    if (ad->get_indexes != NULL) {
        return ad->get_indexes();
    }
    return NULL;
}

static void jxta_advertisement_free(Jxta_advertisement* me)
{
    jxta_advertisement_delete(me);
    free(me);
}

Jxta_advertisement *jxta_advertisement_new(void)
{
    Jxta_advertisement *ad = (Jxta_advertisement *) calloc(1, sizeof(Jxta_advertisement));

    jxta_advertisement_initialize(ad, "jxta:any_many", NULL, NULL, NULL, NULL, (FreeFunc) jxta_advertisement_free);

    /*
     * adv_list is only used for base class advs created by this routine.
     * subclasses do not use adv_list.
     */
    ad->adv_list = jxta_vector_new(1);
    return ad;
}

void jxta_advertisement_get_advs(Jxta_advertisement * ad, Jxta_vector ** advs)
{
    if (ad->adv_list)
        JXTA_OBJECT_SHARE(ad->adv_list);
    *advs = ad->adv_list;
}

/** 
 * Delete function to destroy the index object
 *
 */
static void jxta_Advertisement_idx_delete(Jxta_object * jo)
{
    Jxta_index *ji = (Jxta_index *) jo;
    JXTA_OBJECT_RELEASE(ji->element);
    if (ji->attribute != NULL) {
        JXTA_OBJECT_RELEASE(ji->attribute);
    }
    memset(ji, 0xdd, sizeof(Jxta_index));
    free(ji);
}

/**
 * Convenience function for sub classes of advertisement to create an index vector
*/
Jxta_vector *jxta_advertisement_return_indexes(const char *idx[])
{
    JString *element;
    JString *attribute;

    int i = 0;
    Jxta_vector *ireturn;
    ireturn = jxta_vector_new(0);
    for (i = 0; idx[i] != NULL; i += 2) {
        Jxta_index *jIndex = (Jxta_index *) calloc(1, sizeof(Jxta_index));

        JXTA_OBJECT_INIT(jIndex, (JXTA_OBJECT_FREE_FUNC) jxta_Advertisement_idx_delete, 0);
        element = jstring_new_2((char *) idx[i]);
        if (idx[i + 1] != NULL) {
            attribute = jstring_new_2((char *) idx[i + 1]);
        } else {
            attribute = NULL;
        }
        jIndex->element = element;
        jIndex->attribute = attribute;
        jxta_vector_add_object_last(ireturn, (Jxta_object *) jIndex);
        JXTA_OBJECT_RELEASE(jIndex);
    }
    return ireturn;
}


#ifdef __cplusplus
#ifdef 0
{
#endif
}
#endif
