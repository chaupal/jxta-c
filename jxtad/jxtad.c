/*
 * Copyright (c) 2007 Sun Microsystems, Inc.  All rights reserved.
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
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define PACKAGE "JXTA"
#define VERSION "2.5.2"
#endif

#include <apr_general.h>
#include <apr_getopt.h>
#include <apr_signal.h>

#include "jxta.h"
#include "jxta_peergroup.h"
#include "jxta_log.h"

static const apr_getopt_option_t options[] = {
    { "verbose", 'v', 0, "increase verbose level" },
    { "rdv", 0x103, 0, "run as a rendezvous" },
    { "relay", 0x102, 0, "run as a relay" },
    { "name", 'n', 1, "peer name" },
    { "logfile", 'f', 1, "log filename" },
    { "reconfigure", 'r', 0, "Ignore existing configuration" },
    { "home", 0x101, 1, "Path for configuration" }
};

static apr_proc_t proc;

static struct {
    apr_pool_t *pool;
    apr_thread_cond_t *stop_cond;
    apr_thread_mutex_t *stop_lock;
    Jxta_PG *pg;
    Jxta_log_file *log_f;
    Jxta_log_selector *log_s;
} app;

static int setup_log(const char *fname, const char *selector)
{
    Jxta_status status;

    app.log_s = jxta_log_selector_new_and_set(selector, &status);
    if (NULL == app.log_s) {
        fprintf(stderr, "Failed to init default log selector.\n");
        return -1;
    }

    if (fname) {
        jxta_log_file_open(&app.log_f, fname);
    } else {
        jxta_log_file_open(&app.log_f, "-");
    }

    jxta_log_using(jxta_log_file_append, app.log_f);
    jxta_log_file_attach_selector(app.log_f, app.log_s, NULL);

    return 0;
}

static int process_options(int argc, const char * const* argv) 
{
    static const char *verbositys[] = { "warning", "info", "debug", "trace", "paranoid" };

    char logselector[256];
    apr_getopt_t *os;
    int optch;
    const char *optarg;
    int verbosity = 0;
    const char *fname = NULL;

    apr_getopt_init(&os, app.pool, argc, argv);
    while (APR_SUCCESS == apr_getopt_long(os, options, &optch, &optarg)) {
        switch (optch) {
        case 'v':
            verbosity++;
            break;
        case 'f':
            fname = optarg;
            break;
        default:
            printf("Option -%c[%X] specified with argument %s\n", optch, optch, optarg ? optarg : "N/A");
            break;
        }
    }

    if (verbosity > (sizeof(verbositys) / sizeof(const char *)) - 1) {
        verbosity = (sizeof(verbositys) / sizeof(const char *)) - 1;
    }

    strcpy(logselector, "*.");
    strcat(logselector, verbositys[verbosity]);
    strcat(logselector, "-fatal");

    printf("Starting logger with selector : %s\n", logselector);
    if (setup_log(fname, logselector) < 0) {
        return -1;
    }
    return 0;
}

static void exit_app(int n)
{
    jxta_module_stop((Jxta_module *) app.pg);
    JXTA_OBJECT_RELEASE(app.pg);

    jxta_log_using(NULL, NULL);
    jxta_log_file_close(app.log_f);
    jxta_log_selector_delete(app.log_s);

    jxta_terminate();

    apr_thread_mutex_lock(app.stop_lock);
    apr_thread_cond_signal(app.stop_cond);
    apr_thread_mutex_unlock(app.stop_lock);
}

int main(int argc, char **argv)
{
    Jxta_status status;
    int rc = 0;

    apr_initialize();

    if (APR_SUCCESS != apr_pool_create(&app.pool, NULL)) {
        fprintf(stderr, "Failed to create pool.\n");
        exit(1);
    }

    jxta_initialize();

    rc = process_options(argc, (const) argv);
    if (rc < 0) {
        exit(rc);
    }
    
    switch (apr_proc_fork(&proc, NULL)) {
        case APR_INPARENT:
            apr_pool_destroy(app.pool);
            exit(0);
            break;
        case APR_INCHILD:
            break;
        default:
            exit(-1);
            break;
    }

    setsid();
    apr_thread_cond_create(&app.stop_cond, app.pool);
    apr_thread_mutex_create(&app.stop_lock, APR_THREAD_MUTEX_DEFAULT, app.pool);
    apr_signal(SIGTERM, exit_app);

    status = jxta_PG_new_netpg(&app.pg);
    if (status != JXTA_SUCCESS) {
        fprintf(stderr, "# %s - jxta_PG_netpg_new failed with error: %ld\n", argv[0], status);
        exit_app(0);
        rc = -1;
    }

    apr_thread_mutex_lock(app.stop_lock);
    apr_thread_cond_wait(app.stop_cond, app.stop_lock);
    apr_thread_mutex_unlock(app.stop_lock);
    apr_pool_destroy(app.pool);
    return rc;
}

/* vi: set ts=4 sw=4 tw=130 et: */
