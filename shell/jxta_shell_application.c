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
 * $Id: jxta_shell_application.c,v 1.4 2005/03/29 21:12:11 bondolo Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_shell_environment.h"

struct _jxta_shell_application {
    JXTA_OBJECT_HANDLE;
    Jxta_listener *listener;         /** The input pipe to this appliction */
    Jxta_listener *standout;          /** The output pipe of this application */
    shell_application_stdin standin;  /** The function to process input */
    shell_application_start start;   /** The start function */
    shell_application_printHelp help;  /** The function that prints help */
    Jxta_PG *peergroup;               /** The peer group of this application */
    shell_application_terminate terminate; /** function to call to terminate */
    Jxta_object *parent;                   /** The parent */
    Jxta_object *child;                     /** The child */
    JxtaShellEnvironment *env;            /** The shell environment to use */
};

/**
* Deletes the indicated application object
* @param app the application for which to add  the data
*/
void JxtaShellApplication_delete(Jxta_object * application);


JxtaShellApplication *JxtaShellApplication_new(Jxta_PG * pg,
                                               Jxta_listener * standout,
                                               JxtaShellEnvironment * env,
                                               Jxta_object * parent, shell_application_terminate terminate)
{

    JxtaShellApplication *app = (JxtaShellApplication *) calloc(1, sizeof(JxtaShellApplication));
    if (app == NULL) {
        return app;
    }

    JXTA_OBJECT_INIT(app, JxtaShellApplication_delete, 0);

    app->listener = jxta_listener_new((Jxta_listener_func) JxtaShellApplication_listenerFunction, app, 1, 200);

    if (app->listener == NULL) {
        JXTA_OBJECT_RELEASE(app);
        app = 0;
        return app;
    }

    app->standout = standout ? JXTA_OBJECT_SHARE(standout) : NULL;
    app->parent = parent ? JXTA_OBJECT_SHARE(parent) : NULL;
    app->terminate = terminate;
    app->standin = NULL;
    app->start = NULL;
    app->help = NULL;
    app->env = env ? JXTA_OBJECT_SHARE(env) : JxtaShellEnvironment_new(0);

    JxtaShellEnvironment_set_current_group(app->env, pg);

    jxta_listener_start(app->listener);

    return app;
}

void JxtaShellApplication_delete(Jxta_object * appl)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;


    if (app != NULL) {
        if (app->listener != NULL) {
            jxta_listener_stop(app->listener);
            JXTA_OBJECT_RELEASE(app->listener);
            app->listener = NULL;
        }

        if (app->env != NULL) {
            JXTA_OBJECT_RELEASE(app->env);
            app->env = NULL;
        }

        if (app->standout) {
            JXTA_OBJECT_RELEASE(app->standout);
            app->standout = NULL;
        }

        if (app->parent) {
            JXTA_OBJECT_RELEASE(app->parent);
            app->parent = NULL;
        }

        free(app);
    }
}


void JxtaShellApplication_start(JxtaShellApplication * app, int argv, char **arg)
{
    if (app != NULL && app->start != NULL) {
        app->start(app->child, argv, arg);
    }
}

void JxtaShellApplication_printHelp(JxtaShellApplication * app)
{
    if (app != NULL && app->start != NULL) {
        app->help(app->child);
        JxtaShellApplication_terminate(app);
    }
}

Jxta_listener *JxtaShellApplication_getSTDIN(JxtaShellApplication * app)
{
    Jxta_listener *listener = NULL;

    if (app != NULL) {
        listener = app->listener;
    }
    if (listener != NULL) {
        JXTA_OBJECT_SHARE(listener);
    }
    return listener;
}

JxtaShellEnvironment *JxtaShellApplication_getEnv(JxtaShellApplication * app)
{
    JxtaShellEnvironment *env = NULL;

    if (app != NULL) {
        env = app->env;
    }
    if (env != NULL) {
        JXTA_OBJECT_SHARE(env);
    }
    return env;
}

Jxta_PG *JxtaShellApplication_peergroup(JxtaShellApplication * app)
{
    Jxta_PG *pg = NULL;
    JString *name = jstring_new_2("currGroup");

    if (app != NULL) {
        JxtaShellObject *object = JxtaShellEnvironment_get(app->env, name);

        if (NULL != object) {
            pg = (Jxta_PG *) JxtaShellObject_object(object);
            JXTA_OBJECT_RELEASE(object);
            object = NULL;
        }
    }

    JXTA_OBJECT_RELEASE(name);
    name = NULL;
    return pg;
}

void JxtaShellApplication_terminate(JxtaShellApplication * app)
{
    if (app != NULL) {
        JString *prompt = jstring_new_2("JXTA>");
        JxtaShellApplication_print(app, prompt);
        app->terminate(app->parent, app->child);
        if (app->parent != 0) {
            JXTA_OBJECT_RELEASE(app->parent);
            app->parent = 0;
        }
    }
}

void JxtaShellApplication_setFunctions(JxtaShellApplication * app,
                                       Jxta_object * child,
                                       shell_application_printHelp help,
                                       shell_application_start start, shell_application_stdin standin)
{
    if (app != NULL) {
        app->child = child;
        app->help = help;
        app->start = start;
        app->standin = standin;
    }
}

Jxta_status JxtaShellApplication_print(JxtaShellApplication * app, JString * inputLine)
{
    int result = JXTA_INVALID_ARGUMENT;

    JXTA_OBJECT_CHECK_VALID(inputLine);

    if (app != NULL && app->standout != NULL && inputLine != NULL) {
        JXTA_OBJECT_CHECK_VALID(app->standout);
        JXTA_OBJECT_SHARE(inputLine);
        result = jxta_listener_schedule_object(app->standout, (Jxta_object *) inputLine);
        JXTA_OBJECT_RELEASE(inputLine);
    }
    return result;
}

Jxta_status JxtaShellApplication_println(JxtaShellApplication * app, JString * inputLine)
{
    int result = JXTA_INVALID_ARGUMENT;
    JString *line = jstring_new_0();

    jstring_append_1(line, inputLine);
    jstring_append_2(line, "\n");
    JXTA_OBJECT_SHARE(line);
    result = JxtaShellApplication_print(app, line);
    JXTA_OBJECT_RELEASE(line);
    JXTA_OBJECT_RELEASE(inputLine);
    return result;
}

void JxtaShellApplication_listenerFunction(Jxta_object * obj, void *arg)
{
    JxtaShellApplication *app = (JxtaShellApplication *) arg;
    JString *message = (JString *) obj;

    if (app != NULL && app->standin != NULL) {
        app->standin(app->child, message);
    }
}
