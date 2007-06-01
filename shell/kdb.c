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
 * $Id: kdb.c,v 1.2.4.1 2005/05/06 10:41:58 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_peergroup.h"
#include "jxta_membership_service.h"
#include "kdb.h"

#include "jxta_shell_getopt.h"

static Jxta_PG * group;
static JxtaShellEnvironment * environment;

JxtaShellApplication * jxta_kdb_new(Jxta_PG * pg,
				    Jxta_listener* standout,
				    JxtaShellEnvironment *env,
				    Jxta_object * parent,
				    shell_application_terminate terminate) {
    JxtaShellApplication *app  =
        JxtaShellApplication_new(pg,standout,env,parent,terminate);
    if( app == 0)
        return 0;
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object*)app,
                                      jxta_kdb_print_help,
                                      jxta_kdb_start,
                                      (shell_application_stdin)jxta_kdb_process_input);
    group = pg;
    environment = env;
    return app;
}


void  jxta_kdb_process_input(Jxta_object * appl,
			     JString * inputLine) {

    JxtaShellApplication * app = (JxtaShellApplication*)appl;
    JxtaShellApplication_terminate(app);
}

#ifdef JXTA_OBJECT_TRACKER 
extern void _print_object_table (void);
#endif

void jxta_kdb_start(Jxta_object * appl,
		    int argv,
		    char **arg) {

  JxtaShellApplication * app = (JxtaShellApplication*)appl;

#ifdef JXTA_OBJECT_TRACKER
    _print_object_table();
#else
    printf ("JXTA-C has not been compiled with JXTA_OBJECT_TRACKER defined\n");
#endif
    JxtaShellApplication_terminate(app);
}

void jxta_kdb_print_help(Jxta_object *appl) {

    JxtaShellApplication * app = (JxtaShellApplication*)appl;
    JString * inputLine = jstring_new_2("kdb  - Runs the internal debugger.\n");
    jstring_append_2(inputLine,"SYNOPSIS\n");
    jstring_append_2(inputLine,"\tkdb\n\n");
    jstring_append_2(inputLine,"DESCRIPTION\n");
    jstring_append_2(inputLine,"'kdb' runs the JXTA-C internal debugger " );
    jstring_append_2(inputLine,"OPTIONS\n");

    if( app != NULL ) {
        JxtaShellApplication_print(app,inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}
