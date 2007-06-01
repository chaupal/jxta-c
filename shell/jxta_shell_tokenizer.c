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
 * $Id: jxta_shell_tokenizer.c,v 1.2 2005/08/24 01:21:21 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta_shell_tokenizer.h"


/**
*  Where do we put functions that are private to the object?
*  For the lack of a better place I put them here 
*/

/** Private functions for JxtaShellTokenizer */

struct _jxta_shell_tokenizer {
    JXTA_OBJECT_HANDLE;
    const char *line;     /** The line we are currently using */
    const char *delim;    /** The characters on which we want to split */
    int current;          /** The character we are currently at */
    int tokens;           /** The number of tokens we can return */
    int returnDelim;      /** Do we want to return the characters on which we split */
};

/**
* Advance to the next character from which we would start
* a new token. Return the index of the character we would
* start with.
* @param tok the  JxtaShellTokenizer object to use
* @param start the start character
* @param wordChar if 0 we advance to the next non-delimiter char
*                 if 1 we advance to the next delimiter char
*/
int JxtaShellTokenizer_advanceToNextChar(JxtaShellTokenizer * tok, int start, int wordChar);

/**
* Is the  character token a delimiter
* @param tok the  JxtaShellTokenizer object to use
* @param token the character to check
* @param return 1 if token is a delimiter character 0  otherwise
*/
int JxtaShellTokenizer_isDelimiter(JxtaShellTokenizer * tok, char token);

/**
*  Deletes the JxtaShellTokenizer object
*/
void JxtaShellTokenizer_delete(Jxta_object * tok);

JxtaShellTokenizer *JxtaShellTokenizer_new(const char *line, const char *delim, int returnDelim)
{
    JxtaShellTokenizer *tok = (JxtaShellTokenizer *) malloc(sizeof(JxtaShellTokenizer));

    if (NULL == tok)
        return NULL;

    memset(tok, 0, sizeof(*tok));

    JXTA_OBJECT_INIT(tok, JxtaShellTokenizer_delete, 0);

    tok->line = line;
    tok->delim = delim;
    tok->current = 0;
    tok->tokens = -1;
    if (returnDelim != 0)
        tok->returnDelim = 1;
    else
        tok->returnDelim = returnDelim;

    return tok;
}


void JxtaShellTokenizer_delete(Jxta_object * tok)
{
    JxtaShellTokenizer *tokenizer = (JxtaShellTokenizer *) tok;
    if (tokenizer != NULL) {
        free(tokenizer);
    }
}

int JxtaShellTokenizer_hasMoreTokens(JxtaShellTokenizer * tok)
{
    int result = 0;

    if (tok != 0) {
        if (tok->returnDelim != 0) {
            result = JxtaShellTokenizer_advanceToNextChar(tok, tok->current, 0);
            if (result < 0)
                result = 0;
            else
                result = 1;
        } else {
            if (tok->line != 0 && tok->current < strlen(tok->line)) {
                result = 1;
            }
        }
    }
    return result;
}

char *JxtaShellTokenizer_nextToken(JxtaShellTokenizer * tok)
{
    char *result = 0;
    int length, tmp, i;

    if (tok != 0 && tok->line != 0 && tok->delim != 0) {
        length = strlen(tok->line);
        if (tok->current < length) {
            if (JxtaShellTokenizer_isDelimiter(tok, tok->line[tok->current]) != 0) {
                if (tok->returnDelim == 0) {
                    result = (char *) malloc(sizeof(char) * 2);
                    if (result != 0) {
                        result[0] = tok->line[tok->current];
                        result[1] = '\0';
                    }
                    tok->current++;
                } else {
                    tok->current = JxtaShellTokenizer_advanceToNextChar(tok, tok->current, 0);
                    if (tok->current == -1)
                        tok->current = length - 1;
                    if (tok->current < 0)
                        tok->current = 0;
                }
            }
        }
     /** Now we are correctly positioned */
        if (tok->current < length && result == 0) {
            tmp = JxtaShellTokenizer_advanceToNextChar(tok, tok->current, 1);
            if (tmp == -1)
                tmp = length;
            result = (char *) malloc(sizeof(char) * (tmp + 1 - tok->current));
            for (i = 0; i < tmp - tok->current; i++) {
                result[i] = tok->line[tok->current + i];
            }
            result[tmp - tok->current] = '\0';
            tok->current = tmp;
        }
    }
    return result;
}

int JxtaShellTokenizer_countTokens(JxtaShellTokenizer * tok)
{
    int result = 0, tmp, length;

    if (tok != 0) {
        if (tok->returnDelim != 0) {
            tmp = JxtaShellTokenizer_advanceToNextChar(tok, tok->current, 0);
            while (tmp != -1) {
                result++;
                tmp = JxtaShellTokenizer_advanceToNextChar(tok, tmp, 1);
                if (tmp == -1)
                    break;
                tmp = JxtaShellTokenizer_advanceToNextChar(tok, tmp, 0);
            }
        } else if (tok->line != 0) {
            tmp = tok->current;
            length = strlen(tok->line);
            while (tmp < length && JxtaShellTokenizer_isDelimiter(tok, tok->line[tmp]) != 0) {
                result++;
                tmp++;
            }
            if (tmp >= length)
                tmp = -1;
            while (tmp != -1) {
                result++;
                tmp = JxtaShellTokenizer_advanceToNextChar(tok, tmp, 1);
                if (tmp == length || tmp == -1)
                    break;
                while (tmp < length && JxtaShellTokenizer_isDelimiter(tok, tok->line[tmp]) != 0) {
                    result++;
                    tmp++;
                }
                if (tmp >= length)
                    break;
            }
        }
    }
    return result;
}

int JxtaShellTokenizer_isDelimiter(JxtaShellTokenizer * tok, char token)
{
    int result = 0;
    int length, i;

    if (tok != 0 && tok->delim != 0) {
        length = strlen(tok->delim);
        for (i = 0; i < length; i++) {
            if (token == tok->delim[i]) {
                result = 1;
                break;
            }
        }
    }
    return result;
}

int JxtaShellTokenizer_advanceToNextChar(JxtaShellTokenizer * tok, int start, int wordChar)
{
    int result = -1;
    int length;

    if (tok != 0 && tok->line != 0 && tok->delim != 0) {
        length = strlen(tok->line);
        result = start;

        if (result < length) {
            for (; result < length; result++) {
                if (JxtaShellTokenizer_isDelimiter(tok, tok->line[result]) == 0) {
                    if (wordChar == 0)
                        break;
                } else {
                    if (wordChar != 0)
                        break;
                }
            }
            if (result >= length)
                result = -1;
        } else
            result = -1;
    }
    return result;
}

/*
int main(int argv, char **arg){
  char *delim = "\n\r\t |",*token;
  JxtaShellTokenizer * tok = JxtaShellTokenizer_new(arg[1],delim,1);
  int result;
  
  if( tok == 0) {
    printf("Cannot instantiate tokenizer \n");
    return 1;
  }
  printf("Argument with delimiters not counted: \n");
  printf("Number of words is: %d\n",JxtaShellTokenizer_countTokens(tok));
  while(1){
     result = JxtaShellTokenizer_hasMoreTokens(tok);
     if( result == 0) break;
     token = JxtaShellTokenizer_nextToken(tok);
     printf("Next token is >%s< ",token);
     free(token);
     printf(" and it remain %d tokens. \n",
                 JxtaShellTokenizer_countTokens(tok));
  }
  JXTA_OBJECT_RELEASE(tok);
 
  tok = JxtaShellTokenizer_new(arg[1],delim,0);
  if( tok == 0) {
    printf("Cannot instantiate tokenizer \n");
    return 1;
  }
  printf("\nArgument with delimiters counted: \n");
  printf("Number of words is: %d\n",JxtaShellTokenizer_countTokens(tok));
  while(1){
     result = JxtaShellTokenizer_hasMoreTokens(tok);
     if( result == 0) break;
     token = JxtaShellTokenizer_nextToken(tok);
     printf("Next token is >%s< ",token);
     free(token);
     printf(" and it remain %d tokens. \n",
               JxtaShellTokenizer_countTokens(tok));
  }
  JXTA_OBJECT_RELEASE(tok);
  return 0;
}*/
