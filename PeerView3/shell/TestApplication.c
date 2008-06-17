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

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_peergroup.h"
#include "TestApplication.h"

#include "jxta_shell_getopt.h"


JxtaShellApplication *JxtaTestApplication_new(Jxta_PG * pg,
                                              Jxta_listener * standout,
                                              JxtaShellEnvironment * env,
                                              Jxta_object * parent, shell_application_terminate terminate)
{
    JxtaShellApplication *app = JxtaShellApplication_new(pg, standout, env, parent, terminate);

    if (app == 0)
        return 0;
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object *) app,
                                      JxtaTestApplication_printHelp, JxtaTestApplication_start, JxtaTestApplication_processInput);

    return app;
}

void JxtaTestApplication_processInput(Jxta_object * appl, JString * inputLine)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    const char *line = jstring_get_string(inputLine);

    if (strcmp(line, "exit") == 0 && app != 0) {
        JxtaShellApplication_terminate(app);
    } else if (strcmp(line, "JXTA>") == 0 && app != 0) {
        JxtaShellApplication_terminate(app);
    } else {
        JString *result = jstring_new_0();
        jstring_append_2(result, "echo: ");
        jstring_append_1(result, inputLine);
        JxtaShellApplication_println(app, result);
        JXTA_OBJECT_RELEASE(result);
    }
}


void JxtaTestApplication_start(Jxta_object * appl, int argv, char **arg)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellGetopt *opt = JxtaShellGetopt_new(argv, arg, "arn:v:");
    JString *name = 0, *value = 0, *output;
    int add = 0;
    int error = 0;

    output = jstring_new_0();
    jstring_append_2(output, "Started TestApplication with arguments\n");
    JxtaShellApplication_print(app, output);
    JXTA_OBJECT_RELEASE(output);

    while (1) {
        int type = JxtaShellGetopt_getNext(opt);

        if (type == -1)
            break;
        switch (type) {
        case 'a':
            add = 1;
            break;
        case 'r':
            add = 2;
            break;
        case 'n':
            name = JxtaShellGetopt_OptionArgument(opt);
            break;
        case 'v':
            value = JxtaShellGetopt_OptionArgument(opt);
            break;
        default:
            output = jstring_new_0();
            jstring_append_2(output, "Error:");
            jstring_append_1(output, JxtaShellGetopt_getError(opt));
            JxtaShellApplication_println(app, output);
            JXTA_OBJECT_RELEASE(output);
            error = 1;
            break;
        }
        if (error != 0)
            break;
    }
    JxtaShellGetopt_delete(opt);
    if (error) {
        JxtaTestApplication_printHelp((Jxta_object *) app);
        JxtaShellApplication_terminate((Jxta_object *) app);
        if (name != 0)
            JXTA_OBJECT_RELEASE(name);
        if (value != 0)
            JXTA_OBJECT_RELEASE(value);
        return;
    }

    if (add == 0) {
        output = jstring_new_0();
        jstring_append_2(output, "Please indicate whether you want to delete or remove \n");
        JxtaShellApplication_println(app, output);
        JXTA_OBJECT_RELEASE(output);
        JxtaTestApplication_printHelp(app);
        JxtaShellApplication_terminate(app);
        if (name != 0)
            JXTA_OBJECT_RELEASE(name);
        if (value != 0)
            JXTA_OBJECT_RELEASE(value);
        return;
    }
    if (name == 0) {
        output = jstring_new_0();
        jstring_append_2(output, "Please supply a name for the environment variable \n");
        JxtaShellApplication_println(app, output);
        JXTA_OBJECT_RELEASE(output);
        JxtaTestApplication_printHelp(app);
        JxtaShellApplication_terminate(app);
        if (name != 0)
            JXTA_OBJECT_RELEASE(name);
        if (value != 0)
            JXTA_OBJECT_RELEASE(value);
        return;
    }
    if (value == 0)
        value = jstring_new_2("Missing value");
    if (app != 0) {
        JxtaShellEnvironment *env = JxtaShellApplication_getEnv(app);
        if (add == 1) {
            JxtaShellEnvironment_add_2(env, name, (Jxta_object *) value);
        } else {
            JxtaShellEnvironment_delete(env, name);
        }
        if (env != 0)
            JXTA_OBJECT_RELEASE(env);
    }

    if (name != 0)
        JXTA_OBJECT_RELEASE(name);
    if (value != 0)
        JXTA_OBJECT_RELEASE(value);
    output = jstring_new_0();
    jstring_append_2(output, "Typed strings will be echoed back. \n");
    jstring_append_2(output, "To terminate type exit. \n");
    JxtaShellApplication_print(app, output);
    JXTA_OBJECT_RELEASE(output);
}

void JxtaTestApplication_printHelp(Jxta_object * appl)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *inputLine = jstring_new_2("TestApplication - Tests some aspects of the shell \n\n");
    jstring_append_2(inputLine, "SYNOPSIS\n");
    jstring_append_2(inputLine, "   TestApplication  [-a] Add an environment variable\n");
    jstring_append_2(inputLine, "                    [-r] Remove an environment variable\n");
    jstring_append_2(inputLine, "                    [-n] The name of the variable to add or remove\n");
    jstring_append_2(inputLine, "                    [-v] The value of the variable \n");
    jstring_append_2(inputLine, "\n");
    jstring_append_2(inputLine, "Type data to stdin and press return - the application will echo it back.\n");
    jstring_append_2(inputLine, "Typing exit will terminate \n");
    if (app != 0) {
        JxtaShellApplication_print(app, inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}
