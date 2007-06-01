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
 * $Id: jxta_test_adv.c,v 1.11 2005/08/03 05:51:19 slowhog Exp $
 */


/*
* The following command will compile the output from the script 
* given the apr is installed correctly.
*/
/*
* gcc -DSTANDALONE jxta_advertisement.c DiscoveryResponse.c  -o PA \
  `/usr/local/apache2/bin/apr-config --cflags --includes --libs` \
  -lexpat -L/usr/local/apache2/lib/ -lapr
*/

#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_test_adv.h"
#include "jxta_xml_util.h"

#define KWD_EXT

#ifdef __cplusplus
extern "C" {
#endif

    /** Each of these corresponds to a tag in the
     * xml ad.
     */
    enum tokentype {
        Null_,
        TestAdvertisement_,
        Id_,
        IdAttr_,
        IdAttr1_,
        Type_,
        Name_,
        NameAttr1_
    };


    /** This is the representation of the
     * actual ad in the code.  It should
     * stay opaque to the programmer, and be 
     * accessed through the get/set API.
     */
    struct _jxta_test_adv {
        Jxta_advertisement jxta_advertisement;
        char *Id;
        char *IdAttr;
        char *IdAttr1;
        char *Type;
        char *Name;
        char *NameAttr1;
        char *NameAttr2;
    };

    static void jxta_test_adv_delete(Jxta_test_adv *);
    static Jxta_test_adv *jxta_test_adv_construct(Jxta_test_adv * self);

    /** Handler functions.  Each of these is responsible for
     * dealing with all of the character data associated with the 
     * tag name.
     */
    static void
     handleTestAdvertisement(void *userdata, const XML_Char * cd, int len) {
        if (userdata || cd || len) {
        }
        /* Jxta_test_adv * ad = (Jxta_test_adv*)userdata; */ } static void
     handleId(void *userdata, const XML_Char * cd, int len) {
        Jxta_test_adv *ad = (Jxta_test_adv *) userdata;
        JString *tmp;
        char *tok;
        if (ad->Id)
            free(ad->Id);
        /* Make room for a final \0 in advance; we'll likely need it. */
        tmp = jstring_new_1(len + 1);

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);
        ad->Id = (char *) jstring_get_string(tmp);
        if (strlen(ad->Id) != 0) {
            tok = calloc(1, strlen(ad->Id) + 1);
            memmove(tok, ad->Id, strlen(ad->Id));
            ad->Id = tok;
        } else {
            ad->Id = NULL;
        }
        JXTA_OBJECT_RELEASE(tmp);
    }
    static void
     handleIdAttr(void *userdata, const XML_Char * cd, int len) {
        Jxta_test_adv *ad = (Jxta_test_adv *) userdata;
        JString *tmp;
        char *tok;
        if (ad->IdAttr)
            free(ad->IdAttr);

        /* Make room for a final \0 in advance; we'll likely need it. */
        tmp = jstring_new_1(len + 1);

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);
        ad->IdAttr = (char *) jstring_get_string(tmp);
        if (strlen(ad->IdAttr) != 0) {
            tok = calloc(1, strlen(ad->IdAttr) + 1);
            memmove(tok, ad->IdAttr, strlen(ad->IdAttr));
            ad->IdAttr = tok;
        } else {
            ad->IdAttr = NULL;
        }
        JXTA_OBJECT_RELEASE(tmp);
    }
    static void
     handleIdAttr1(void *userdata, const XML_Char * cd, int len) {
        Jxta_test_adv *ad = (Jxta_test_adv *) userdata;
        JString *tmp;
        char *tok;
        if (ad->IdAttr1)
            free(ad->IdAttr1);

        /* Make room for a final \0 in advance; we'll likely need it. */
        tmp = jstring_new_1(len + 1);

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);
        ad->IdAttr1 = (char *) jstring_get_string(tmp);
        if (strlen(ad->IdAttr1) != 0) {
            tok = calloc(1, strlen(ad->IdAttr1) + 1);
            memmove(tok, ad->IdAttr1, strlen(ad->IdAttr1));
            ad->IdAttr1 = tok;
        } else {
            ad->IdAttr1 = NULL;
        }
        JXTA_OBJECT_RELEASE(tmp);
    }

    static void
     handleType(void *userdata, const XML_Char * cd, int len) {
        Jxta_test_adv *ad = (Jxta_test_adv *) userdata;
        JString *tmp;
        char *tok;
        if (ad->Type)
            free(ad->Type);

        /* Make room for a final \0 in advance; we'll likely need it. */
        tmp = jstring_new_1(len + 1);

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);
        ad->Type = (char *) jstring_get_string(tmp);
        if (strlen(ad->Type) != 0) {
            tok = calloc(1, strlen(ad->Type) + 1);
            memmove(tok, ad->Type, strlen(ad->Type));
            ad->Type = tok;
        } else {
            ad->Type = NULL;
        }
        JXTA_OBJECT_RELEASE(tmp);
    }

    static void
     handleName(void *userdata, const XML_Char * cd, int len) {
        Jxta_test_adv *ad = (Jxta_test_adv *) userdata;
        JString *tmp;
        char *tok;
        if (ad->Name)
            free(ad->Name);

        /* Make room for a final \0 in advance; we'll likely need it. */
        tmp = jstring_new_1(len + 1);

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);
        ad->Name = (char *) jstring_get_string(tmp);
        if (strlen(ad->Name) != 0) {
            tok = calloc(1, strlen(ad->Name) + 1);
            memmove(tok, ad->Name, strlen(ad->Name));
            ad->Name = tok;
        } else {
            ad->Name = NULL;
        }
        JXTA_OBJECT_RELEASE(tmp);
    }
    static void
     handleNameAttr1(void *userdata, const XML_Char * cd, int len) {
        Jxta_test_adv *ad = (Jxta_test_adv *) userdata;
        JString *tmp;
        char *tok;
        if (ad->NameAttr1)
            free(ad->NameAttr1);

        /* Make room for a final \0 in advance; we'll likely need it. */
        tmp = jstring_new_1(len + 1);

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);
        ad->NameAttr1 = (char *) jstring_get_string(tmp);
        if (strlen(ad->NameAttr1) != 0) {
            tok = calloc(1, strlen(ad->NameAttr1) + 1);
            memmove(tok, ad->NameAttr1, strlen(ad->NameAttr1));
            ad->NameAttr1 = tok;
        } else {
            ad->NameAttr1 = NULL;
        }
        JXTA_OBJECT_RELEASE(tmp);
    }
    static void
     handleNameAttr2(void *userdata, const XML_Char * cd, int len) {
        Jxta_test_adv *ad = (Jxta_test_adv *) userdata;
        JString *tmp;
        char *tok;
        if (ad->NameAttr2)
            free(ad->NameAttr2);

        /* Make room for a final \0 in advance; we'll likely need it. */
        tmp = jstring_new_1(len + 1);

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);
        ad->NameAttr2 = (char *) jstring_get_string(tmp);
        if (strlen(ad->NameAttr2) != 0) {
            tok = calloc(1, strlen(ad->NameAttr2) + 1);
            memmove(tok, ad->NameAttr2, strlen(ad->NameAttr2));
            ad->NameAttr2 = tok;
        } else {
            ad->NameAttr2 = NULL;
        }
        JXTA_OBJECT_RELEASE(tmp);
    }

    /** The get/set functions represent the public
     * interface to the ad class, that is, the API.
     */

    JXTA_DECLARE(Jxta_id *) jxta_test_adv_get_testid(Jxta_test_adv * ad) {
        Jxta_id *testid = NULL;
        JString *tmps = jstring_new_2(ad->Id);
        jxta_id_from_jstring(&testid, tmps);
        JXTA_OBJECT_RELEASE(tmps);
        return testid;
    }

    JXTA_DECLARE(const char *) jxta_test_adv_get_Id(Jxta_test_adv * ad) {
        return ad->Id;
    }

    char *JXTA_STDCALL jxta_test_adv_get_Id_string(Jxta_advertisement * ad) {

        char *res = NULL;
        if (((Jxta_test_adv *) ad)->Id != NULL) {
            res = strdup(((Jxta_test_adv *) ad)->Id);
        }
        return res;
    }
    JXTA_DECLARE(Jxta_status) jxta_test_adv_set_IdAttr1(Jxta_test_adv * ad, const char *val) {

        if (val != NULL) {
            ad->IdAttr1 = calloc(1, strlen(val) + 1);
            strcpy(ad->IdAttr1, val);
        } else {
            return JXTA_INVALID_ARGUMENT;
        }

        return JXTA_SUCCESS;
    }

    JXTA_DECLARE(const char *) jxta_test_adv_get_IdAttr1(Jxta_test_adv * ad) {
        return ad->IdAttr1;
    }

    char *JXTA_STDCALL jxta_test_adv_get_idattr1_string(Jxta_advertisement * ad) {
        char *res;
        res = strdup(((Jxta_test_adv *) ad)->IdAttr1);
        return res;
    }

    JXTA_DECLARE(Jxta_status) jxta_test_adv_set_Id(Jxta_test_adv * ad, const char *val) {

        if (val != NULL) {
            ad->Id = calloc(1, strlen(val) + 1);
            strcpy(ad->Id, val);
        } else {
            return JXTA_INVALID_ARGUMENT;
        }

        return JXTA_SUCCESS;
    }
    JXTA_DECLARE(Jxta_status) jxta_test_adv_set_IdAttr(Jxta_test_adv * ad, const char *val) {

        if (val != NULL) {
            ad->IdAttr = calloc(1, strlen(val) + 1);
            strcpy(ad->IdAttr, val);
        } else {
            return JXTA_INVALID_ARGUMENT;
        }

        return JXTA_SUCCESS;
    }

    JXTA_DECLARE(const char *) jxta_test_adv_get_IdAttr(Jxta_test_adv * ad) {
        return ad->IdAttr;
    }

    JXTA_DECLARE(char *) jxta_test_adv_get_idattr_string(Jxta_advertisement * ad) {
        char *res;
        res = strdup(((Jxta_test_adv *) ad)->IdAttr);
        return res;
    }

    JXTA_DECLARE(const char *) jxta_test_adv_get_Type(Jxta_test_adv * ad) {
        return ad->Type;
    }

    char *JXTA_STDCALL jxta_test_adv_get_Type_string(Jxta_advertisement * ad) {
        return strdup(((Jxta_test_adv *) ad)->Type);
    }

    JXTA_DECLARE(Jxta_status) jxta_test_adv_set_Type(Jxta_test_adv * ad, const char *val) {

        if (val != NULL) {
            ad->Type = calloc(1, strlen(val) + 1);
            strcpy(ad->Type, val);
        } else {
            return JXTA_INVALID_ARGUMENT;
        }

        return JXTA_SUCCESS;
    }


    JXTA_DECLARE(const char *) jxta_test_adv_get_Name(Jxta_test_adv * ad) {
        return ad->Name;
    }

    char *JXTA_STDCALL jxta_test_adv_get_Name_string(Jxta_advertisement * ad) {
        return strdup(((Jxta_test_adv *) ad)->Name);

    }
    JXTA_DECLARE(Jxta_status) jxta_test_adv_set_NameAttr1(Jxta_test_adv * ad, const char *val) {

        if (val != NULL) {
            ad->NameAttr1 = calloc(1, strlen(val) + 1);
            strcpy(ad->NameAttr1, val);
        } else {
            return JXTA_INVALID_ARGUMENT;
        }

        return JXTA_SUCCESS;
    }
    JXTA_DECLARE(const char *) jxta_test_adv_get_NameAttr1(Jxta_test_adv * ad) {
        return ad->NameAttr1;
    }

    char *JXTA_STDCALL jxta_test_adv_get_nameattr1_string(Jxta_advertisement * ad) {
        char *res;
        res = strdup(((Jxta_test_adv *) ad)->NameAttr1);
        return res;
    }

    Jxta_status jxta_test_adv_set_NameAttr2(Jxta_test_adv * ad, const char *val) {

        if (val != NULL) {
            ad->NameAttr2 = malloc(strlen(val) + 1);
            strcpy(ad->NameAttr2, val);
        } else {
            return JXTA_INVALID_ARGUMENT;
        }

        return JXTA_SUCCESS;
    }

    char *jxta_test_adv_get_NameAttr2(Jxta_test_adv * ad) {
        return (char *) ad->NameAttr2;
    }

    char *jxta_test_adv_get_nameattr2_string(Jxta_advertisement * ad) {
        char *res;
        res = strdup(((Jxta_test_adv *) ad)->NameAttr2);
        return res;
    }

    JXTA_DECLARE(Jxta_status) jxta_test_adv_set_Name(Jxta_test_adv * ad, const char *val) {

        if (val != NULL) {
            ad->Name = calloc(1, strlen(val) + 1);
            strcpy(ad->Name, val);
        } else {
            return JXTA_INVALID_ARGUMENT;
        }

        return JXTA_SUCCESS;
    }

    char *JXTA_STDCALL jxta_test_adv_handle_parm(Jxta_advertisement * adv, const char *test) {
        if (adv) {
        }
        printf("got the message back in the advertisement with parm ---- %s \n", (char *) test);
        return strdup(test);
    }

    /** Now, build an array of the keyword structs.  Since
     * a top-level, or null state may be of interest, 
     * let that lead off.  Then, walk through the enums,
     * initializing the struct array with the correct fields.
     * Later, the stream will be dispatched to the handler based
     * on the value in the char * kwd.
     */

    static const Kwdtab TestAdvertisement_tags[] = {
        {"Null", Null_, NULL, NULL, NULL},
        {"demo:TestAdvertisement", TestAdvertisement_, *handleTestAdvertisement, NULL, NULL},
        {"testId", Id_, *handleId, jxta_test_adv_get_Id_string, NULL},
        {"testId IdAttribute", IdAttr_, *handleIdAttr, jxta_test_adv_get_idattr_string, NULL},
        {"testId IdAttr1", IdAttr1_, *handleIdAttr1, jxta_test_adv_get_idattr1_string, NULL},
        {"Type", Type_, *handleType, jxta_test_adv_get_Type_string, NULL},
        {"Name", Name_, *handleName, jxta_test_adv_get_Name_string, NULL},
        {"Name NameAttr1", NameAttr1_, *handleNameAttr1, jxta_test_adv_get_nameattr1_string, NULL},
        {"Name NameAttr2", NameAttr1_, *handleNameAttr2, NULL, jxta_test_adv_handle_parm},
        {"Name NameAttr3", NameAttr1_, *handleNameAttr1, NULL, jxta_test_adv_handle_parm},
        {"Name NameAttr4", NameAttr1_, *handleNameAttr1, NULL, jxta_test_adv_handle_parm},
        {NULL, 0, 0, NULL, NULL}
    };


    JXTA_DECLARE(Jxta_status) jxta_test_adv_get_xml(Jxta_test_adv * ad, JString ** xml) {
        JString *string;

        if (xml == NULL) {
            return JXTA_INVALID_ARGUMENT;
        }

        string = jstring_new_0();

        jstring_append_2(string, "<?xml version=\"1.0\"?>\n");
        jstring_append_2(string, "<!DOCTYPE demo:TestAdvertisement>\n");
        jstring_append_2(string, "<demo:TestAdvertisement xmlns:demo=\"http://jxta.org\" type=\"demo:TestAdvertisement\">\n");
        jstring_append_2(string, "<testId IdAttribute=\"");
        jstring_append_2(string, jxta_test_adv_get_IdAttr((Jxta_test_adv *) ad));
        jstring_append_2(string, "\" ");
        jstring_append_2(string, " IdAttr1=\"");
        jstring_append_2(string, jxta_test_adv_get_IdAttr1((Jxta_test_adv *) ad));
        jstring_append_2(string, "\">");
        jstring_append_2(string, jxta_test_adv_get_Id((Jxta_test_adv *) ad));
        jstring_append_2(string, "</testId>\n");
        jstring_append_2(string, "<Type>");
        jstring_append_2(string, jxta_test_adv_get_Type((Jxta_test_adv *) ad));
        jstring_append_2(string, "</Type>\n");
        jstring_append_2(string, "<Name NameAttr1=\"");
        jstring_append_2(string, jxta_test_adv_get_NameAttr1(ad));
        jstring_append_2(string, "\"");
        jstring_append_2(string, " NameAttr2=\"");
        jstring_append_2(string, jxta_test_adv_get_NameAttr1(ad));
        jstring_append_2(string, "\"");
        jstring_append_2(string, " NameAttr3=\"");
        jstring_append_2(string, jxta_test_adv_get_NameAttr1(ad));
        jstring_append_2(string, "\"");
        jstring_append_2(string, " NameAttr4=\"");
        jstring_append_2(string, jxta_test_adv_get_NameAttr1(ad));
        jstring_append_2(string, "\">");
        jstring_append_2(string, jxta_test_adv_get_Name((Jxta_test_adv *) ad));
        jstring_append_2(string, "</Name>\n");
        jstring_append_2(string, "</demo:TestAdvertisement>\n");
        *xml = string;
        return JXTA_SUCCESS;
    }

    static void
     jxta_test_adv_delete(Jxta_test_adv * ad) {
        /* Fill in the required freeing functions here. */
        if (ad->Id) {
            free(ad->Id);
            ad->Id = NULL;
        }

        if (ad->Type) {
            free(ad->Type);
            ad->Type = NULL;
        }
        if (ad->Name) {
            free(ad->Name);
            ad->Name = NULL;
        }
        if (ad->NameAttr1) {
            free(ad->NameAttr1);
        }
        if (ad->IdAttr) {
            free(ad->IdAttr);
        }
        if (ad->IdAttr1) {
            free(ad->IdAttr1);
        }
        if (ad->NameAttr2) {
            free(ad->NameAttr2);
        }
        jxta_advertisement_delete((Jxta_advertisement *) ad);
        memset(ad, 0x00, sizeof(Jxta_test_adv));
        free(ad);
    }

    /** Get a new instance of the ad.
     * The memory gets shredded going in to 
     * a value that is easy to see in a debugger,
     * just in case there is a segfault (not that 
     * that would ever happen, but in case it ever did.)
     */
    JXTA_DECLARE(Jxta_test_adv *) jxta_test_adv_new(void) {

        Jxta_test_adv *ad;
        ad = (Jxta_test_adv *) calloc(1, sizeof(Jxta_test_adv));


        JXTA_OBJECT_INIT((Jxta_advertisement *) ad, jxta_test_adv_delete, 0);
        return jxta_test_adv_construct(ad);

    }

    static Jxta_test_adv *jxta_test_adv_construct(Jxta_test_adv * ad) {

        jxta_advertisement_construct((Jxta_advertisement *) ad,
                                     "demo:TestAdvertisement",
                                     TestAdvertisement_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_test_adv_get_xml,
                                     (JxtaAdvertisementGetIDFunc) jxta_test_adv_get_testid,
                                     (JxtaAdvertisementGetIndexFunc) jxta_test_adv_get_indexes);

        ad->Id = NULL;
        ad->Type = NULL;
        ad->Name = NULL;
        ad->NameAttr1 = NULL;
        ad->NameAttr2 = NULL;
        ad->IdAttr = NULL;
        ad->IdAttr1 = NULL;
        return ad;
    }

    /** Shred the memory going out.  Again,
     * if there ever was a segfault (unlikely,
     * of course), the hex value dddddddd will 
     * pop right out as a piece of memory accessed
     * after it was freed...
     */

    JXTA_DECLARE(void)
        jxta_test_adv_parse_charbuffer(Jxta_test_adv * ad, const char *buf, int len) {

        jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
    }

    JXTA_DECLARE(void)
     jxta_test_adv_parse_file(Jxta_test_adv * ad, FILE * stream) {

        jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
    }

    void jxta_test_advertisement_idx_delete(Jxta_object * jo) {
        Jxta_index *ji = (Jxta_index *) jo;
        JXTA_OBJECT_RELEASE(ji->element);
        if (ji->attribute != NULL) {
            JXTA_OBJECT_RELEASE(ji->attribute);
        }

        memset(ji, 0xdd, sizeof(Jxta_index));
        free(ji);
    }

    static
    Jxta_vector *jxta_test_advertisement_return_indexes(const char *idx[]) {
        JString *element;
        JString *attribute;

        int i = 0;
        Jxta_vector *ireturn;
        ireturn = jxta_vector_new(0);
        for (i = 0; idx[i] != NULL; i += 3) {
            Jxta_index *jIndex = (Jxta_index *) calloc(1, sizeof(Jxta_index));
            JXTA_OBJECT_INIT(jIndex, (JXTA_OBJECT_FREE_FUNC) jxta_test_advertisement_idx_delete, 0);
            element = jstring_new_2((char *) idx[i]);
            if (idx[i + 1] != NULL) {
                attribute = jstring_new_2((char *) idx[i + 1]);
            } else {
                attribute = NULL;
            }
            jIndex->element = element;
            jIndex->attribute = attribute;
            jIndex->parm = (char *) idx[i + 2];
            jxta_vector_add_object_last(ireturn, (Jxta_object *) jIndex);
            JXTA_OBJECT_RELEASE((Jxta_object *) jIndex);
        }
        return ireturn;
    }

    JXTA_DECLARE(Jxta_vector *)
        jxta_test_adv_get_indexes(Jxta_advertisement * dummy) {
        static const char *idx[][3] = {
            {"Name", NULL, "1"},
            {"Name", "NameAttr1", "2"},
            {"Name", "NameAttr2", "2"},
            {"Name", "NameAttr3", "3"},
            {"testId", NULL, "3"},
            {"testId", "IdAttribute", "4"},
            {"testId", "IdAttr1", "5"},
            {NULL, NULL, NULL}
        };
        return jxta_test_advertisement_return_indexes(idx[0]);
    }


#ifdef STANDALONE
    int
     main(int argc, char **argv) {
        Jxta_test_adv *ad;
        FILE *testfile;

        if (argc != 2) {
            printf("usage: ad <filename>\n");
            return -1;
        }

        ad = jxta_test_adv_new();

        testfile = fopen(argv[1], "r");
        jxta_test_adv_parse_file(ad, testfile);
        fclose(testfile);

        /* Jxta_test_adv_print_xml(ad,fprintf,stdout); */
        jxta_test_adv_delete(ad);

        return 0;
    }
#endif
#ifdef __cplusplus
}
#endif
