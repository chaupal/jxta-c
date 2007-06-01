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
 * $Id: jxta_mca.c,v 1.23 2005/08/03 05:51:16 slowhog Exp $
 */


/*
* The following command will compile the output from the script 
* given the apr is installed correctly.
*/
/*
* gcc -DSTANDALONE jxta_advertisement.c MCA.c  -o PA \
  `/usr/local/apache2/bin/apr-config --cflags --includes --libs` \
  -lexpat -L/usr/local/apache2/lib/ -lapr
*/

#include <stdio.h>
#include <string.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_mca.h"

#ifdef __cplusplus
extern "C" {
#endif

    /** Each of these corresponds to a tag in the
     * xml ad.
     */
    enum tokentype {
        Null_,
        MCA_,
        MCID_,
        Name_,
        Desc_
    };


    /** This is the representation of the
     * actual ad in the code.  It should
     * stay opaque to the programmer, and be 
     * accessed through the get/set API.
     */
    struct _MCA {
        Jxta_advertisement jxta_advertisement;
        char *MCA;
        char *MCID;
        char *Name;
        char *Desc;
    };


    /** Handler functions.  Each of these is responsible for
     * dealing with all of the character data associated with the 
     * tag name.
     */
    static void
     handleMCA(void *userdata, const XML_Char * cd, int len) {
        /* MCA * ad = (MCA*)userdata; */
        JXTA_LOG("In MCA element\n");
    } static void
     handleMCID(void *userdata, const XML_Char * cd, int len) {
        /* MCA * ad = (MCA*)userdata; */
        JXTA_LOG("In MCID element\n");
    }

    static void
     handleName(void *userdata, const XML_Char * cd, int len) {
        /* MCA * ad = (MCA*)userdata; */
        JXTA_LOG("In Name element\n");
    }

    static void
     handleDesc(void *userdata, const XML_Char * cd, int len) {
        /* MCA * ad = (MCA*)userdata; */
        JXTA_LOG("In Desc element\n");
    }




    /** The get/set functions represent the public
     * interface to the ad class, that is, the API.
     */
    JXTA_DECLARE(char *) MCA_get_MCA(MCA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MCA_set_MCA(MCA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MCA_get_MCID(MCA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MCA_set_MCID(MCA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MCA_get_Name(MCA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MCA_set_Name(MCA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MCA_get_Desc(MCA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MCA_set_Desc(MCA * ad, char *name) {
    }

    /** Now, build an array of the keyword structs.  Since
     * a top-level, or null state may be of interest, 
     * let that lead off.  Then, walk through the enums,
     * initializing the struct array with the correct fields.
     * Later, the stream will be dispatched to the handler based
     * on the value in the char * kwd.
     */
    static const Kwdtab MCA_tags[] = {
        {"Null", Null_, NULL, NULL, NULL},
        {"jxta:MCA", MCA_, *handleMCA, NULL, NULL},
        {"MCID", MCID_, *handleMCID, NULL, NULL},
        {"Name", Name_, *handleName, NULL, NULL},
        {"Desc", Desc_, *handleDesc, NULL, NULL},
        {NULL, 0, 0, NULL, NULL}
    };


    /**
     * @todo  Return an appropriate value.
     */
    JXTA_DECLARE(Jxta_status) MCA_get_xml(MCA * ad, JString ** string) {

        /*
           printer(stream,"<MCA>\n");
           printer(stream,"<MCID>%s</MCID>\n",MCA_get_MCID(ad));
           printer(stream,"<Name>%s</Name>\n",MCA_get_Name(ad));
           printer(stream,"<Desc>%s</Desc>\n",MCA_get_Desc(ad));
           printer(stream,"</MCA>\n");
         */
        return JXTA_NOTIMP;

    }


    /** Get a new instance of the ad.
     * The memory gets shredded going in to 
     * a value that is easy to see in a debugger,
     * just in case there is a segfault (not that 
     * that would ever happen, but in case it ever did.)
     */
    JXTA_DECLARE(MCA *) MCA_new() {

        MCA *ad;
        ad = (MCA *) malloc(sizeof(MCA));
        memset(ad, 0xda, sizeof(MCA));


        jxta_advertisement_initialize((Jxta_advertisement *) ad, "jxta:MCA", MCA_tags, (JxtaAdvertisementGetXMLFunc) MCA_get_xml,
                                      /* XXX Fixme MCA_get_MCID is not implemented nor has the correct sig.
                                         when it is fixed this needs to be corrected.  it's NULL for now */
                                      NULL, (JxtaAdvertisementGetIndexFunc) MCA_get_indexes, (FreeFunc) MCA_delete);

        return ad;
    }

    /** Shred the memory going out.  Again,
     * if there ever was a segfault (unlikely,
     * of course), the hex value dddddddd will 
     * pop right out as a piece of memory accessed
     * after it was freed...
     */
    void
     MCA_delete(MCA * ad) {
        /* Fill in the required freeing functions here. */

        memset(ad, 0xdd, sizeof(MCA));
        free(ad);
    }

    JXTA_DECLARE(void)
     MCA_parse_charbuffer(MCA * ad, const char *buf, int len) {

        jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
    }

    JXTA_DECLARE(void)
     MCA_parse_file(MCA * ad, FILE * stream) {

        jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
    }

    JXTA_DECLARE(Jxta_vector *) MCA_get_indexes(Jxta_advertisement * dummy) {
        const char *idx[][2] = {
            {"Name", NULL},
            {NULL, NULL}
        };
        return jxta_advertisement_return_indexes(idx[0]);
    }

#ifdef STANDALONE
    int
     main(int argc, char **argv) {
        MCA *ad;
        FILE *testfile;

        if (argc != 2) {
            printf("usage: ad <filename>\n");
            return -1;
        }

        ad = MCA_new();

        testfile = fopen(argv[1], "r");
        MCA_parse_file(ad, testfile);
        fclose(testfile);

        /* MCA_print_xml(ad,fprintf,stdout); */
        MCA_delete(ad);

        return 0;
    }
#endif
#ifdef __cplusplus
}
#endif
