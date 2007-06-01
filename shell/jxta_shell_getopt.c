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
 * $Id: jxta_shell_getopt.c,v 1.3.4.1 2005/05/21 01:03:44 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <jxta.h>
#include "jxta_shell_getopt.h"


struct _jxta_shell_getopt {
    int argc;
    const char **argv;
    const char *optionChars;
    size_t optLength;
    int currentArg;
    JxtaShellGetOptError optError;
    JString *optArgument;
};

/** Private functions for ShellGetopt */

/**
* Sets the option argument and returns 0 if all is well or
* one of the relevant errors. The value for
* opt->optError gets set correctly
* @param opt the JxtaShellGetopt structure to use
*/
static JxtaShellGetOptError JxtaShellGetopt_setOptionArgument(JxtaShellGetopt * opt);


JxtaShellGetopt *JxtaShellGetopt_new(int argc, const char **argv, const char *optionChars)
{
    JxtaShellGetopt *opt = (JxtaShellGetopt *) malloc(sizeof(JxtaShellGetopt));
    
    if( NULL == opt ) {
        return NULL;
        }

    opt->argc = argc;
    opt->argv = argv;
    opt->optionChars = optionChars;
    opt->currentArg = -1;
    opt->optionChars = optionChars;
    opt->optLength = strlen(opt->optionChars);
    opt->optError = MISC_ERROR;
    opt->optArgument = NULL;
    return opt;
}

void JxtaShellGetopt_delete(JxtaShellGetopt * opt)
{
    if (opt != NULL) {
        if (opt->optArgument != NULL) {
            JXTA_OBJECT_RELEASE(opt->optArgument);
        }
        free(opt);
    }
}

int JxtaShellGetopt_getCurrentArgument(const JxtaShellGetopt * opt)
{
    int result = -1;

    if (opt != NULL && opt->currentArg < opt->argc) {
        result = opt->currentArg;
    }
    return result;
}

JString *JxtaShellGetopt_OptionArgument(const JxtaShellGetopt * opt)
{
    return opt->optArgument;
}

JString *JxtaShellGetopt_getOptionArgument(const JxtaShellGetopt * opt)
{
    JString *result = NULL;

    if (opt != NULL) {
        result = opt->optArgument;
        if (result != NULL)
            JXTA_OBJECT_SHARE(result);
    }
    
    return result;
}

JxtaShellGetOptError JxtaShellGetopt_getErrorCode(const JxtaShellGetopt * opt)
{
    JxtaShellGetOptError result = MISC_ERROR;

    if (opt != NULL) {
        result = opt->optError;
    }
    
    return result;
}

JString *JxtaShellGetopt_getError(const JxtaShellGetopt * opt)
{
    JString *result;

    if (opt == NULL) {
        result = jstring_new_2("An error occured");
        return result;
    }
    
    switch (opt->optError) {
    case OPT_NO_ERROR:
        result = jstring_new_2("No error");
        break;
    case NO_MORE_ARGS:
        result = jstring_new_2("No more options");
        break;
    case MISSING_OPTION:
        result = jstring_new_2("Missing option ");
        break;
     case MISSING_ARG:
        result = jstring_new_2("Missing argument for option ");
        jstring_append_0(result, &opt->argv[opt->currentArg][1], 1);
        break;
    case INVALID_OPTION:
        result = jstring_new_2("Options must start with -");
        break;
    case UNKNOWN_OPTION:
        result = jstring_new_2("Unknown option ");
        jstring_append_0(result, &opt->argv[opt->currentArg][1], 1);
        break;
    case MISC_ERROR:
    default:
        result = jstring_new_2("An error occured");
        break;
    }

    return result;
}

int JxtaShellGetopt_getNext(JxtaShellGetopt * opt)
{
    size_t length;
    size_t atChar = 0;
    size_t i;

    if (opt == NULL)
        return -1;

    opt->currentArg += 1;

    /** Reset the parameters */
    opt->optError = OPT_NO_ERROR;
    if (opt->optArgument != NULL) {
        JXTA_OBJECT_RELEASE(opt->optArgument);
        opt->optArgument = NULL;
    }

    /** If no  more arguments are found return  */
    if (opt->currentArg >= opt->argc) {
        opt->optError = NO_MORE_ARGS;
        return -1;
    }

    length = strlen(opt->argv[opt->currentArg]);

    if (length < 2) {
        opt->optError = MISSING_OPTION;
        return -1;
    }

    if ('-' != opt->argv[opt->currentArg][atChar]) {
        opt->optError = INVALID_OPTION;
        return -1;
    }

    atChar++;

    for (i = 0; i < opt->optLength; i++) {
        if (opt->argv[opt->currentArg][atChar] == opt->optionChars[i]) {
            atChar++;
            if (i + 1 < opt->optLength && opt->optionChars[i + 1] == ':') {
                opt->optError = JxtaShellGetopt_setOptionArgument(opt);

                if (OPT_NO_ERROR != opt->optError) {
                    return -1;
                }
            }

            return opt->optionChars[i];
        }
    }

    opt->optError = UNKNOWN_OPTION;
    return -1;
}

static JxtaShellGetOptError JxtaShellGetopt_setOptionArgument(JxtaShellGetopt * opt)
{
    size_t length = strlen(opt->argv[opt->currentArg]);
    const char *src;

    if (length > 2) {
        length -= 2;
        src = &opt->argv[opt->currentArg][2];
    } else {
        opt->currentArg += 1;

        if (opt->currentArg >= opt->argc) {
            return MISSING_ARG;
        }

        src = opt->argv[opt->currentArg];
        length = strlen(src);
    }

    opt->optArgument = jstring_new_1(length);
    jstring_append_0(opt->optArgument, src, length);

    return OPT_NO_ERROR;
}

#if 0
int main(int argc, char **argv){
   JxtaShellGetopt * opt = JxtaShellGetopt_new(argc-1, argv+1,"abc:de:"); 
   JString *argument;

   while(1){
     int type = JxtaShellGetopt_getNext(opt);
     int error = 0;
   
     if( type == -1) break;
     switch(type){
        case 'a':
          printf("Got option a\n");
	  break;
        case 'b':
          printf("Got option b\n");
	  break;
        case 'c':
          argument = JxtaShellGetopt_getOptionArgument(opt);
          printf("Got option c with argument %s\n", 
                  jstring_get_string(argument) );
          JXTA_OBJECT_RELEASE( argument);
	  break;
        case 'd':
          printf("Got option d\n");
	  break;
        case 'e':
          argument = JxtaShellGetopt_getOptionArgument(opt);
          printf("Got option e with argument %s\n", 
                  jstring_get_string(argument) );
          JXTA_OBJECT_RELEASE( argument);
	  break;
        default:
          argument = JxtaShellGetopt_getError(opt);
          printf("Error: %s \n", jstring_get_string ( argument));
          JXTA_OBJECT_RELEASE(argument);
          error = 1;
          break;
     }
     if( error != 0) break;
   }
   JxtaShellGetopt_delete(opt);
   return 0;
}
#endif

/* vi: set ts=4 sw=4 tw=130 et: */
