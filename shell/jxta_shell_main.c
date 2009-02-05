/*
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
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
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define PACKAGE "JXTA"
#define VERSION "2.5.1"
#endif

#include <apr_general.h>

#include "jxta.h"
#include "jxta_peergroup.h"
#include "jxta_log.h"

#include "jxta_shell_tokenizer.h"
#include "jxta_shell_application.h"
#include "jxta_shell_environment.h"
#include "jxta_shell_getopt.h"
#include "jxta_shell_main.h"

#include "jxta_shell_app_env.h"

#include "TestApplication.h"
#include "cat.h"
#include "peers.h"
#include "talk.h"
#include "groups.h"
#include "whoami.h"
#include "join.h"
#include "monitor.h"
#include "rdvcontrol.h"
#include "rdvstatus.h"
#include "leave.h"
#include "kdb.h"
#include "search.h"
#include "publish.h"
static const char *__log_cat = "SHELL_MAIN";

struct _shell_startup_application {
    JXTA_OBJECT_HANDLE;
    JxtaShellApplication *app;
    JxtaShellApplication *currentProcess;
};

void ShellStartupApplication_delete(Jxta_object * app);

/**
*  Processes the command line 
* @param frame the ShellStartupApplication that starts the new process
* @param command the name of the program to start
* @param argv the number of arguments to pass
* @param arg the list of arguments
*/
void ShellStartupApplication_processCommandLine(ShellStartupApplication * frame, char *command, int argv, char **arg);

/**
*  Starts a new application
* @param frame the ShellStartupApplication that starts the new process
* @param parent the  parent of the application to start
* @param parentObject
* @param command the name of the program to start
* @param argv the number of arguments to pass
* @param arg the list of arguments
* @param setAsCurrent do we want to set this as the main application
*/
JxtaShellApplication *ShellStartupApplication_startApplication(ShellStartupApplication * frame,
                                                               JxtaShellApplication * parent,
                                                               Jxta_object * parentObject,
                                                               char *command, int argv, char **arg, Jxta_boolean setAsCurrent);

struct commands {
    const char *name;
    shell_application_new constructor;
};

static const struct commands allCommands[] = {
    {"TestApplication", JxtaTestApplication_new},
    {"env", JxtaShellApp_env_new},
    {"cat", jxta_cat_new},
    {"whoami", jxta_whoami_new},
    {"kdb", jxta_kdb_new},
    {"peers", jxta_peers_new},
    {"talk", talk_new},
    {"join", jxta_join_new},
    {"groups", jxta_groups_new},
    {"search", jxta_search_new},
    {"leave", jxta_leave_new},
    {"rdvstatus", jxta_rdvstatus_new},
    {"rdvcontrol", jxta_rdvcontrol_new},
    {"publish", publish_new},
    {"monitor", monitor_new},
    {NULL, NULL}
};

ShellStartupApplication *ShellStartup_new(Jxta_PG * pg, Jxta_listener * standout, shell_application_terminate terminate)
{
    JxtaShellEnvironment *env = NULL;
    ShellStartupApplication *startup = (ShellStartupApplication *) calloc(1, sizeof(ShellStartupApplication));

    if (startup == NULL)
        return NULL;

    JXTA_OBJECT_INIT_FLAGS(startup, JXTA_OBJECT_SHARE_TRACK, ShellStartupApplication_delete, 0);

    startup->app = JxtaShellApplication_new(pg, standout, NULL, (Jxta_object *) startup, terminate);

    if (startup->app == NULL)
        return NULL;

    startup->currentProcess = NULL;

    JxtaShellApplication_setFunctions(startup->app,
                                      (Jxta_object *) startup,
                                      ShellStartupApplication_printHelp,
                                      ShellStartupApplication_start, ShellStartupApplication_stdin);

    /* Add default items to enviroment */

    env = JxtaShellApplication_getEnv(startup->app);

    if (NULL != env) {
        JxtaShellObject *var = NULL;
        JString *name = NULL;
        JString *type = NULL;
        Jxta_PA *pa = NULL;
        Jxta_id *pgid = NULL;

        jxta_PG_get_PA(pg, &pa);
        name = jstring_new_2("thisPeer");
        type = jstring_new_2("PeerAdvertisement");
        var = JxtaShellObject_new(name, (Jxta_object *) pa, type);
        JXTA_OBJECT_RELEASE(name);
        JXTA_OBJECT_RELEASE(type);

        JxtaShellEnvironment_add_0(env, var);
        JXTA_OBJECT_RELEASE(var);   /* release local */
        JXTA_OBJECT_RELEASE(pa);
        pa = NULL;  /* release local */

        /* add the net peer group if we have it */
        jxta_PG_get_GID(pg, &pgid);

        if (jxta_id_equals(pgid, jxta_id_defaultNetPeerGroupID)) {
            name = jstring_new_2("netPeerGroup");
            type = jstring_new_2("Group");
            var = JxtaShellObject_new(name, (Jxta_object *) pg, type);
            JXTA_OBJECT_RELEASE(name);
            JXTA_OBJECT_RELEASE(type);

            JxtaShellEnvironment_add_0(env, var);
            JXTA_OBJECT_RELEASE(var);
            var = NULL; /* release local */
        }
        JXTA_OBJECT_RELEASE(pgid);
        pgid = NULL;    /* release local */

        JxtaShellEnvironment_set_current_group(env, pg);
    }

    JXTA_OBJECT_RELEASE(env);

    return startup;
}

void ShellStartupApplication_stdin(Jxta_object * app, JString * inputLine)
{
    char const *inp = jstring_get_string(inputLine);
    if (inp != NULL) {
        fprintf(stdout, "%s", inp);
        fflush(stdout);
    }
}


void ShellStartupApplication_start(Jxta_object * parent, int argv, char **arg)
{
    ShellStartupApplication *frame = (ShellStartupApplication *) parent;
    Jxta_listener *listener = NULL;
    char line[1025];
    char *delim = "\t\n ", **arguments, *token;
    int count;
    int tmp;

    JxtaShellTokenizer *tok;
    JString *message = NULL;

    if (frame == NULL)
        return;

    printf("=========================================================\n");
    printf("========= Welcome to the JXTA-C Shell  Version " VERSION " ======\n");
    printf("=========================================================\n");

    printf(" \n");
    printf("The JXTA Shell provides an interactive environment to the JXTA\n");
    printf("platform. The Shell provides basic commands to discover peers and\n");
    printf("peergroups, to join and resign from peergroups, to create pipes\n");
    printf("between peers, and to send pipe messages. The Shell provides environment\n");
    printf("variables that permit binding symbolic names to Jxta platform objects.\n");
    printf("Environment variables allow Shell commands to exchange data between \n");
    printf("themselves. The shell command 'env' displays all defined environment \n");
    printf("variables in the current Shell session.\n");
    printf(" ");
    printf("\n");
    printf("A 'man' command is available to list the commands available.\n");
    printf("To exit the Shell, use the 'exit' command.\n");
    printf("\n");
    printf("JXTA>");

    while (TRUE) {
        int i = 0;
        JXTA_OBJECT_CHECK_VALID(frame);
        while (i <= 1024) {
            tmp = getchar();
            if ((EOF == tmp) || (tmp == '\n'))
                break;

            line[i++] = (char) tmp;
        }
        line[i] = '\0';

            /** If we have a current process send input to it */
        if (frame->currentProcess != NULL) {
            message = jstring_new_2(line);
            if (message == NULL)
                continue;
            listener = JxtaShellApplication_getSTDIN(frame->currentProcess);
            if (listener != NULL) {
                JXTA_OBJECT_SHARE(message);
                jxta_listener_schedule_object(listener, (Jxta_object *) message);
                JXTA_OBJECT_RELEASE(listener);
                JXTA_OBJECT_RELEASE(message);
            }
            continue;
        }
            /** Otherwise we want to start a new proceess */
        else {
            tok = JxtaShellTokenizer_new(line, delim, 1);
            count = JxtaShellTokenizer_countTokens(tok);
            if (count == 0) {
                printf("JXTA>");
                JXTA_OBJECT_RELEASE(tok);
                continue;
            } else {
                token = JxtaShellTokenizer_nextToken(tok);
                if (strcmp(token, "exit") == 0 || strcmp(token, "quit") == 0) {
                    free(token);
                    JXTA_OBJECT_RELEASE(tok);
                    break;
                }
                count--;
                if (count > 0) {
                    arguments = (char **) malloc(sizeof(char *) * count);
                    for (i = 0; i < count; i++) {
                        arguments[i] = JxtaShellTokenizer_nextToken(tok);
                    }
                } else {
                    arguments = 0;
                }
                JXTA_OBJECT_CHECK_VALID(frame);
                ShellStartupApplication_processCommandLine(frame, token, count, arguments);
                JXTA_OBJECT_CHECK_VALID(frame);
                free(token);
                JXTA_OBJECT_RELEASE(tok);
                if (count > 0) {
                    for (i = 0; i < count; i++) {
                        free(arguments[i]);
                    }
                    free(arguments);
                    arguments = 0;
                }
            }
        }
    }

    JXTA_OBJECT_RELEASE(frame->app);
    frame->app = NULL;
    printf("Exiting Jxta shell\n");
}

void ShellStartupApplication_processCommandLine(ShellStartupApplication * frame, char *command, int argv, char **arg)
{
    JxtaShellApplication *app = frame->app;
    int i, c, pipedArgv = argv;
    char **pipedArg = arg;
    Jxta_boolean first = TRUE;
    Jxta_object *parentObject = (Jxta_object *) frame;

    JXTA_OBJECT_CHECK_VALID(parentObject);

    for (i = argv - 1; i >= 0; i--) {
        if (strcmp(arg[i], "|") == 0 && i + 1 < argv) {
            pipedArgv = i;
            pipedArg = ((i + 2) < argv) ? &arg[i + 2] : 0;

            c = argv - i - 2;
            if (c < 0)
                c = 0;
            app = ShellStartupApplication_startApplication(frame, app, parentObject, arg[i + 1], c, pipedArg, first);

            if (!first)
                JXTA_OBJECT_RELEASE(app);

            parentObject = (Jxta_object *) app;   /** startApplication already shared app */
            first = FALSE;
        }
    }

    app = ShellStartupApplication_startApplication(frame, app, parentObject, command, pipedArgv, pipedArg, first);

    if (app != NULL)
        JXTA_OBJECT_RELEASE(app);
}


JxtaShellApplication *ShellStartupApplication_startApplication(ShellStartupApplication * frame,
                                                               JxtaShellApplication * parent,
                                                               Jxta_object * parentObject,
                                                               char *command, int argv, char **arg, Jxta_boolean setAsCurrent)
{
    JxtaShellApplication *app = NULL;

    if (strcmp(command, "man") == 0) {
        if (argv == 0) {
            const struct commands *eachCommand = &allCommands[0];
            printf("For which command do you want help?\n");

            while (NULL != eachCommand->name) {
                printf("\t%s\n", eachCommand->name);

                eachCommand++;
            }
            goto COMMON_EXIT;
        } else {
            app = ShellStartupApplication_loadApplication(parent, parentObject, arg[0]);
        }
    } else {
        app = ShellStartupApplication_loadApplication(parent, parentObject, command);
    }

    if (app != NULL) {
        JXTA_OBJECT_SHARE(app);
        if (setAsCurrent)
            frame->currentProcess = app;
        if (strcmp(command, "man") == 0) {
            JxtaShellApplication_printHelp(app);
        } else {
            JxtaShellApplication_start(app, argv, arg);
        }
        return app;
    } else {
        if (strcmp(command, "man") == 0) {
            printf("Command %s not supported\n", arg[0]);
        } else {
            printf("Command %s not supported\n", command);
        }
    }

  COMMON_EXIT:
    JXTA_LOG("Returning application : %pp\n", app);
    printf("JXTA>");
    fflush(stdout);
    return app;
}


void ShellStartupApplication_printHelp(Jxta_object * appl)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *helpLine = jstring_new_2("#\tshell - Interactive enviroment for using JXTA.\n");
    jstring_append_2(helpLine, "SYNOPSIS\n");
    jstring_append_2(helpLine, "\tshell [-v] [-v] [-v] [-l file] [-f file]\n\n");
    jstring_append_2(helpLine, "DESCRIPTION\n");
    jstring_append_2(helpLine, "\tInteractive enviroment for using JXTA. ");
    jstring_append_2(helpLine, "OPTIONS\n");
    jstring_append_2(helpLine,
                     "\t-v\tProduce more verbose logging output by default. Each additional -v causes more output to be produced.\n");
    jstring_append_2(helpLine, "\t-f file\tSpecify the file into which log output will go.\n");
    jstring_append_2(helpLine, "\t-l file\tFile from which to read log settings.\n");

    if (app != NULL) {
        JxtaShellApplication_print(app, helpLine);
    } else {
        fprintf(stderr, jstring_get_string(helpLine));
    }
    JXTA_OBJECT_RELEASE(helpLine);
}


void ShellStartupApplication_delete(Jxta_object * appl)
{
    ShellStartupApplication *app = (ShellStartupApplication *) appl;
    if (app != NULL) {
        if (app->app != NULL) {
            JXTA_OBJECT_RELEASE(app->app);
        }

        free(app);
    }
}


void ShellStartupApplication_mainFinished(Jxta_object * parent, Jxta_object * child)
{
    ShellStartupApplication *app = (ShellStartupApplication *) parent;
    printf("JXTA>");
    fflush(stdout);
    if (app != NULL && app->app == (JxtaShellApplication *) child && child != NULL) {
        JXTA_OBJECT_RELEASE(child);
        app->app = NULL;
    }
}

void ShellStartupApplication_spawnApplication_terminate(Jxta_object * parent, Jxta_object * child)
{
    ShellStartupApplication *app = (ShellStartupApplication *) parent;

    if (app != NULL && app->currentProcess == (JxtaShellApplication *) child && child != NULL) {
        JXTA_OBJECT_RELEASE(app->currentProcess);
        app->currentProcess = 0;
    }
}

JxtaShellApplication *ShellStartupApplication_loadApplication(JxtaShellApplication * parent,
                                                              Jxta_object * parentObject, char *app)
{
    JxtaShellApplication *process = NULL;
    const struct commands *eachCommand = &allCommands[0];

    if (parent == NULL)
        return NULL;

    while (NULL != eachCommand->name) {
        if (strcmp(app, eachCommand->name) == 0) {
            Jxta_PG *pg;
            Jxta_listener *std_in;
            JxtaShellEnvironment *env;

            pg = JxtaShellApplication_peergroup(parent);
            std_in = JxtaShellApplication_getSTDIN(parent);
            env = JxtaShellApplication_getEnv(parent);
            process = eachCommand->constructor(pg, std_in, env, parentObject, ShellStartupApplication_spawnApplication_terminate);
            JXTA_OBJECT_RELEASE(env);
            JXTA_OBJECT_RELEASE(std_in);
            JXTA_OBJECT_RELEASE(pg);
            break;
        }
        eachCommand++;
    }

    return process;
}

static const char *verbositys[] = { "warning", "info", "debug", "trace", "paranoid" };

int main(int argc, char **argv)
{
    Jxta_PG *pg;
    Jxta_status status;
    ShellStartupApplication *startup;
    Jxta_log_file *log_f;
    Jxta_log_selector *log_s;
    char logselector[256];
    char *lfname = NULL;
    char *fname = NULL;
    JxtaShellGetopt *opts = JxtaShellGetopt_new(argc - 1, argv + 1, "f:l:v");
    int opt = -1;
    int verbosity = 0;
    int rc = 0;
    JString *tmp;

    jxta_initialize();

    while (-1 != (opt = JxtaShellGetopt_getNext(opts))) {
        switch (opt) {
        case 'v':
            verbosity++;
            break;
        case 'f':
            if (fname) {
                free(fname);
            }
            tmp = JxtaShellGetopt_getOptionArgument(opts);
            fname = strdup(jstring_get_string(tmp));
            JXTA_OBJECT_RELEASE(tmp);
            break;
        case 'l':
            if (lfname) {
                free(lfname);
            }
            tmp = JxtaShellGetopt_getOptionArgument(opts);
            lfname = strdup(jstring_get_string(tmp));
            JXTA_OBJECT_RELEASE(tmp);
            break;
        default:
            if (opt < 0) {
                JString *error = JxtaShellGetopt_getError(opts);
                fprintf(stderr, "# Error - %s \n", jstring_get_string(error));
                JXTA_OBJECT_RELEASE(error);
            } else {
                fprintf(stderr, "# Error - Bad option \n");
            }
            ShellStartupApplication_printHelp(NULL);
            return 1;
            break;
        }
    }

    if (NO_MORE_ARGS != JxtaShellGetopt_getErrorCode(opts)) {
        JString *error = JxtaShellGetopt_getError(opts);
        fprintf(stderr, "# %s - %s.\n", argv[0], jstring_get_string(error));
        JXTA_OBJECT_RELEASE(error);
        return -1;
    }

    JxtaShellGetopt_delete(opts);

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
        exit(-1);
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
                    exit(-1);
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
    status = jxta_PG_new_netpg(&pg);
    if (status != JXTA_SUCCESS) {
        fprintf(stderr, "# %s - jxta_PG_netpg_new failed with error: %ld\n", argv[0], status);
        rc = -1;
        goto final_exit;
    }

    startup = ShellStartup_new(pg, NULL, NULL);

    JXTA_OBJECT_CHECK_VALID(startup);

    ShellStartupApplication_start((Jxta_object *) startup, 0, NULL);
    jxta_module_stop((Jxta_module *) pg);

    JXTA_OBJECT_RELEASE(pg);

    JXTA_OBJECT_RELEASE(startup);

  final_exit:
    jxta_log_using(NULL, NULL);
    jxta_log_file_close(log_f);
    jxta_log_selector_delete(log_s);

    jxta_terminate();

    return rc;
}

/* vi: set ts=4 sw=4 tw=130 et: */
