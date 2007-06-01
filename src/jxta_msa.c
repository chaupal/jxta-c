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
 * $Id: jxta_msa.c,v 1.21 2005/08/03 05:51:16 slowhog Exp $
 */


/*
* The following command will compile the output from the script 
* given the apr is installed correctly.
*/
/*
* gcc -DSTANDALONE jxta_advertisement.c MSA.c  -o PA \
  `/usr/local/apache2/bin/apr-config --cflags --includes --libs` \
  -lexpat -L/usr/local/apache2/lib/ -lapr
*/

#include <stdio.h>
#include <string.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_msa.h"

#ifdef __cplusplus
extern "C" {
#endif

    /** Each of these corresponds to a tag in the
     * xml ad.
     */
    enum tokentype {
        Null_,
        MSA_,
        MSID_,
        Name_,
        Crtr_,
        SURI_,
        Vers_,
        Desc_,
        Parm_,
        PipeAdvertisement_,
        Proxy_,
        Auth_
    };


    /** This is the representation of the
     * actual ad in the code.  It should
     * stay opaque to the programmer, and be 
     * accessed through the get/set API.
     */
    struct _MSA {
        Jxta_advertisement jxta_advertisement;
        char *MSA;
        char *MSID;
        char *Name;
        char *Crtr;
        char *SURI;
        char *Vers;
        char *Desc;
        char *Parm;
        char *PipeAdvertisement;
        char *Proxy;
        char *Auth;
    };


    /** Handler functions.  Each of these is responsible for
     * dealing with all of the character data associated with the 
     * tag name.
     */
    static void
     handleMSA(void *userdata, const XML_Char * cd, int len) {
        /* MSA * ad = (MSA*)userdata; */
        JXTA_LOG("In MSA element\n");
    } static void
     handleMSID(void *userdata, const XML_Char * cd, int len) {
        /* MSA * ad = (MSA*)userdata; */
        JXTA_LOG("In MSID element\n");
    }

    static void
     handleName(void *userdata, const XML_Char * cd, int len) {
        /* MSA * ad = (MSA*)userdata; */
        JXTA_LOG("In Name element\n");
    }

    static void
     handleCrtr(void *userdata, const XML_Char * cd, int len) {
        /* MSA * ad = (MSA*)userdata; */
        JXTA_LOG("In Crtr element\n");
    }

    static void
     handleSURI(void *userdata, const XML_Char * cd, int len) {
        /* MSA * ad = (MSA*)userdata; */
        JXTA_LOG("In SURI element\n");
    }

    static void
     handleVers(void *userdata, const XML_Char * cd, int len) {
        /* MSA * ad = (MSA*)userdata; */
        JXTA_LOG("In Vers element\n");
    }

    static void
     handleDesc(void *userdata, const XML_Char * cd, int len) {
        /* MSA * ad = (MSA*)userdata; */
        JXTA_LOG("In Desc element\n");
    }

    static void
     handleParm(void *userdata, const XML_Char * cd, int len) {
        /* MSA * ad = (MSA*)userdata; */
        JXTA_LOG("In Parm element\n");
    }

    static void
     handlePipeAdvertisement(void *userdata, const XML_Char * cd, int len) {
        /* MSA * ad = (MSA*)userdata; */
        JXTA_LOG("In PipeAdvertisement element\n");
    }

    static void
     handleProxy(void *userdata, const XML_Char * cd, int len) {
        /* MSA * ad = (MSA*)userdata; */
        JXTA_LOG("In Proxy element\n");
    }

    static void
     handleAuth(void *userdata, const XML_Char * cd, int len) {
        /* MSA * ad = (MSA*)userdata; */
        JXTA_LOG("In Auth element\n");
    }



    /** The get/set functions represent the public
     * interface to the ad class, that is, the API.
     */
    JXTA_DECLARE(char *) MSA_get_MSA(MSA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MSA_set_MSA(MSA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MSA_get_MSID(MSA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MSA_set_MSID(MSA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MSA_get_Name(MSA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MSA_set_Name(MSA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MSA_get_Crtr(MSA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MSA_set_Crtr(MSA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MSA_get_SURI(MSA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MSA_set_SURI(MSA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MSA_get_Vers(MSA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MSA_set_Vers(MSA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MSA_get_Desc(MSA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MSA_set_Desc(MSA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MSA_get_Parm(MSA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MSA_set_Parm(MSA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MSA_get_PipeAdvertisement(MSA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MSA_set_PipeAdvertisement(MSA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MSA_get_Proxy(MSA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MSA_set_Proxy(MSA * ad, char *name) {
    }

    JXTA_DECLARE(char *) MSA_get_Auth(MSA * ad) {
        return NULL;
    }

    JXTA_DECLARE(void)
     MSA_set_Auth(MSA * ad, char *name) {
    }



    /** Now, build an array of the keyword structs.  Since
     * a top-level, or null state may be of interest, 
     * let that lead off.  Then, walk through the enums,
     * initializing the struct array with the correct fields.
     * Later, the stream will be dispatched to the handler based
     * on the value in the char * kwd.
     */
    static const Kwdtab MSA_tags[] = {
        {"Null", Null_, NULL, NULL, NULL},
        {"jxta:MSA", MSA_, *handleMSA, NULL, NULL},
        {"MSID", MSID_, *handleMSID, NULL, NULL},
        {"Name", Name_, *handleName, NULL, NULL},
        {"Crtr", Crtr_, *handleCrtr, NULL, NULL},
        {"SURI", SURI_, *handleSURI, NULL, NULL},
        {"Vers", Vers_, *handleVers, NULL, NULL},
        {"Desc", Desc_, *handleDesc, NULL, NULL},
        {"Parm", Parm_, *handleParm, NULL, NULL},
        {"PipeAdvertisement", PipeAdvertisement_, *handlePipeAdvertisement, NULL, NULL},
        {"Proxy", Proxy_, *handleProxy, NULL, NULL},
        {"Auth", Auth_, *handleAuth, NULL, NULL},
        {NULL, 0, 0, NULL, NULL}
    };


    JXTA_DECLARE(Jxta_status) MSA_get_xml(MSA * ad, JString ** string) {

        /*
           printer(stream,"<jxta:MSA>\n");
           printer(stream,"<MSID>%s</MSID>\n",MSA_get_MSID(ad));
           printer(stream,"<Name>%s</Name>\n",MSA_get_Name(ad));
           printer(stream,"<Crtr>%s</Crtr>\n",MSA_get_Crtr(ad));
           printer(stream,"<SURI>%s</SURI>\n",MSA_get_SURI(ad));
           printer(stream,"<Vers>%s</Vers>\n",MSA_get_Vers(ad));
           printer(stream,"<Desc>%s</Desc>\n",MSA_get_Desc(ad));
           printer(stream,"<Parm>%s</Parm>\n",MSA_get_Parm(ad));
           printer(stream,"<PipeAdvertisement>%s</PipeAdvertisement>\n",MSA_get_PipeAdvertisement(ad));
           printer(stream,"<Proxy>%s</Proxy>\n",MSA_get_Proxy(ad));
           printer(stream,"<Auth>%s</Auth>\n",MSA_get_Auth(ad));
           printer(stream,"</MSA>\n");
         */

        return JXTA_NOTIMP;
    }



    /** Get a new instance of the ad.
     * The memory gets shredded going in to 
     * a value that is easy to see in a debugger,
     * just in case there is a segfault (not that 
     * that would ever happen, but in case it ever did.)
     */
    JXTA_DECLARE(MSA *) MSA_new() {

        MSA *ad;
        ad = (MSA *) malloc(sizeof(MSA));
        memset(ad, 0xda, sizeof(MSA));

        /*
           JXTA_OBJECT_INIT((Jxta_advertisement*)ad,MSA_delete, 0);
         */

        jxta_advertisement_initialize((Jxta_advertisement *) ad, "jxta:MSA", MSA_tags, (JxtaAdvertisementGetXMLFunc) MSA_get_xml,
                                      /* XXX Fixme MSA_get_MSID is not implemented nor has the correct sig.
                                         when it is fixed this needs to be corrected.  it's NULL for now
                                       */
                                      NULL, (JxtaAdvertisementGetIndexFunc) MSA_get_indexes, (FreeFunc) MSA_delete);

        return ad;
    }

    /** Shred the memory going out.  Again,
     * if there ever was a segfault (unlikely,
     * of course), the hex value dddddddd will 
     * pop right out as a piece of memory accessed
     * after it was freed...
     */
    void
     MSA_delete(MSA * ad) {
        /* Fill in the required freeing functions here. */

        memset(ad, 0xdd, sizeof(MSA));
        free(ad);
    }

    JXTA_DECLARE(void)
     MSA_parse_charbuffer(MSA * ad, const char *buf, int len) {

        jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
    }



    JXTA_DECLARE(void)
     MSA_parse_file(MSA * ad, FILE * stream) {

        jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
    }

    JXTA_DECLARE(Jxta_vector *) MSA_get_indexes(Jxta_advertisement * dummy) {
        const char *idx[][2] = {
            {"Name", NULL},
            {NULL, NULL}
        };
        return jxta_advertisement_return_indexes(idx[0]);
    }

#ifdef STANDALONE
    int
     main(int argc, char **argv) {
        MSA *ad;
        FILE *testfile;

        if (argc != 2) {
            printf("usage: ad <filename>\n");
            return -1;
        }

        ad = MSA_new();

        testfile = fopen(argv[1], "r");
        MSA_parse_file(ad, testfile);
        fclose(testfile);

        /* MSA_print_xml(ad,fprintf,stdout); */
        MSA_delete(ad);

        return 0;
    }
#endif
#ifdef __cplusplus
}
#endif
