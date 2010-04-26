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

#include "jxta.h"
#include <apr.h>
#include <apr_general.h>
#include <stdio.h>
#include <dirent.h>
#include "jxta_peergroup.h"
#include "jxta_endpoint_service.h"
#include "jxta_transport.h"
#include "../src/jxta_private.h"
#include "../src/jxta_endpoint_service_priv.h"
#include "jxta_listener.h"


#define ENDPOINT_TEST_SERVICE_NAME "jxta:EndpointTest"
#define ENDPOINT_TEST_SERVICE_PARAMS "EndpointTestParams"


extern Jxta_endpoint_service *jxta_endpoint_service_new_instance(void);

Jxta_PG *pg;
apr_pool_t *pool;
Jxta_endpoint_service *endpoint = NULL;
Jxta_endpoint_address *hostAddr;
Jxta_endpoint_address *localEndpointAddr;
Jxta_transport *http_transport;
apr_thread_mutex_t *mutex;
apr_thread_cond_t *cond;
int thread_count;
int loop_count;

static void test_thread_done(void);

static Jxta_status callback(Jxta_object * obj, void *cookie)
{
    printf("Received a message\n");
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



/**
 ** This functions is used by the message implementation in order
 ** to transfer bytes between the file containing the message and the
 ** message object (this is a callback).
 **/


Jxta_status readMessageBytes(void *stream, char *buf, apr_size_t len)
{

    fread((void *) buf, (size_t) len, 1, (FILE *) stream);
    return APR_SUCCESS;
}

apr_status_t writeMessageBytes(void *stream, char *buf, apr_size_t len)
{

    fwrite((void *) buf, (size_t) len, 1, (FILE *) stream);
    return APR_SUCCESS;
}


apr_thread_t **spawn_threads(int n, apr_thread_start_t thread_func, void *arg, apr_pool_t * pool)
{
    int i;
    apr_thread_t **threads = (apr_thread_t **) malloc(n * sizeof(apr_thread_t *));

    for (i = 0; i < n; i++) {

        apr_thread_create(&threads[i], NULL, thread_func, arg, pool);
    }

    return threads;
}


void *APR_THREAD_FUNC single_test(apr_thread_t * thread, void *arg)
{
    char *msg_dir = (char *) arg;
    DIR *dir = opendir(msg_dir);
    struct dirent *entry;
    FILE *file;
    Jxta_message *msg;
    int i;

    if (dir) {
        for (i = 0; i < loop_count; i++) {
            printf("Opened directory %s\n", msg_dir);

            while ((entry = readdir(dir))) {
                if (strncmp(entry->d_name, "jxta_message*", 12) == 0) {


                    char *filename = (char *) malloc(strlen(msg_dir) + 1 + strlen(entry->d_name) + 1);

                    sprintf(filename, "%s/%s", msg_dir, entry->d_name);

                    file = fopen(filename, "r");

                    if (file) {
                        printf("Opened message file %s\n", filename);
                        msg = jxta_message_new();
                        printf("Allocated msg %p\n", msg);
                        jxta_message_read(msg, "application/x-jxta-msg", readMessageBytes, (void *) file);
                        printf("Read into msg %p\n", msg);
                        if (strstr(entry->d_name, "output")) {
                            printf("It's an output message, sending it.\n");
                            jxta_endpoint_service_send(pg, endpoint, msg, hostAddr, NULL);
/*         } else if (strstr (entry->d_name, "input")) {
            printf ("It's a sample input message, demuxing it.\n");
            jxta_endpoint_service_demux (endpoint, msg);
*/ }

                        JXTA_OBJECT_RELEASE(msg);
                        fclose(file);
                    }
                }
            }
            rewinddir(dir);
        }

        closedir(dir);

        test_thread_done();

    } else {
        printf("Unable to open message directory.");
    }

    return NULL;
}


static void test_thread_done()
{
    apr_thread_mutex_lock(mutex);
    printf("signaling thread done.\n");
    thread_count--;
    apr_thread_cond_signal(cond);
    apr_thread_mutex_unlock(mutex);
}

int main(int argc, char **argv)
{
    char *host;
    char *msg_dir;

#ifndef WIN32
    struct sigaction sa;

    sa.sa_flags = 0;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
#endif


    if (argc != 5) {
        printf("usage: endpoint_test ip:port message_dir thread_count loop_count\n");
        return -1;
    }

    jxta_initialize();

    host = argv[1];
    msg_dir = argv[2];
    thread_count = atoi(argv[3]);
    loop_count = atoi(argv[4]);

    printf("using ip:port=%s msg_dir=%s thread_count=%d loop_count=%d\n", host, msg_dir, thread_count, loop_count);

    /* Initialise the endpoint and http service, set the endpoint listener */
    init();

    /* Builds the destination address */
    hostAddr = jxta_endpoint_address_new_2((char *) "http",
                                           host, (char *) ENDPOINT_TEST_SERVICE_NAME, (char *) ENDPOINT_TEST_SERVICE_PARAMS);

    /* Builds a message (empty) and sends it */
    /* msg = jxta_message_new(pool);

       jxta_endpoint_service_send (endpoint,
       msg,
       hostAddr);

       JXTA_OBJECT_RELEASE (msg);
     */


    apr_thread_mutex_create(&mutex, APR_THREAD_MUTEX_DEFAULT, pool);
    apr_thread_cond_create(&cond, pool);

    spawn_threads(thread_count, single_test, msg_dir, pool);

    apr_thread_mutex_lock(mutex);
    while (thread_count > 0) {
        apr_thread_cond_wait(cond, mutex);
        printf("woke up from wait with thread_count = %d\n", thread_count);
    }
    apr_thread_mutex_unlock(mutex);

    /* Sleep a bit to let the endpoint works */
    apr_sleep(1 * 60 * 1000 * 1000);

    apr_thread_mutex_destroy(mutex);
    apr_thread_cond_destroy(cond);
    apr_pool_destroy(pool);

    jxta_terminate();
    return 0;
}
