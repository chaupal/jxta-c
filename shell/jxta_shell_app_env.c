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
 * $Id: jxta_shell_app_env.c,v 1.2 2004/12/05 02:16:39 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_shell_environment.h"
#include "jxta_peergroup.h"
#include "jxta_shell_app_env.h"


JxtaShellApplication * JxtaShellApp_env_new(Jxta_PG * pg,
                                            Jxta_listener* standout,
                                            JxtaShellEnvironment *env,
                                            Jxta_object * parent,
				      shell_application_terminate terminate){
    JxtaShellApplication *app  = 
            JxtaShellApplication_new(pg,standout,env,parent,terminate);

    if( app == 0) return 0;
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object*)app,
                                      JxtaShellApp_env_printHelp,
                                      JxtaShellApp_env_start,
                                      JxtaShellApp_env_processInput);
      
    return app;
}

void  JxtaShellApp_env_processInput(Jxta_object * appl,
                                    JString * inputLine){
  /** should not get here - in case we do, let's terminate ourself */
  JxtaShellApplication * app = (JxtaShellApplication*)appl;
  JxtaShellApplication_terminate(app);  
}


void JxtaShellApp_env_start(Jxta_object * appl,
                               int argv, 
                               char **arg){
   JxtaShellApplication * app = (JxtaShellApplication*)appl;
   JxtaShellEnvironment * env = JxtaShellApplication_getEnv(app);
   Jxta_vector* list = JxtaShellEnvironment_contains_values(env);
   Jxta_object **found;
   JString *line,*tmp;;

   if( list != 0){
     int i,max = jxta_vector_size (list);
     JxtaShellObject * obj;
 
     found = (Jxta_object**)malloc(sizeof(JxtaShellObject*)*1); 
     for(i = 0; i < max; i++){
        if( jxta_vector_get_object_at ( list, found,i) == JXTA_SUCCESS){ 
	  if( found[0] != 0){
            obj = (JxtaShellObject*)found[0];
            line = jstring_new_2("name=");
            tmp = JxtaShellObject_name(obj);
            if( tmp != 0){
               jstring_append_1(line,tmp);
               JXTA_OBJECT_RELEASE(tmp);
  	    }
            jstring_append_2(line,"; type=");
            tmp = JxtaShellObject_type(obj);
            if( tmp != 0){
               jstring_append_1(line,tmp);
               JXTA_OBJECT_RELEASE(tmp);
  	    }
            JXTA_OBJECT_SHARE(line);
            JxtaShellApplication_println(app,line);
            JXTA_OBJECT_RELEASE(line); 
	    JXTA_OBJECT_RELEASE(obj);
	  } 
	}
     }
     JXTA_OBJECT_RELEASE(list);

   }
   
   if( found != 0) free(found);
   if( env != 0) JXTA_OBJECT_RELEASE(env); 
   JxtaShellApplication_terminate(app);
}

void JxtaShellApp_env_printHelp(Jxta_object *appl){
	JxtaShellApplication * app = (JxtaShellApplication*)appl;
	JString * inputLine = jstring_new_2("env - Prints the environment variables \n\n");
	jstring_append_2(inputLine,"SYNOPSIS\n");
	jstring_append_2(inputLine,"   Takes no arguments \n");
	jstring_append_2(inputLine,"\n");
	if( app != 0){
		JXTA_OBJECT_SHARE(inputLine);
		JxtaShellApplication_print(app,inputLine);
	}
	JXTA_OBJECT_RELEASE(inputLine);
}
