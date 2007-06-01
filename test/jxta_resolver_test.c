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
 * $Id: jxta_resolver_test.c,v 1.23.4.1 2006/11/16 00:06:36 bondolo Exp $
 */

#include "jxta.h"
#include "jxta_resolver_service.h"
#include "jxta_errno.h"
#include "jxta_peergroup.h"
#include "jxta_endpoint_service.h"
#include "jxta_object.h"
#include "jxta_apr.h"
#ifndef WIN32
#include <unistd.h>
#else
static char *optarg = NULL;     /* global argument pointer */
static int optind = 0;  /* global argv index */
static int getopt(int argc, char *argv[], char *optstring)
{
    static char *next = NULL;
    char c;
    char *cp = NULL;

    if (optind == 0)
        next = NULL;

    optarg = NULL;

    if (next == NULL || *next == '\0') {
        if (optind == 0)
            optind++;

        if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0') {
            optarg = NULL;
            if (optind < argc)
                optarg = argv[optind];
            return EOF;
        }

        if (strcmp(argv[optind], "--") == 0) {
            optind++;
            optarg = NULL;
            if (optind < argc)
                optarg = argv[optind];
            return EOF;
        }

        next = argv[optind];
        next++;
        optind++;
    }

    c = *next++;
    cp = strchr(optstring, c);

    if (cp == NULL || c == ':')
        return '?';

    cp++;
    if (*cp == ':') {
        if (*next != '\0') {
            optarg = next;
            next = NULL;
        } else if (optind < argc) {
            optarg = argv[optind];
            optind++;
        } else {
            return '?';
        }
    }

    return c;
}
#endif

/*
 *
 * Resolver test
 *
 */
static void rs_wait(int msec)
{
    apr_sleep((Jpr_interval_time) msec);
}

int main(int argc, char *argv[])
{
    Jxta_PG *pg;
    Jxta_resolver_service *resolver;
    Jxta_rdv_service *rendezvous;
    Jxta_status status;
    Jxta_endpoint_address *addr = NULL;
    FILE *testfile;
    int c;
    JString *rdv = jstring_new_0();
    JString *rq = jstring_new_0();
    ResolverQuery *query;
    while (1) {
        c = getopt(argc, argv, "q:r:");
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'r':
            jstring_append_2(rdv, optarg);
            break;

        case 'q':
            jstring_append_2(rq, optarg);
            break;

        default:
            fprintf(stderr, "getopt error 0%o \n", c);
        }
    }

    if (argc < 2) {
        fprintf(stderr, "jxta_resolver_test usage: jxta_resolver_test -q <query.xml> -r 127.0.0.1:9700\n");
        return -1;
    }

    jxta_initialize();

    status = jxta_PG_new_netpg(&pg);
    if (status != JXTA_SUCCESS) {
        fprintf(stderr, "jxta_resolver_test: jxta_PG_netpg_new failed with error: %ld\n", status);
    }

    jxta_PG_get_resolver_service(pg, &resolver);
    jxta_PG_get_rendezvous_service(pg, &rendezvous);
    /* read in the query */
    testfile = fopen(jstring_get_string(rq), "r");
    if (NULL == testfile)
        return -1;
    query = jxta_resolver_query_new();
    jxta_resolver_query_parse_file(query, testfile);
    fclose(testfile);

        addr = jxta_endpoint_address_new_2("http", jstring_get_string(rdv), NULL, NULL);
    jxta_rdv_service_add_seed(rendezvous, addr);
        JXTA_OBJECT_RELEASE(addr);

    fprintf(stdout, "sending %s \n", jstring_get_string(rq));
    /* use null id to begin with. Support real addresses later */
    status = jxta_resolver_service_sendQuery(resolver, query, jxta_id_nullID);
    if (status != JXTA_SUCCESS) {
        fprintf(stderr, "jxta_resolver_test: failed to send query: %ld\n", status);
    } else {
        fprintf(stdout, "jxta_resolver_test: query %s sent to %s\n", jstring_get_string(rq), jstring_get_string(rdv));
    }
#ifdef WIN32
    Sleep(10000);
#else
    sleep(10);
#endif
    JXTA_OBJECT_RELEASE(resolver);
    jxta_module_stop((Jxta_module *) pg);
    JXTA_OBJECT_RELEASE(pg);

    jxta_terminate();
    return 0;
}
