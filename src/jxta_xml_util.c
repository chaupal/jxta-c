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
 * $Id: jxta_xml_util.c,v 1.18 2005/01/12 22:37:45 bondolo Exp $
 */

#include <stdlib.h>
#include <apr_strings.h>

#include "jxta_debug.h"
#include "jxta_errno.h"
#include "jxta_object.h"
#include "jxta_xml_util.h"

#define DEBUG 0

/* Ok, here is the deal: apr is not dealing with 
 * arpa/ and netinet/ very well.  At some point 
 * it probably will.  In the meantime, this stuff
 * will have to be manually defined the way we want it.
 * It will likely be a full days work to sort it out
 * among linux, solaris, win32, bsd etc.
 */
#ifndef WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

#include <ctype.h>
#include <apr_want.h>
#include <apr_network_io.h>

#include "jxta_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/** Single call to extract an ip address and port number from 
 *  a character buffer. 
 *
 * @param  string contains character data in the form "127.0.0.1:9700"
 *         corresponding to ip address and port number.
 * @param  length is necessary because of the way strtok works.  We
 *         don't want to operate on possibly const char data, so we 
 *         need to copy it to a temporary buffer, which is statically 
 *         preallocated.  Copying only the first length chars prevents 
 *         buffer nastiness.
 * @param  ip points to a calling address.  What we really want is to 
 *         return two values which could be done by dropping the value
 *         of a struct on the stack, or passing in addresses.  Passing 
 *         in addresses works for now.
 * @param  port works the same way as the ip address does, represents the 
 *         second return address.
 * 
 * @return Two return arguments, ip and port, are passed back through 
 *         pointers in the function argument list.
 *
 * @todo   Make sure that apr_strtok is reentrant.
 *
 * @todo   Implement some way for this function to degrade gracefully
 *         if someone tries to ram a huge buffer into it.  Or maybe that 
 *         won't be an issue. At any rate, the behavior for data of 
 *         length 129 should be documented.
 *
 * @todo   Implement some of way handling bogus data nicely. For example,
 *         if someone feeds us something like "foo.bar.baz.foo:abcd", 
 *         it would be nice to do something intelligent with it rather 
 *         than relying on the underlying library to barf.  Or at least 
 *         catch the barf here.
 */ 
void
extract_ip_and_port(const char * string,
		    int length,
		    Jxta_in_addr * ip,
		    Jxta_port    * port) {

  /* for apr_strtoks */
  char * state = NULL;
  char * current_tok = NULL;
  const char * DELIMITERS = ": \t\n";
  /* This buffer MUST be initted to \0, 
   * otherwise strtok will fail.  apr_strtok 
   * will probably fail also, as it uses strchr
   * to walk the buf.
   */
  char * temp = malloc (length + 1);
  memset (temp, 0, length + 1);
  /* char temp[128] = {0}; */ /* temporary for chasing mem leak. */

  strncpy(temp,string,length);
  JXTA_LOG("temp string copied extract_ip_and_port: %s\n",temp);

  /* Can use strtok also. */
  /*  current_tok = strtok(temp,DELIMITERS); */
  current_tok = apr_strtok(temp,DELIMITERS,&state);


  if (current_tok == NULL) {
    /*  whoops empty line no data */
    free (temp);
    return;
  }

#if DEBUG
  JXTA_LOG("current_tok: %s, length: %d\n",current_tok,strlen(current_tok)); 
#endif
 *ip = apr_inet_addr(current_tok); 

 /* ip = 0 indicates current_tok was invalid data.
  * FIXME: Trap the error here, using whatever apr has 
  * to deal with it.  We could just return since we are 
  * are 0 for an uninitialized variable flag. Returning
  * gets us out of the situation like "foo.bar.baz.foo:9700"
  * which could correctly set the port number.
  */
  if (ip == 0) {
    free (temp);
    return;
  }


  /* strtok also works. */
  /* current_tok = strtok(NULL,DELIMITERS);  */
  current_tok = apr_strtok(NULL,DELIMITERS,&state);
#if DEBUG
  JXTA_LOG("current_tok: %s, length: %d\n",current_tok,strlen(current_tok)); 
#endif
  /* Checking this value for error in c is a bitch.  The fastest
   * way to make it robust is probably to write a set of functions
   * to see how it acts on various platforms.  gnu recommends 
   * using to strtol instead of atoi, which still is messy for 
   * checking errors.
   * TODO: See what MS recommends here, and how apr handles it.
   */ 
 *port = atoi(current_tok); 
#if DEBUG
  JXTA_LOG("port number as int: %d\n",*port);
#endif
  free (temp);

}


/** API comments in header file. */
void 
extract_ip(const char * buf,
           int buf_length,
           Jxta_in_addr * ip_address) {

  /* for apr_strtoks */
  char * state;
  char * current_tok = NULL;
  const char * DELIMITERS = ": \t\n";
  /* This buffer MUST be initted to \0, 
   * otherwise strtok will fail.  apr_strtok 
   * will probably fail also, as it uses strchr
   * to walk the buf.
   */
  char * temp = malloc (buf_length + 1);
  memset (temp, 0, buf_length + 1);

  strncpy(temp,buf,buf_length);
  /* Can use strtok also. */
  /*  current_tok = strtok(temp,DELIMITERS); */
  current_tok = apr_strtok(temp,DELIMITERS,&state);

  if (current_tok == NULL) {
    /*  whoops empty line no data */
    free (temp);
    return;
  }

#if DEBUG
  printf("current_tok: %s, length: %d\n",current_tok,strlen(current_tok)); 
#endif
 /* ip = 0 indicates current_tok was invalid data.
  * FIXME: Trap the error here, using whatever apr has 
  * to deal with it.  We could just return since we are 
  * are 0 for an uninitialized variable flag. 
  */
 *ip_address = apr_inet_addr(current_tok); 
 free (temp);
}




/** API comments in header file. */
void 
extract_port               (const char * buf,
                            int buf_length,
                            Jxta_port * port) {

  /* for apr_strtoks */
  char * state;
  char * current_tok = NULL;
  const char * DELIMITERS = ": \t\n";
  /* This buffer MUST be initted to \0, 
   * otherwise strtok will fail.  apr_strtok 
   * will probably fail also, as it uses strchr
   * to walk the buf.
   */
  char * temp = malloc (buf_length + 1);
  memset (temp, 0, buf_length + 1);

#if DEBUG
  printf("buf: %s, buf_length: %d\n",buf,buf_length);
#endif

  strncpy(temp,buf,buf_length);
  /* Can use strtok also. */
  /*  current_tok = strtok(temp,DELIMITERS); */
  current_tok = apr_strtok(temp,DELIMITERS,&state);

  if (current_tok == NULL) {
    /*  whoops empty line no data */
    free (temp);
    return;
  }
#if DEBUG
  printf("current_tok: %s, length: %d\n",current_tok,strlen(current_tok)); 
#endif
  /* Checking this value for error in c is a bitch.  The fastest
   * way to make it robust is probably to write a set of functions
   * to see how it acts on various platforms.  gnu recommends 
   * using to strtol instead of atoi, which still is messy for 
   * checking errors.
   * TODO: See what MS recommends here, and how apr handles it.
   */ 
 *port = atoi(current_tok); 
#if DEBUG
  printf("port number as int: %d\n",*port);
#endif

  /* TODO: If we were *really* motivated, we could check the 
   * value of the port number to make sure it was in range.
   */
  free (temp);
}


/** See header file for documentation. */
void
extract_char_data(const char * string,int strlength,char * tok) {

   const char * tmp;
   const char * start;

   if (string == NULL) {
     tok [0] = 0;
     return;
   }

   tmp = string;
   /* Strip off any /t /n /r space from the begining of the value */
   while (tmp && ((tmp - string) < strlength) && isspace(*tmp)) ++tmp;

   if ((tmp == NULL) || (*tmp == 0)) {
     /* The value is empty */
     tok[0] = 0;
     return;
   }
   start = tmp;

   /* Get the end of the string value */
   while (tmp && ((tmp - string) < strlength) && !isspace(*tmp) && (*tmp != 0)) ++tmp;

   memcpy (tok, start, tmp - start);
   tok[tmp - start] = 0;
}

/**
*  Removes escapes from the contents of a JString that has been passed as XML
*  element content.
*  
* @param src the string to decode.
* @param dest the resulting decoded string.
* @result JXTA_SUCCESS for success, JXTA_INVALID_ARGUMENT for bad params and
*  (rarely) JXTA_NOMEM
**/
Jxta_status jxta_xml_util_decode_jstring( JString * src, JString** dest ) {
  const char* srcbuf;
  const char* startchunk;
  
  if( !JXTA_OBJECT_CHECK_VALID(src) )
    return JXTA_INVALID_ARGUMENT;
  
  if( NULL == dest )
    return JXTA_INVALID_ARGUMENT;
    
  srcbuf = jstring_get_string( src );
  
  if( NULL == srcbuf ) {
    return JXTA_INVALID_ARGUMENT;
    }

  *dest = jstring_new_1( jstring_length(src) );
  
  if( *dest == NULL )
    return JXTA_NOMEM;

  startchunk = srcbuf;
  
  do {
    switch( *srcbuf ) {
      case 0 : {
         /* append any remaining part */
        jstring_append_0( *dest, startchunk, srcbuf - startchunk );
        return JXTA_SUCCESS;
        }
        
      case '&' : {
        const char* endchar = strchr( srcbuf + 1, ';' );
        
        if( NULL != endchar ) {
            if( 0 == strncmp( "&amp;", srcbuf, endchar - srcbuf + 1) ) {
              /* copy everything up to this char */
              jstring_append_0( *dest, startchunk, srcbuf - startchunk );

              /* append replacement */
              jstring_append_0( *dest, "&", 1 );
            } else if( 0 == strncmp( "&lt;", srcbuf, endchar - srcbuf + 1) ) {
              /* copy everything up to this char */
              jstring_append_0( *dest, startchunk, srcbuf - startchunk );

              /* append replacement */
              jstring_append_0( *dest, "<", 1 );
            } else if( 0 == strncmp( "&gt;", srcbuf, endchar - srcbuf + 1) ) {
              /* copy everything up to this char */
              jstring_append_0( *dest, startchunk, srcbuf - startchunk );

              /* append replacement */
              jstring_append_0( *dest, ">", 1 );
              }
            }
          else {
            /* FIXME 20020319 bondolo@jxta.org when we figure out what we are
               doing with unicode we need to fix numeric character references here
            */
            break;
          }

          /* next loop around we want to do one char past endchar */
          srcbuf = endchar;
          startchunk = srcbuf + 1;
        }
        break;
        
      default :
        /* do nothing. */
        break;
      }
    
    srcbuf++;
    } 
  while( 1 );

  return JXTA_SUCCESS;
}

/**
*   Escapes the contents of a JString to make it suitable for passing as xml
*   element content.
*  
* @param src the string to encode.
* @param dest the resulting encoded string.
* @result JXTA_SUCCESS for success, JXTA_INVALID_ARGUMENT for bad params and
*  (rarely) JXTA_NOMEM
**/
Jxta_status jxta_xml_util_encode_jstring( JString * src, JString** dest ) {
  const char* srcbuf;
  const char* startchunk;
  
  if( !JXTA_OBJECT_CHECK_VALID(src) )
    return JXTA_INVALID_ARGUMENT;
  
  if( NULL == dest )
    return JXTA_INVALID_ARGUMENT;
    
  srcbuf = jstring_get_string( src );
  
  if( NULL == srcbuf ) {
    return JXTA_INVALID_ARGUMENT;
    }

  *dest = jstring_new_1( jstring_length(src) );
  
  if( *dest == NULL )
    return JXTA_NOMEM;

  startchunk = srcbuf;
  
  do {
    switch( *srcbuf ) {
      case 0 : {
         /* append any remaining part */
        jstring_append_0( *dest, startchunk, srcbuf - startchunk );
        return JXTA_SUCCESS;
        }
        
      case '<' : {
        /* copy everything up to this char */
        jstring_append_0( *dest, startchunk, srcbuf - startchunk );
        startchunk = srcbuf + 1;

        /* append replacement */
        jstring_append_0( *dest, "&lt;", 4 );
        }
        break;
        
      /* 
         FIXME bondolo@jxta.org 20020410 this replacement should not
         be necessary but was added for compatibility with the j2se
         implementation.
      */
      case '>' : {
        /* copy everything up to this char */
        jstring_append_0( *dest, startchunk, srcbuf - startchunk );
        startchunk = srcbuf + 1;

        /* append replacement */
        jstring_append_0( *dest, "&gt;", 4 );
        }
        break;
        
      case '&' : {
        /* copy everything up to this char */
        jstring_append_0( *dest, startchunk, srcbuf - startchunk );
        startchunk = srcbuf + 1;

        /* append replacement */
        jstring_append_0( *dest, "&amp;", 5 );
        }
        break;
        
      default :
        /* do nothing. */
        break;
      }
    
    srcbuf++;
    } 
  while( 1 );

  return JXTA_SUCCESS;
}

#ifdef __cplusplus
}
#endif
