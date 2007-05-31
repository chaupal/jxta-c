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
 * $Id: dr_adv_test.c,v 1.15 2005/01/12 20:52:37 bondolo Exp $
 */
 
#include <stdio.h>
#include "jxta_dr.h"
#include "jxta_pa.h"

boolean
dr_type_test(Jxta_DiscoveryResponse * ad, short truetype) {

    boolean passed = TRUE;

    short type = jxta_discovery_response_get_type(ad);
    if (type == truetype) {
        fprintf(stderr,"Passed dr_type_test\n");
    } else {
        fprintf(stderr,"Failed dr_type_test %d\n", type);
        passed = FALSE;
    }

    return passed;
}

boolean
dr_count_test(Jxta_DiscoveryResponse * ad, int truecount) {

    boolean passed = TRUE;

    int count = jxta_discovery_response_get_count(ad);
    if (count == truecount) {
        fprintf(stderr,"Passed dr_count_test\n");
    } else {
        fprintf(stderr,"Failed dr_count_test %d\n", count);
        passed = FALSE;
    }

    return passed;
}

boolean
dr_response_test(Jxta_DiscoveryResponse * ad) {
    int count = jxta_discovery_response_get_count(ad);
    int i = 0;
    boolean passed = FALSE;
    Jxta_vector * responses = NULL;
    Jxta_vector * advertisements = NULL;
    Jxta_advertisement * adv = NULL;
    JString* id_string = NULL;
    JString* adv_string = NULL;
    Jxta_id* id = NULL;
    
    jxta_discovery_response_get_responses(ad, &responses);
    jxta_discovery_response_get_advertisements(ad, &advertisements);
    for (i =0; i< jxta_vector_size(advertisements) ; i++) {
        jxta_vector_get_object_at(advertisements,
                                 (Jxta_object**) &adv,
                                  i);
        jxta_advertisement_get_xml(adv, &adv_string);
        fprintf(stdout,"%s\n",jstring_get_string(adv_string));
        id = jxta_advertisement_get_id(adv);
        jxta_id_to_jstring(id,&id_string);
        printf("Advertisement ID=%s\n",jstring_get_string(id_string));
    }
    if (jxta_vector_size(responses) == count && jxta_vector_size(advertisements) == count) {
        passed = TRUE;
        fprintf(stderr,"Passed dr_response_test\n");
    } else {
        fprintf(stderr,"Failed dr_response_test count =%d, response count =%d parsed count =%d\n", count, jxta_vector_size(responses),jxta_vector_size(advertisements));
    }
    return passed;
}


boolean
dr_pa_test(Jxta_DiscoveryResponse * ad) {

    boolean passed = TRUE;
    Jxta_PA * pa = jxta_PA_new();
    Jxta_advertisement *padv = jxta_advertisement_new();
    JString * js = jstring_new_0();
    JString * pabuf = NULL;

    jxta_discovery_response_get_peer_advertisement(ad, &padv);
    jxta_discovery_response_get_peeradv(ad, &pabuf);
    jxta_PA_parse_charbuffer(pa, jstring_get_string (pabuf) , jstring_length(pabuf));
    jxta_PA_get_xml(pa, &js);
    fprintf(stdout,"%s\n",jstring_get_string(js));
    return passed;
}



boolean
dr_adv_test(int argc, char ** argv) {

    Jxta_DiscoveryResponse * ad = jxta_discovery_response_new();
    FILE *testfile;
    JString * js = jstring_new_0();
    char * pabuf;

    if(argc != 2) {
        printf("usage: ad <filename>\n");
        return -1;
    }

    testfile = fopen (argv[1], "r");
    jxta_discovery_response_parse_file(ad, testfile);
    fclose(testfile);
    dr_type_test(ad, 2);
    dr_count_test(ad, 3);
    dr_pa_test(ad);
    dr_response_test(ad);
    jxta_discovery_response_get_xml(ad, &js);
    fprintf(stdout,"%s\n",jstring_get_string(js));
    jxta_discovery_response_free(ad);

    return FALSE;
}

int
main (int argc, char **argv) {

    int retval;

    jxta_initialize();
    retval = dr_adv_test(argc,argv);
    jxta_terminate();
    return retval;
}
