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
#include <signal.h>
#include "jxta.h"
#include "jxta_peergroup.h"
#include "jxta_qos.h"

#include "../src/jxta_private.h"
#include "../src/jxta_endpoint_service_priv.h"

/****************************************************************
 **
 ** This test program tests the JXTA message code. It
 ** reads a pre-cooked message (testmsg in the test directory)
 ** which is in the wire format, and builds a message.
 **
 ****************************************************************/


#define ENDPOINT_TEST_SERVICE_NAME "jxta:EndpointTest"
#define ENDPOINT_TEST_SERVICE_PARAMS "EndpointTestParams"

static const char *verbositys[] = { "warning", "info", "debug", "trace", "paranoid" };

Jxta_PG *pg = NULL;
apr_pool_t *pool;
Jxta_endpoint_service *endpoint = NULL;

Jxta_status JXTA_STDCALL callback(Jxta_object * msg, void *cookie)
{
    apr_uint16_t ttl;
    apr_uint16_t priority;
    time_t t;
    Jxta_message *m = (Jxta_message*) msg;

    printf("Received a message: ");
    if (JXTA_SUCCESS == jxta_message_get_ttl(m, &ttl)) {
        printf(" TTL %lu", ttl);
    }
    if (JXTA_SUCCESS == jxta_message_get_priority(m, &priority)) {
        printf(" priority %lu", priority);
    }
    if (JXTA_SUCCESS == jxta_message_get_lifespan(m, &t)) {
        printf(" lifespan %lu against %lu", t, time(NULL));
    }
    printf("\n");
    return JXTA_SUCCESS;
}

void init(void)
{
    Jxta_status res;
    void *cookie;

    apr_pool_create(&pool, NULL);

  /**
   ** Get the endpoint srvce from the net pg.
   **/

    res = jxta_PG_new_netpg(&pg);

    if (res != JXTA_SUCCESS) {
        printf("jxta_PG_netpg_new failed with error: %ld\n", res);
        exit(res);
    }

    jxta_PG_get_endpoint_service(pg, &endpoint);

    /* sets and endpoint listener */
    endpoint_service_add_recipient(endpoint, &cookie, ENDPOINT_TEST_SERVICE_NAME, ENDPOINT_TEST_SERVICE_PARAMS, callback, NULL);
}

int main(int argc, char **argv)
{
    int verbosity = 0;
    Jxta_log_file *log_f = NULL;
    Jxta_log_selector *log_s = NULL;
    char logselector[256];
    char *lfname = NULL;
    char *fname = NULL;
    int optc;
    char **optv = argv;
    Jxta_message *msg;
    char *host = "127.0.0.1:9701";
    Jxta_status rv;
    apr_uint16_t ttl[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    apr_uint16_t priority[10] = { 0x500, 0x400, 0x500, 0x0, 0xFFFF, 0x700, 0x800, 0x200, 0xA00, 0x100 };
    int tdiff[10] = { 120, 0, 5, 1800, 240, 120, 120, 120, 120, -120 };
    Jxta_qos *qos_set[10];
    int i;
    Jxta_status status = JXTA_SUCCESS;
    Jxta_endpoint_address *hostAddr;

#ifndef WIN32
    struct sigaction sa;

    sa.sa_flags = 0;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
#endif

    jxta_initialize();

    for(optc = 1; optc < argc; optc++ ) {
        char *opt = argv[optc];
        
        if( *opt != '-' ) {
            fprintf(stderr, "# %s - Invalid option.\n", argv[0]);
            status = JXTA_INVALID_ARGUMENT;
            goto FINAL_EXIT;
        }
        
        opt++;
        
        switch( *opt ) {
            case 'v' :
                verbosity++;
                break;
                
            case 'l' :
                lfname = strdup(argv[++optc]);
                break;

            case 'f' : 
                fname = strdup(argv[++optc]);
                break;
            
            case 'h' :
                host = argv[++optc];
                break;

            default :
                fprintf(stderr, "# %s - Invalid option : %c.\n", argv[0], *opt );
                status = JXTA_INVALID_ARGUMENT;
                goto FINAL_EXIT;
        }
    }

    if (verbosity > (sizeof(verbositys) / sizeof(const char *)) - 1) {
        verbosity = (sizeof(verbositys) / sizeof(const char *)) - 1;
    }

    strcpy(logselector, "*.");
    strcat(logselector, verbositys[verbosity]);
    strcat(logselector, "-fatal");

    fprintf(stderr, "Starting logger with selector : %s\n", logselector);
    log_s = jxta_log_selector_new_and_set(logselector, &status);
    if (NULL == log_s) {
        fprintf(stderr, "# %s - Failed to init default log selector.\n", argv[0]);
        status = JXTA_INVALID_ARGUMENT;
        goto FINAL_EXIT;
    }

    if (fname) {
        jxta_log_file_open(&log_f, fname);
        free(fname);
    } else {
        jxta_log_file_open(&log_f, "-");
    }

    jxta_log_using(jxta_log_file_append, log_f);
    jxta_log_file_attach_selector(log_f, log_s, NULL);

    if (NULL != lfname) {
        FILE *loggers = fopen(lfname, "r");

        if (NULL != loggers) {
            do {
                if (NULL == fgets(logselector, sizeof(logselector), loggers)) {
                    break;
                }

                fprintf(stderr, "Starting logger with selector : %s\n", logselector);

                log_s = jxta_log_selector_new_and_set(logselector, &status);
                if (NULL == log_s) {
                    fprintf(stderr, "# %s - Failed to init default log selector.\n", argv[0]);
                    status = JXTA_INVALID_ARGUMENT;
                    goto FINAL_EXIT;
                }

                jxta_log_file_attach_selector(log_f, log_s, NULL);
            } while (TRUE);

            fclose(loggers);
        }
    }

    jxta_log_file_attach_selector(log_f, log_s, NULL);

    if (lfname) {
        free(lfname);
    }
    /* Initialise the endpoint and http service, set the endpoint listener */
    init();

    /* Builds the destination address */
    hostAddr = jxta_endpoint_address_new_2("tcp", host, ENDPOINT_TEST_SERVICE_NAME, ENDPOINT_TEST_SERVICE_PARAMS);

    for (i = 0; i < 10; i++) {
        rv = jxta_qos_create(qos_set + i, pool);
        if (JXTA_SUCCESS != rv) {
            printf("Failed to create Jxta_qos set[%d] with status %d\n", i, rv);
            return -1;
        }
        jxta_qos_set_long(qos_set[i], "jxtaMsg:TTL", ttl[i]);
        jxta_qos_set_long(qos_set[i], "jxtaMsg:priority", priority[i]);
    }
    
    /* Builds a message (empty) and sends it */
    printf("Async mode: Expect to see msg with TTL: 1,2,3,4,5,6,7 in rough order of priority\n");
    for (i = 0; i < 10; i++) {
        msg = jxta_message_new();
        jxta_qos_set_long(qos_set[i], "jxtaMsg:lifespan", time(NULL) + tdiff[i]);
        jxta_message_attach_qos(msg, qos_set[i]);

        jxta_endpoint_service_send_async(pg, endpoint, msg, hostAddr, NULL);
        JXTA_OBJECT_RELEASE(msg);
    }

    apr_sleep(5000000L);
    printf("Sync mode: Expect to see msg with TTL: 2,3,4,5,6,7 in order regardless of priority\n");
    for (i = 0; i < 10; i++) {
        msg = jxta_message_new();
        jxta_message_attach_qos(msg, qos_set[i]);
        jxta_endpoint_service_send_sync(pg, endpoint, msg, hostAddr, NULL);
        JXTA_OBJECT_RELEASE(msg);
    }
    
    JXTA_OBJECT_RELEASE(hostAddr);

    /* Sleep a bit to let the endpoint works */
    apr_sleep(1 * 60 * 1000 * 1000);

FINAL_EXIT:
    jxta_log_using(NULL, NULL);
    jxta_log_file_close(log_f);
    jxta_log_selector_delete(log_s);

    apr_pool_destroy(pool);
    JXTA_OBJECT_RELEASE(endpoint);
    JXTA_OBJECT_RELEASE(pg);
    jxta_terminate();

    return 0;
}

/* vim: set ts=4 sw=4 et tw=130: */
