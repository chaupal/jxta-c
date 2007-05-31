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
 * $Id: leave.c,v 1.2 2004/12/05 02:16:43 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_peergroup.h"
#include "jxta_membership_service.h"
#include "leave.h"

#include "jxta_shell_getopt.h"

static Jxta_PG * group;
static JxtaShellEnvironment * environment;

JxtaShellApplication * jxta_leave_new(Jxta_PG * pg,
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
                                      jxta_leave_print_help,
                                      jxta_leave_start,
                                      (shell_application_stdin)jxta_leave_process_input);
    group = pg;
    environment = env;
    return app;
}


void  jxta_leave_process_input(Jxta_object * appl,
                                JString * inputLine) {
    JxtaShellApplication * app = (JxtaShellApplication*)appl;
    JxtaShellApplication_terminate(app);
}


void jxta_leave_start(Jxta_object * appl,
                       int argv,
                       char **arg) {

    Jxta_status res;
    JxtaShellGetopt * opt = JxtaShellGetopt_new(argv,arg,"hr");
    JxtaShellApplication * app = (JxtaShellApplication*)appl;
    JString * outputLine = jstring_new_0();
    Jxta_PG * newPG = NULL;
    Jxta_boolean doResign = FALSE;

    while(1) {
        int type = JxtaShellGetopt_getNext(opt);
        int error = 0;

        if( type == -1)
            break;
        switch(type) {
		case 'h':
            jxta_leave_print_help(appl);
            break;
		case 'r':
            doResign = TRUE;
            break;
        default:
            jxta_leave_print_help(appl);
            error = 1;
            break;
        }
        if( error != 0 )
            goto Common_Exit;
    }

    JxtaShellGetopt_delete(opt);

    if( !JXTA_OBJECT_CHECK_VALID( group ) ) {
        jstring_append_2 ( outputLine, "# ERROR - invalid group object\n" );
        goto Common_Exit;
    }
    
    if( doResign ) {
        Jxta_membership_service * membership; 
        
        jxta_PG_get_membership_service( group, &membership );
        
        if( NULL != membership ) {
            jxta_membership_service_resign( membership );
            }
        }
    
    jxta_PG_get_parentgroup( group, &newPG );
    
    if( NULL == newPG ) {
        JString * grpname = jstring_new_2( "netPeerGroup" );
        JxtaShellObject * object = JxtaShellEnvironment_get ( environment, grpname );
        
        if( NULL != object )
            newPG = (Jxta_PG*) JxtaShellObject_object(object);
        JXTA_OBJECT_RELEASE(grpname); grpname = NULL;
        }
     
    if( NULL == newPG ) {
        jstring_append_2 ( outputLine, "# ERROR - could not get parent or net peer group!\n" );
        goto Common_Exit;
        }
        
    res = JxtaShellEnvironment_set_current_group ( environment, newPG);

Common_Exit:
    if( jstring_length( outputLine ) > 0 )
        JxtaShellApplication_print(app, outputLine );

    JxtaShellApplication_terminate(app);
}

void jxta_leave_print_help(Jxta_object *appl) {

    JxtaShellApplication * app = (JxtaShellApplication*)appl;
    JString * inputLine = jstring_new_2("#\tleave - leave a peer group \n\n");
    jstring_append_2(inputLine,"SYNOPSIS\n");
    jstring_append_2(inputLine,"\tleave [-r]\n\n");
    jstring_append_2(inputLine,"DESCRIPTION\n");
    jstring_append_2(inputLine,"\t'leave' allows you to leave a peer group and " );
    jstring_append_2(inputLine,"optionally resign from the group.\n\n" );
    jstring_append_2(inputLine,"OPTIONS\n");
    jstring_append_2(inputLine,"\t-h\tthis help information. \n");
    jstring_append_2(inputLine,"\t-r\tresign from the group. \n\n");

    if( app != NULL ) {
        JXTA_OBJECT_SHARE(inputLine);
        JxtaShellApplication_print(app,inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}
