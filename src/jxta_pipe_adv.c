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
 * $Id: jxta_pipe_adv.c,v 1.16 2005/02/02 02:58:30 exocetrick Exp $
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
#include "jxta_pipe_adv.h"
#include "jxta_xml_util.h"

#ifdef __cplusplus
extern "C" {
#endif

    /** Each of these corresponds to a tag in the
     * xml ad.
     */
    enum tokentype {
        Null_,
        PipeAdvertisement_,
        Id_,
        Type_,
        Name_
    };


    /** This is the representation of the
     * actual ad in the code.  It should
     * stay opaque to the programmer, and be 
     * accessed through the get/set API.
     */
    struct _jxta_pipe_adv {
        Jxta_advertisement jxta_advertisement;
        char *Id;
        char *Type;
        char *Name;
    };

    extern void jxta_pipe_adv_delete(Jxta_pipe_adv *);

    /** Handler functions.  Each of these is responsible for
     * dealing with all of the character data associated with the 
     * tag name.
     */
    static void
     handlePipeAdvertisement(void *userdata, const XML_Char * cd, int len) {
        /* Jxta_pipe_adv * ad = (Jxta_pipe_adv*)userdata; */
    } static void
     handleId(void *userdata, const XML_Char * cd, int len) {
        Jxta_pipe_adv *ad = (Jxta_pipe_adv *) userdata;

        char *tok = malloc(len + 1);
        memset(tok, 0, len + 1);

        extract_char_data(cd, len, tok);

        if (strlen(tok) != 0) {
            ad->Id = tok;
        } else {
            free(tok);
        }
    }

    static void
     handleType(void *userdata, const XML_Char * cd, int len) {

        Jxta_pipe_adv *ad = (Jxta_pipe_adv *) userdata;

        char *tok = malloc(len + 1);
        memset(tok, 0, len + 1);

        extract_char_data(cd, len, tok);

        if (strlen(tok) != 0) {
            ad->Type = tok;
        } else {
            free(tok);
        }
    }

    static void
     handleName(void *userdata, const XML_Char * cd, int len) {

        Jxta_pipe_adv *ad = (Jxta_pipe_adv *) userdata;

        char *tok = malloc(len + 1);
        memset(tok, 0, len + 1);

        extract_char_data(cd, len, tok);

        if (strlen(tok) != 0) {
            ad->Name = tok;
        } else {
            free(tok);
        }
    }

    /** The get/set functions represent the public
     * interface to the ad class, that is, the API.
     */

    Jxta_id *jxta_pipe_adv_get_pipeid(Jxta_pipe_adv * ad) {
        Jxta_id *pipeid = NULL;
        JString *tmps = jstring_new_2(ad->Id);
        jxta_id_from_jstring(&pipeid, tmps);
        JXTA_OBJECT_RELEASE(tmps);
        return pipeid;
    }

    const char *jxta_pipe_adv_get_Id(Jxta_pipe_adv * ad) {
        return ad->Id;
    }

    char *jxta_pipe_adv_get_Id_string(Jxta_advertisement * ad) {

        char *res;
        res = strdup(((Jxta_pipe_adv *) ad)->Id);
        return res;
    }

    Jxta_status jxta_pipe_adv_set_Id(Jxta_pipe_adv * ad, const char *val) {

        if (val != NULL) {
            ad->Id = malloc(strlen(val) + 1);
            strcpy(ad->Id, val);
        } else {
            return JXTA_INVALID_ARGUMENT;
        }

        return JXTA_SUCCESS;
    }

    const char *jxta_pipe_adv_get_Type(Jxta_pipe_adv * ad) {
        return ad->Type;
    }

    char *jxta_pipe_adv_get_Type_string(Jxta_advertisement * ad) {
        return strdup(((Jxta_pipe_adv *) ad)->Type);

    }

    Jxta_status jxta_pipe_adv_set_Type(Jxta_pipe_adv * ad, const char *val) {

        if (val != NULL) {
            ad->Type = malloc(strlen(val) + 1);
            strcpy(ad->Type, val);
        } else {
            return JXTA_INVALID_ARGUMENT;
        }

        return JXTA_SUCCESS;
    }


    const char *jxta_pipe_adv_get_Name(Jxta_pipe_adv * ad) {
        return ad->Name;
    }

    char *jxta_pipe_adv_get_Name_string(Jxta_advertisement * ad) {
        return strdup(((Jxta_pipe_adv *) ad)->Name);

    }

    Jxta_status jxta_pipe_adv_set_Name(Jxta_pipe_adv * ad, const char *val) {

        if (val != NULL) {
            ad->Name = malloc(strlen(val) + 1);
            strcpy(ad->Name, val);
        } else {
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
    const static Kwdtab PipeAdvertisement_tags[] = {
        {"Null", Null_, NULL, NULL},
        {"jxta:PipeAdvertisement", PipeAdvertisement_, *handlePipeAdvertisement, NULL},
        {"Id", Id_, *handleId, jxta_pipe_adv_get_Id_string},
        {"Type", Type_, *handleType, jxta_pipe_adv_get_Type_string},
        {"Name", Name_, *handleName, jxta_pipe_adv_get_Name_string},
        {NULL, 0, 0, NULL}
    };


    Jxta_status jxta_pipe_adv_get_xml(Jxta_pipe_adv * ad, JString ** xml) {
        JString *string;

        if (xml == NULL) {
            return JXTA_INVALID_ARGUMENT;
        }

        string = jstring_new_0();

        jstring_append_2(string, "<?xml version=\"1.0\"?>\n");
        jstring_append_2(string, "<!DOCTYPE jxta:PipeAdvertisement>");

        jstring_append_2(string, "<jxta:PipeAdvertisement>\n");
        jstring_append_2(string, "<Id>");
        jstring_append_2(string, jxta_pipe_adv_get_Id(ad));
        jstring_append_2(string, "</Id>\n");
        jstring_append_2(string, "<Type>");
        jstring_append_2(string, jxta_pipe_adv_get_Type(ad));
        jstring_append_2(string, "</Type>\n");
        jstring_append_2(string, "<Name>");
        jstring_append_2(string, jxta_pipe_adv_get_Name(ad));
        jstring_append_2(string, "</Name>\n");

        jstring_append_2(string, "</jxta:PipeAdvertisement>\n");
        *xml = string;
        return JXTA_SUCCESS;
    }


    /** Get a new instance of the ad.
     * The memory gets shredded going in to 
     * a value that is easy to see in a debugger,
     * just in case there is a segfault (not that 
     * that would ever happen, but in case it ever did.)
     */
    Jxta_pipe_adv *jxta_pipe_adv_new(void) {

        Jxta_pipe_adv *ad;
        ad = (Jxta_pipe_adv *) malloc(sizeof(Jxta_pipe_adv));
        memset(ad, 0x0, sizeof(Jxta_pipe_adv));

        /*
           JXTA_OBJECT_INIT((Jxta_advertisement*)ad,jxta_pipe_adv_delete, 0);
         */

        jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                      "jxta:PipeAdvertisement",
                                      PipeAdvertisement_tags,
                                      (JxtaAdvertisementGetXMLFunc)jxta_pipe_adv_get_xml,
                                      (JxtaAdvertisementGetIDFunc) jxta_pipe_adv_get_pipeid,
				      (JxtaAdvertisementGetIndexFunc)jxta_pipe_adv_get_indexes,
                                      (FreeFunc)jxta_pipe_adv_delete);


        ad->Id = NULL;
        ad->Type = NULL;
        ad->Name = NULL;

        return ad;
    }

    /** Shred the memory going out.  Again,
     * if there ever was a segfault (unlikely,
     * of course), the hex value dddddddd will 
     * pop right out as a piece of memory accessed
     * after it was freed...
     */
    void
     jxta_pipe_adv_delete(Jxta_pipe_adv * ad) {
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

        jxta_advertisement_delete((Jxta_advertisement *) ad);
        memset(ad, 0x00, sizeof(Jxta_pipe_adv));
        free(ad);
    }

    void
     jxta_pipe_adv_parse_charbuffer(Jxta_pipe_adv * ad, const char *buf, int len) {

        jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
    }



    void
     jxta_pipe_adv_parse_file(Jxta_pipe_adv * ad, FILE * stream) {

        jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
    }
    
    Jxta_vector * 
    jxta_pipe_adv_get_indexes(void) {
        static const char * idx[][2] = { 
					{ "Name", NULL },
					{ "Id", NULL },
        				{ NULL, NULL }
					};
    return jxta_advertisement_return_indexes(idx);
}


#ifdef STANDALONE
    int
     main(int argc, char **argv) {
        Jxta_pipe_adv *ad;
        FILE *testfile;

        if (argc != 2) {
            printf("usage: ad <filename>\n");
            return -1;
        }

        ad = jxta_pipe_adv_new();

        testfile = fopen(argv[1], "r");
        jxta_pipe_adv_parse_file(ad, testfile);
        fclose(testfile);

        /* Jxta_pipe_adv_print_xml(ad,fprintf,stdout); */
        jxta_pipe_adv_delete(ad);

        return 0;
    }
#endif
#ifdef __cplusplus
}
#endif
