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
 * $Id$
 */

/**
 * Internal documentation for the jxta advertisement handling
 * code.
 *
 * jxta advertisements are handled using a sax interface to an 
 * xml parser.  The advertisements are more alike than different,
 * which allows code common to all advertisements to be handled 
 * in the same place.  Most of the code is `private' and will never
 * be seen by an application developer.  The API specified for 
 * handling advertisements has completely opaque implementation.
 * Consequently, function-level documentation will not be very 
 * helpful to understand how the system works.  Instead, most 
 * function names are self-descriptive.
 *
 * Understanding how the system works first requires understanding 
 * xml sax parsing.  This code currently uses expat, for which there
 * is a substantial amount of documentation available on the web.
 * 
 * The concept behind the advertisement handling is separation 
 * of data from data processing.  This is accomplished by using
 * a form of inheritance: all jxta advertisements include a 
 * jxta_advertisement type as the first member of an advertisement 
 * struct.  The jxta_advertisement type is defined in jxta_advertisement.h,
 * which should be considered `private'.  This allows each advertisement,
 * such as a peer advertisement, to be cast up to a jxta_advertisement
 * and processed with code common to all advertisements.  This saves
 * approximately 500 bytes from each advertisements code, and 
 * vastly simplifies maintenance: parsing any advertisement correct
 * parses them all correctly.  The functions that handle the parsing 
 * are in jxta_advertisement.c, and are completely private.  These 
 * functions carry no internal state.
 *
 * Data is carried by classes that specify each advertisement 
 * according to type.  These classes provide a public api for 
 * manipulating the advertisement, through get/set functions.
 * The advertisement itself is opaque.
 *
 * Parsing a specific type of advertisement is accomplished by 
 * invoking that advertisements parse methods on the appropriate 
 * data stream.  The parse methods are wrappers for the 
 * the actual code residing in jxta_advertisement.c that performs 
 * the work.  The data consists of the parser state (see documentation
 * for sax parsing), strings corresponding to tag names, and 
 * callbacks for handling character data associated with a specific
 * element.  These data are initialized in an array of Kwdtab
 * structs.  The jxta_advertisement parsing consists of dereferencing
 * this array to set the state in the start_element method, then 
 * dispatching the character data to the function specified by that 
 * state in the callback.
 *
 * There is not very much code here, but there is a lot going on.
 * As stated previously, without a global view of what this code does,
 * no individual element is going to shed much light on it.  Conversely,
 * it should be pretty easy to see what any particular piece is 
 * doing.  These comments will be extended over time to better describe
 * the internals. Other comments welcome. List any questions below, and 
 * I will answer them.
 *
 * There will appear test code that exercises most of this that 
 * can be used as example.
 *
 * Questions on code: 
 * 
 * 1. ???
 *
 */

#ifndef __JXTA_ADVERTISEMENT_H__
#define __JXTA_ADVERTISEMENT_H__

#include <stddef.h>
#include <stdio.h>

#include <expat.h>

#include "jxta_object.h"

#include "jstring.h"
#include "jxta_hashtable.h"
#include "jxta_id.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_advertisement Jxta_advertisement;
typedef struct _jxta_index Jxta_index;
typedef struct _jxta_attribute Jxta_attribute;

#ifndef FREE_FUNC
#define FREE_FUNC
/* FIXME 20060822 bondolo Why is this not JXTA_OBJECT_FREE_FUNC? */
typedef void (*FreeFunc) (void *me);
#endif

#ifndef  JXTA_ADVERTISEMENT_GET_XML_FUNC
#define  JXTA_ADVERTISEMENT_GET_XML_FUNC
typedef Jxta_status(JXTA_STDCALL * JxtaAdvertisementGetXMLFunc) (Jxta_advertisement *, JString **);
#endif

#ifndef  JXTA_ADVERTISEMENT_NEW_FUNC
#define  JXTA_ADVERTISEMENT_NEW_FUNC
typedef Jxta_advertisement *(JXTA_STDCALL * JxtaAdvertisementNewFunc) (void);
#endif

#ifndef  JXTA_ADVERTISEMENT_GET_ID_FUNC
#define  JXTA_ADVERTISEMENT_GET_ID_FUNC
typedef Jxta_id *(JXTA_STDCALL * JxtaAdvertisementGetIDFunc) (Jxta_advertisement *);
#endif

#ifndef  JXTA_ADVERTISEMENT_GET_INDEX_FUNC
#define  JXTA_ADVERTISEMENT_GET_INDEX_FUNC
typedef Jxta_vector *(JXTA_STDCALL * JxtaAdvertisementGetIndexFunc) (Jxta_advertisement *);
#endif

/** Each tag has a character representation, and we have 
 * also given it an enum representation above.  We can 
 * tie those to the handler function in one struct, thus
 * having any one of them gets any of the others.
 *
 * For each tag recognized there has to be handler.
 * nodeparse and string parse are the same thing.
 */

/*
 * Change of behavior:
 *
 * The handler is now called twice: once when the start tag is seen, and once
 * when the end-tag is seen. If this handler switches to a subdocument type
 * by calling jxta_advertisement_set_handlers, it has to be on the first call
 * then, the second call never occurs.
 *
 * The first call is always made with a zero-length string. The second call
 * is never made with a zero-length string. The second call provides all the
 * text contained in the element as one string, including separators. If the
 * element was empty, a string of exactly one space is passed.
 *
 * The reason behind this behavior is backward compatibility:
 * Before this change was introduced, handlers could be called any number
 * of times with or without any data and without enough info to discriminate
 * tag boundaries.
 *
 * All legacy code had to (incorrectly) assume that data was either
 * zero-length after stripping or complete, or could only deal (correctly)
 * with fully cumulative data (there could be only one occurrence of a given
 * tag). With this change of behavior, all this code becomes or remains
 * correct without modification. Code (so far broken) that could have to
 * deal with empty tags that could repeat, should be fixed as follows:
 *
 * 1 - always ignore zero length data - this is the start.
 * 2 - strip non-zero length data, what's left is the value even if
 *     there's nothing left.
 *
 */

typedef void (*Jxta_advertisement_node_handler) (void *, const XML_Char *, int);

typedef int Jxta_kwd_entry_flags;

#define NO_REPLICATION 0x8000

struct _kwdtab {
    const char *kwd;
    Jxta_kwd_entry_flags tokentype;
    Jxta_advertisement_node_handler nodeparse;

    char *(JXTA_STDCALL * get) (Jxta_advertisement *);
    char *(JXTA_STDCALL * get_parm) (Jxta_advertisement *, const char *);
};

typedef struct _kwdtab Kwdtab;

struct _jxta_advertisement {
    JXTA_OBJECT_HANDLE;

    const char *document_name;
    JxtaAdvertisementGetXMLFunc get_xml;
    JxtaAdvertisementGetIDFunc get_id;
    JxtaAdvertisementGetIndexFunc get_indexes;
    Kwdtab *dispatch_table;
    char *local_id;

    /*
     * The following is used when this base class is used directly
     * to handle a document that may contain any number of unknown
     * advertisements. Each advertisement is constructed and accumulated
     * in this vector.
     * jxta_advertisement_new takes care of properly init'ing 
     * jxta_advertisement for that particular usage.
     */
    Jxta_vector *adv_list;

    Jxta_advertisement *parent;
    XML_Parser parser;
    Jxta_advertisement_node_handler *handler_stk;
    unsigned int depth;
    const char **atts;
    JString *accum;
    Jxta_advertisement_node_handler curr_handler;
    const char *currElement;
};

struct _jxta_index {
    JXTA_OBJECT_HANDLE;
    JString *element;
    JString *attribute;
    char *parm;
    Jxta_object *range;
};

/**
*   
*
*   @deprecated This is being deprecated in favor of the style used by other jxta objects. @see jxta_advertisement_construct()
*   and @see jxta_advertisement_destruct()
**/
JXTA_DECLARE(Jxta_advertisement *) jxta_advertisement_initialize(Jxta_advertisement * ad,
                                                 const char *document_name,
                                                 const Kwdtab * dispatch_table,
                                                 JxtaAdvertisementGetXMLFunc,
                                                 JxtaAdvertisementGetIDFunc, 
                                                 JxtaAdvertisementGetIndexFunc, 
                                                 FreeFunc deleter);

JXTA_DECLARE(Jxta_advertisement *) jxta_advertisement_construct(Jxta_advertisement * ad,
                                                                const char *document_name,
                                                                const Kwdtab * dispatch_table,
                                                                JxtaAdvertisementGetXMLFunc get_xml_func,
                                                                JxtaAdvertisementGetIDFunc get_id_func,
                                                                JxtaAdvertisementGetIndexFunc get_index_func);

/**
*   to be called from inside destruct functions for sub-classes
*/
JXTA_DECLARE(void) jxta_advertisement_destruct(Jxta_advertisement * ad);

/**
*   
*
*   @deprecated This should be private. Sub-classes should use "jxta_advertisement_destruct"
**/
void jxta_advertisement_delete(Jxta_advertisement * ad);


JXTA_DECLARE(void) jxta_advertisement_set_handlers(Jxta_advertisement * ad, XML_Parser parser, Jxta_advertisement * parent);

JXTA_DECLARE(void) jxta_advertisement_clear_handlers(Jxta_advertisement * ad );

/** 
 ** Convenience function to builds a vector of Jxta_index structs that return the
 ** element/attribute pairs returned for indexing. 
 **
 ** @param idx - array of element/attribute pairs to be indexed.
 */
JXTA_DECLARE(Jxta_vector *) jxta_advertisement_return_indexes(const char *idx[]);

/** Public. */

/** 
 * Wrapper function for accumulating the data in any advertisement
 *  into XML format for transmission.  This should just invoke a 
 *  preregistered callback.
 * 
 * @param Jxta_advertisement * ad
 * @param JString ** an object reference to accumulate tags and data
 *        for string representation of advertisements.
 *
 * @return status
 */
JXTA_DECLARE(Jxta_status) jxta_advertisement_get_xml(Jxta_advertisement * ad, JString **);

/** Some advertisements have IDs that are used for indexing etc. 
 * This function wraps a callback to get the specific ID type 
 * for any ID.
 * 
 * @param Jxta_advertisement * ad
 * @param
 *
 * @return Jxta_id *, could be NULL depending on whether the advertisement
 *         has an ID.
 */
JXTA_DECLARE(Jxta_id *) jxta_advertisement_get_id(Jxta_advertisement * ad);

/** For Advertisements that do not return an ID a hash of the document is used as the ID for the local cache
 * 
 *  @param - Jxta_advertisement * ad - advertisement pointer
 *
 *  @return const char *, the string representing the local cache ID.
*/
JXTA_DECLARE(const char *) jxta_advertisement_get_local_id_string(Jxta_advertisement * ad);

/** Set a local cache ID when an advertisement does not have an AdvertisementID element.
 *
 *  @param - Jxta_advertisement * ad - advertisement pointer
 *  @param - const char * local_id - id used in the local cache.
 *  
*/
JXTA_DECLARE(void) jxta_advertisement_set_local_id_string(Jxta_advertisement * ad, const char *local_id);

/** Advertisement that index items provide a callback to return
 * a vector of items. 
 * 
 * @param Jxta_advertisement * ad
 *
 * @return Jxta_vector *, vector of Jxta_index items
 */
JXTA_DECLARE(Jxta_vector *) jxta_advertisement_get_indexes(Jxta_advertisement * ad);

/** Each advertisement is initialized with a constant character 
 *  array denoting its type.  This value should be copied if it 
 *  is used for purposes other than comparison or printing.
 * 
 * @param Jxta_advertisement * ad
 *
 * @return const char * to document name.  
 * @warning Do not free the returned char *
 */
JXTA_DECLARE(const char *) jxta_advertisement_get_document_name(Jxta_advertisement * ad);

/** Registers callback for a printing function.
 * 
 * @param Jxta_advertisement * ad
 * @param PrintFunc callback for printing to a stream.
 *
 * @return nothing
 */
JXTA_DECLARE(void) jxta_advertisement_set_printer(Jxta_advertisement * ad, PrintFunc printer);

/**  Many fields of an advertisement struct can be represented as 
 * a character string, which may be obtained by this function.
 * 
 * @param Jxta_advertisement * ad
 * @param char * key for advertisement field desired
 *
 * @return char * representation of value corresponding to 
 *         key.
 *
 * @warning Returns a copy that must be freed.
 */
JXTA_DECLARE(char *) jxta_advertisement_get_string(Jxta_advertisement * ad, Jxta_index * ji);

/**  Many fields of an advertisement struct can be represented as 
 * a character string, which may be obtained by this function.
 * 
 * @param Jxta_advertisement * ad
 * @param char * key for advertisement field desired
 * @param Jxta_index_flags * flags indicating properties of the value
 *
 * @return char * representation of value corresponding to 
 *         key.
 *
 * @warning Returns a copy that must be freed.
 */
JXTA_DECLARE(char *) jxta_advertisement_get_string_1(Jxta_advertisement * ad, Jxta_index * ji, Jxta_kwd_entry_flags * flags);

/** Messages saved in a buffer can be parsed later using 
 * this function.
 * 
 * @param Jxta_advertisement * ad
 * @param const char * buf is a buffer of character data presumably
 *        containing a valid XML application.
 * @param len length of character buffer containing xml data.
 *
 * @return JXTA_SUCCESS if the parse succeeded otherwise errors.
 */
JXTA_DECLARE(Jxta_status) jxta_advertisement_parse_charbuffer(Jxta_advertisement * ad, const char *buf, size_t len);

/**
 * 
 * @param Jxta_advertisement * ad
 * @param FILE * a valid input stream
 *
 * @return JXTA_SUCCESS if the parse succeeded otherwise errors.
 *
 * @todo Change the FILE * to a void pointer.
 */
JXTA_DECLARE(Jxta_status) jxta_advertisement_parse_file(Jxta_advertisement * ad, FILE * instream);

/**
 * 
 * @param Jxta_advertisement * ad
 * @param
 *
 * @return const char **, i.e., an array of char * that points to 
 *         const strings stored in the private tags table in each 
 *         type of adverttisement.
 */
JXTA_DECLARE(const char **) jxta_advertisement_get_tagnames(Jxta_advertisement * ad);

/**
 * 
 * @param Jxta_advertisement * ad
 * @param
 *
 * @return nothing
 */
JXTA_DECLARE(void) jxta_advertisement_register_global_handler(const char *key, const JxtaAdvertisementNewFunc ad_new_function);

/**
 *
 * Returns whether a particulare ns:element/attribute should be replicated.
 *
 * @param const char * concatenation of the namespace:element/attribute
 *
 * @return TRUE-advertisement wants this element replicated FALSE- do not replicate
*/
JXTA_DECLARE(Jxta_boolean) jxta_advertisement_is_element_replicated(const char *ns_elem_attr);

/**
*
**/
JXTA_DECLARE(Jxta_status) jxta_advertisement_global_handler(Jxta_advertisement * ad, const char *doc_type, Jxta_advertisement** new_ad);

/**
 * The base type Jxta_advertisement can be used to parse documents containing
 * any number of unspecified advertisements, provided that their document
 * types have been registered with jxta_advertisement_register_global_handler.
 * Only base advertisement objects returned by jxta_advertisement_new() can
 * perform that function.
 *
 * @param Jxta_advertisement * ad
 * @param advs A location where to return a ptr to a vector containing all
 * the advertisements that could be parsed from the input text. If the given
 * ad does not point at a base class Jxta_advertisement object, *advs will be
 * set to NULL.
 *
 * @return nothing
 */
JXTA_DECLARE(void) jxta_advertisement_get_advs(Jxta_advertisement * ad, Jxta_vector ** advs);

/**
 * Builds a base Jxta_advertisement object that can be used to parse documents
 * containing unspecified advertisement types; provided that their document
 * types have been registered with jxta_advertisement_register_global_handler.
 * @return Jxta_advertisement* a pointer to the newly created advertisement.
 */
JXTA_DECLARE(Jxta_advertisement *) jxta_advertisement_new(void);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_ADVERTISEMENT_H__ */
