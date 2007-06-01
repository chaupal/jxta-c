/*
 * Copyright (c) 2005 Sun Microsystems, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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
 * $Id: jxta_bidipipe_test.c,v 1.3.2.3 2005/05/20 01:05:15 slowhog Exp $
 */

#include <apr_thread_proc.h>
#include <apr_thread_cond.h>
#include <apr_thread_mutex.h>
#include "jxta.h"
#include "jxta_peergroup.h"
#include "jxta_object.h"
#include "jxta_bidipipe.h"
#include "jxta_log.h"
#include "jxta_discovery_service.h"

#define BIDITEST_LOG "BidipipeTest"

static struct _app {
    Jxta_PG *pg;
    apr_thread_mutex_t *mutex;
    apr_thread_cond_t *pipe_ready_cond;
    apr_thread_cond_t *pipe_close_cond;
    apr_thread_cond_t *svr_thread_stop_cond;
    int serial_no;
} app;


void clt_listener_func(Jxta_object * obj, void *arg)
{
    Jxta_message *msg = (Jxta_message *) obj;

    if (NULL == obj) {
        printf("Connection closed by server.\n");
        apr_thread_cond_signal(app.pipe_close_cond);
        return;
    }

    printf("Server: \n");
    jxta_message_print(msg);
}

void svr_listener_func(Jxta_object * obj, void *arg)
{
    Jxta_message *msg = (Jxta_message *) obj;

    if (NULL == obj) {
        printf("Connection closed by client.\n");
        apr_thread_cond_signal(app.pipe_close_cond);
        return;
    }

    printf("Client: \n");
    jxta_message_print(msg);
}

void * APR_THREAD_FUNC svr_thread_func(apr_thread_t * thread, void *arg)
{
    Jxta_pipe_adv *pipe_adv = (Jxta_pipe_adv *) arg;
    Jxta_bidipipe *svr = NULL;
    Jxta_listener *listener;

    svr = jxta_bidipipe_new(app.pg);

    if (NULL == svr) {
        jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_ERROR, "Failed to create server pipe\n");
        exit(-3);
    }
    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Server BiDi pipe created\n");

    listener = jxta_listener_new(svr_listener_func, svr, 1, 30);
    if (NULL == listener) {
        jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_ERROR, "Failed to create server listener\n");
        exit(-4);
    }
    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Server listener created\n");

    jxta_listener_start(listener);

    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Server pipe waiting for connection request\n");
    jxta_bidipipe_accept(svr, pipe_adv, listener);

    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Server pipe connected\n");
    apr_thread_mutex_lock(app.mutex);
    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Mutex locked, waiting for pipe to close\n");
    apr_thread_cond_wait(app.pipe_close_cond, app.mutex);
    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Pipe closed at %s:%d\n", __FILE__, __LINE__);
    apr_thread_mutex_unlock(app.mutex);

    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Stop server listener ...\n");
    jxta_listener_stop(listener);
    JXTA_OBJECT_RELEASE(listener);

    jxta_bidipipe_delete(svr);

    apr_thread_cond_signal(app.svr_thread_stop_cond);
    return NULL;
}

int main(int argc, char *argv[])
{
    Jxta_bidipipe *clt;
    Jxta_listener *clt_listener;
    apr_thread_t *svr_thread;
    apr_pool_t *pool;
    Jxta_status res;
    Jxta_pipe_adv *adv;
    Jxta_message *msg;
    Jxta_message_element *el;
    FILE *f;
    char buf[32];
    int i, len;
    Jxta_log_file *log_f;
    Jxta_log_selector *log_s;
    Jxta_discovery_service *discovery;

    if (argc < 2) {
        printf("Usage: %s adv_file_name\n", argv[0]);
        exit(1);
    }

    jxta_initialize();

    log_s = jxta_log_selector_new_and_set("*.*", &res);
    if (NULL == log_s) {
        printf("failed to init log selector.\n");
        exit(-1);
    }

    jxta_log_file_open(&log_f, "-");
    jxta_log_file_attach_selector(log_f, log_s, NULL);
    jxta_log_using(jxta_log_file_append, log_f);

    apr_pool_create(&pool, NULL);
    apr_thread_mutex_create(&app.mutex, APR_THREAD_MUTEX_DEFAULT, pool);
    apr_thread_cond_create(&app.pipe_ready_cond, pool);
    apr_thread_cond_create(&app.pipe_close_cond, pool);
    apr_thread_cond_create(&app.svr_thread_stop_cond, pool);

    app.serial_no = 0;

    res = jxta_PG_new_netpg(&app.pg);
    if (res != JXTA_SUCCESS) {
        printf("jxta_PG_new_netpg failed with error: %ld\n", res);
        exit(-1);
    }

    jxta_PG_get_discovery_service(app.pg, &discovery);

    adv = jxta_pipe_adv_new();
    f = fopen(argv[1], "r");
    if (NULL == f) {
        printf("Failed to open pipe advertisement file %s\n", argv[1]);
    }

    jxta_pipe_adv_parse_file(adv, f);
    fclose(f);

    res = discovery_service_publish(discovery, adv, DISC_ADV, 5L * 60L * 1000L, 5L * 60L * 1000L);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_ERROR, "Failed to publish pipe advertisement\n");
        exit(-3);
    }

    JXTA_OBJECT_RELEASE(discovery);

    res = apr_thread_create(&svr_thread, NULL, svr_thread_func, adv, pool);
    if (APR_SUCCESS != res) {
        printf("Failed to create server thread with error: %ld\n", res);
        exit(-2);
    }

    clt = jxta_bidipipe_new(app.pg);
    if (NULL == clt) {
        jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_ERROR, "Failed to create client pipe\n");
        exit(-3);
    }

    clt_listener = jxta_listener_new(clt_listener_func, clt, 1, 30);
    if (NULL == clt_listener) {
        jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_ERROR, "Failed to create client listener\n");
        exit(-4);
    }
    jxta_listener_start(clt_listener);

    printf("Delay 2 sec for RDV indexing\n");
    apr_sleep(2L * 1000L * 1000L);

    printf("Try to connect for 5 sec...\n");
    res = jxta_bidipipe_connect(clt, adv, clt_listener, 5L * 1000L * 1000L);
    if (JXTA_SUCCESS != res) {
        printf("Failed to connect to server pipe with error: %ld\n", res);
        exit(-5);
    }

    JXTA_OBJECT_RELEASE(adv);

    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Wait 2 sec to see if connection can complete\n");
    apr_sleep(2L * 1000L * 1000L);
    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Has handshake completed?\n");
    for (i = 0; i < 10; i++) {
        msg = jxta_message_new();
        len = sprintf(buf, "Message %d", ++app.serial_no);
        el = jxta_message_element_new_1("MSG", "text/plain", buf, len, NULL);
        jxta_message_add_element(msg, el);
        jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Sending message %d with serial NO %ld\n", i, app.serial_no);
        res = jxta_bidipipe_send(clt, msg);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_ERROR, "Sending message failed with error %ld\n", res);
        }
        /* Without sleep will cause segfault */
        apr_sleep(50L * 1000L);
/*        jxta_message_remove_element(msg, el); */
        JXTA_OBJECT_RELEASE(el);
        JXTA_OBJECT_RELEASE(msg);
    }

    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Close bidipipe from client ...\n");
    jxta_bidipipe_close(clt);

    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Waiting for server thread to stop ...\n");
    apr_thread_mutex_lock(app.mutex);
    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Mutex locked, waiting for server thread to stop ...\n");
    apr_thread_cond_wait(app.svr_thread_stop_cond, app.mutex);
    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Server thread stopped\n");
    apr_thread_mutex_unlock(app.mutex);

    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Stopping client listener ...\n");
    jxta_listener_stop(clt_listener);
    JXTA_OBJECT_RELEASE(clt_listener);

    jxta_log_append(BIDITEST_LOG, JXTA_LOG_LEVEL_TRACE, "Delete client bidipipe ...\n");
    jxta_bidipipe_delete(clt);

    jxta_module_stop((Jxta_module *) app.pg);
    JXTA_OBJECT_RELEASE(app.pg);

    apr_thread_cond_destroy(app.svr_thread_stop_cond);
    apr_thread_cond_destroy(app.pipe_close_cond);
    apr_thread_cond_destroy(app.pipe_ready_cond);
    apr_thread_mutex_destroy(app.mutex);
    apr_pool_destroy(pool);

    jxta_log_file_close(log_f);
    jxta_log_selector_delete(log_s);
    jxta_terminate();

    return 0;
}

/* vim: set ts=4 sw=4 wm=130 et: */
