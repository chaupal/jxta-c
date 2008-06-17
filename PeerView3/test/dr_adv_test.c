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
 * $Id$
 */

#include <stdio.h>
#include "jxta.h"
#include "jxta_dr.h"
#include "jxta_pa.h"
#include "jxta_pipe_adv.h"

Jxta_boolean dr_type_test(Jxta_DiscoveryResponse * ad, short truetype)
{

    Jxta_boolean passed = TRUE;

    short type = jxta_discovery_response_get_type(ad);
    if (type == truetype) {
        printf("Passed dr_type_test\n");
    } else {
        printf("Failed dr_type_test %d\n", type);
        passed = FALSE;
    }

    return passed;
}

Jxta_boolean dr_count_test(Jxta_DiscoveryResponse * ad, int truecount)
{

    Jxta_boolean passed = TRUE;

    int count = jxta_discovery_response_get_count(ad);
    if (count == truecount) {
        printf("Passed dr_count_test\n");
    } else {
        printf("Failed dr_count_test %d\n", count);
        passed = FALSE;
    }

    return passed;
}

Jxta_boolean dr_response_test(Jxta_DiscoveryResponse * ad)
{
    unsigned int count;
    unsigned int i = 0;

    Jxta_boolean passed = FALSE;
    Jxta_vector *responses = NULL;
    Jxta_vector *advertisements = NULL;
    Jxta_advertisement *adv = NULL;
    JString *id_string = NULL;
    JString *adv_string = NULL;
    Jxta_id *id = NULL;

    count = jxta_discovery_response_get_count(ad);
    jxta_discovery_response_get_responses(ad, &responses);
    jxta_discovery_response_get_advertisements(ad, &advertisements);
    for (i = 0; i < jxta_vector_size(advertisements); i++) {
        jxta_vector_get_object_at(advertisements, JXTA_OBJECT_PPTR(&adv), i);
        jxta_advertisement_get_xml(adv, &adv_string);
        fprintf(stdout, "%s\n", jstring_get_string(adv_string));
        id = jxta_advertisement_get_id(adv);
        jxta_id_to_jstring(id, &id_string);
        printf("Advertisement ID=%s\n", jstring_get_string(id_string));
    }
    if (jxta_vector_size(responses) == count && jxta_vector_size(advertisements) == count) {
        passed = TRUE;
        printf("Passed dr_response_test\n");
    } else {
        printf("Failed dr_response_test count =%d, response count =%d parsed count =%d\n", count,
               jxta_vector_size(responses), jxta_vector_size(advertisements));
    }
    return passed;
}


Jxta_boolean dr_pa_test(Jxta_DiscoveryResponse * ad)
{

    Jxta_boolean passed = TRUE;
    Jxta_PA *pa = jxta_PA_new();
    Jxta_advertisement *padv = jxta_advertisement_new();
    JString *js = jstring_new_0();
    JString *pabuf = NULL;

    jxta_discovery_response_get_peer_advertisement(ad, &padv);
    jxta_discovery_response_get_peeradv(ad, &pabuf);
    jxta_PA_parse_charbuffer(pa, jstring_get_string(pabuf), jstring_length(pabuf));
    jxta_PA_get_xml(pa, &js);
    fprintf(stdout, "%s\n", jstring_get_string(js));
    return passed;
}



Jxta_boolean dr_adv_test(int argc, char **argv)
{

    Jxta_DiscoveryResponse *ad = jxta_discovery_response_new();
    FILE *testfile;
    Jxta_boolean ret = TRUE;
    JString *js = jstring_new_0();

    if (argc != 2) {
        printf("usage: ad <filename>\n");
        return FALSE;
    }
    testfile = fopen(argv[1], "r");
    if (NULL == testfile)
        return FALSE;
    jxta_discovery_response_parse_file(ad, testfile);
    fclose(testfile);
    while (TRUE) {
        ret = dr_type_test(ad, 2);
        if (FALSE == ret)
            break;
        ret = dr_count_test(ad, 3);
        if (FALSE == ret)
            break;
        ret = dr_pa_test(ad);
        if (FALSE == ret)
            break;
        ret = dr_response_test(ad);
        break;
    }
    jxta_discovery_response_get_xml(ad, &js);
    if (NULL == js)
        return FALSE;
    fprintf(stdout, "%s\n", jstring_get_string(js));
    JXTA_OBJECT_RELEASE(ad);

    return ret;
}

int main(int argc, char **argv)
{

    int retval;

    jxta_initialize();
    jxta_advertisement_register_global_handler("jxta:PipeAdvertisement", (const JxtaAdvertisementNewFunc) jxta_pipe_adv_new);
    retval = dr_adv_test(argc, argv);
    jxta_terminate();
    if (TRUE == retval)
        retval = 0;
    else
        retval = -1;
    return retval;
}
