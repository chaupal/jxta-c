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

static const char *__log_cat = "XMLUTIL";

#include <stdlib.h>
#include <ctype.h>
#if defined(GZIP_ENABLED) || defined(GUNZIP_ENABLED)
#include <zlib.h>
#endif

#include "jxta_apr.h"
#include "jxta_types.h"
#include "jxta_log.h"
#include "jxta_errno.h"
#include "jxta_object.h"
#include "jxta_xml_util.h"


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
JXTA_DECLARE(void) extract_ip_and_port(const char *string, int length, Jxta_in_addr * ip, Jxta_port * port)
{

    /* for apr_strtoks */
    char *state = NULL;
    char *current_tok = NULL;
    const char *DELIMITERS = ": \t\n";
    /* This buffer MUST be initted to \0, 
     * otherwise strtok will fail.  apr_strtok 
     * will probably fail also, as it uses strchr
     * to walk the buf.
     */
    char *temp = (char *) malloc(length + 1);
    memset(temp, 0, length + 1);
    /* char temp[128] = {0}; *//* temporary for chasing mem leak. */

    strncpy(temp, string, length);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "temp string copied extract_ip_and_port: %s\n", temp);

    /* Can use strtok also. */
    /*  current_tok = strtok(temp,DELIMITERS); */
    current_tok = apr_strtok(temp, DELIMITERS, &state);


    if (current_tok == NULL) {
        /*  whoops empty line no data */
        free(temp);
        return;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "current_tok: %s, length: %d\n", current_tok, strlen(current_tok));
    *ip = apr_inet_addr(current_tok);

    /* ip = 0 indicates current_tok was invalid data.
     * FIXME: Trap the error here, using whatever apr has 
     * to deal with it.  We could just return since we are 
     * are 0 for an uninitialized variable flag. Returning
     * gets us out of the situation like "foo.bar.baz.foo:9700"
     * which could correctly set the port number.
     */
    if (ip == 0) {
        free(temp);
        return;
    }


    /* strtok also works. */
    /* current_tok = strtok(NULL,DELIMITERS);  */
    current_tok = apr_strtok(NULL, DELIMITERS, &state);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "current_tok: %s, length: %d\n", current_tok, strlen(current_tok));
    /* Checking this value for error in c is a bitch.  The fastest
     * way to make it robust is probably to write a set of functions
     * to see how it acts on various platforms.  gnu recommends 
     * using to strtol instead of atoi, which still is messy for 
     * checking errors.
     * TODO: See what MS recommends here, and how apr handles it.
     */
    *port = atoi(current_tok);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "port number as int: %d\n", *port);
    free(temp);

}


/** API comments in header file. */
JXTA_DECLARE(void) extract_ip(const char *buf, int buf_length, Jxta_in_addr * ip_address)
{

    /* for apr_strtoks */
    char *state;
    char *current_tok = NULL;
    const char *DELIMITERS = ": \t\n";
    /* This buffer MUST be initted to \0, 
     * otherwise strtok will fail.  apr_strtok 
     * will probably fail also, as it uses strchr
     * to walk the buf.
     */
    char *temp = (char *) malloc(buf_length + 1);
    memset(temp, 0, buf_length + 1);

    strncpy(temp, buf, buf_length);
    /* Can use strtok also. */
    /*  current_tok = strtok(temp,DELIMITERS); */
    current_tok = apr_strtok(temp, DELIMITERS, &state);

    if (current_tok == NULL) {
        /*  whoops empty line no data */
        free(temp);
        return;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "current_tok: %s, length: %d\n", current_tok, strlen(current_tok));
    /* ip = 0 indicates current_tok was invalid data.
     * FIXME: Trap the error here, using whatever apr has 
     * to deal with it.  We could just return since we are 
     * are 0 for an uninitialized variable flag. 
     */
    *ip_address = apr_inet_addr(current_tok);
    free(temp);
}

/** API comments in header file. */
JXTA_DECLARE(void) extract_port(const char *buf, int buf_length, Jxta_port * port)
{

    /* for apr_strtoks */
    char *state;
    char *current_tok = NULL;
    const char *DELIMITERS = ": \t\n";
    /* This buffer MUST be initted to \0, 
     * otherwise strtok will fail.  apr_strtok 
     * will probably fail also, as it uses strchr
     * to walk the buf.
     */
    char *temp = (char *) malloc(buf_length + 1);
    memset(temp, 0, buf_length + 1);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "buf: %s, buf_length: %d\n", buf, buf_length);

    strncpy(temp, buf, buf_length);
    /* Can use strtok also. */
    /*  current_tok = strtok(temp,DELIMITERS); */
    current_tok = apr_strtok(temp, DELIMITERS, &state);

    if (current_tok == NULL) {
        /*  whoops empty line no data */
        free(temp);
        return;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "current_tok: %s, length: %d\n", current_tok, strlen(current_tok));

    /* Checking this value for error in c is a bitch.  The fastest
     * way to make it robust is probably to write a set of functions
     * to see how it acts on various platforms.  gnu recommends 
     * using to strtol instead of atoi, which still is messy for 
     * checking errors.
     * TODO: See what MS recommends here, and how apr handles it.
     */
    *port = atoi(current_tok);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "port number as int: %d\n", *port);

    /* TODO: If we were *really* motivated, we could check the 
     * value of the port number to make sure it was in range.
     */
    free(temp);
}

/** See header file for documentation. */
JXTA_DECLARE(void) extract_char_data(const char *string, int strlength, char *tok)
{

    const char *tmp;
    const char *start;

    if (string == NULL) {
        tok[0] = 0;
        return;
    }

    tmp = string;
    /* Strip off any /t /n /r space from the begining of the value */
    while (tmp && ((tmp - string) < strlength) && isspace(*tmp))
        ++tmp;

    if ((tmp == NULL) || (*tmp == 0)) {
        /* The value is empty */
        tok[0] = 0;
        return;
    }
    start = tmp;

    /* Get the end of the string value */
    while (tmp && ((tmp - string) < strlength) && !isspace(*tmp) && (*tmp != 0))
        ++tmp;

    memcpy(tok, start, tmp - start);
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
JXTA_DECLARE(Jxta_status) jxta_xml_util_decode_jstring(JString * src, JString ** dest)
{
    const char *srcbuf;
    const char *startchunk;

    if (!JXTA_OBJECT_CHECK_VALID(src))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == dest)
        return JXTA_INVALID_ARGUMENT;

    srcbuf = jstring_get_string(src);

    if (NULL == srcbuf) {
        return JXTA_INVALID_ARGUMENT;
    }

    *dest = jstring_new_1(jstring_length(src));

    if (*dest == NULL)
        return JXTA_NOMEM;

    startchunk = srcbuf;

    do {
        switch (*srcbuf) {
        case 0:{
                /* append any remaining part */
                jstring_append_0(*dest, startchunk, srcbuf - startchunk);
                return JXTA_SUCCESS;
            }

        case '&':{
                const char *endchar = strchr(srcbuf + 1, ';');

                if (NULL != endchar) {
                    if (0 == strncmp("&amp;", srcbuf, endchar - srcbuf + 1)) {
                        /* copy everything up to this char */
                        jstring_append_0(*dest, startchunk, srcbuf - startchunk);

                        /* append replacement */
                        jstring_append_0(*dest, "&", 1);
                    } else if (0 == strncmp("&lt;", srcbuf, endchar - srcbuf + 1)) {
                        /* copy everything up to this char */
                        jstring_append_0(*dest, startchunk, srcbuf - startchunk);

                        /* append replacement */
                        jstring_append_0(*dest, "<", 1);
                    } else if (0 == strncmp("&gt;", srcbuf, endchar - srcbuf + 1)) {
                        /* copy everything up to this char */
                        jstring_append_0(*dest, startchunk, srcbuf - startchunk);

                        /* append replacement */
                        jstring_append_0(*dest, ">", 1);
                    }
                } else {
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

        default:
            /* do nothing. */
            break;
        }

        srcbuf++;
    }
    while (1);
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
JXTA_DECLARE(Jxta_status) jxta_xml_util_encode_jstring(JString * src, JString ** dest)
{
    const char *srcbuf;
    const char *startchunk;

    if (!JXTA_OBJECT_CHECK_VALID(src))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == dest)
        return JXTA_INVALID_ARGUMENT;

    srcbuf = jstring_get_string(src);

    if (NULL == srcbuf) {
        return JXTA_INVALID_ARGUMENT;
    }

    *dest = jstring_new_1(jstring_length(src));

    if (*dest == NULL)
        return JXTA_NOMEM;

    startchunk = srcbuf;

    do {
        switch (*srcbuf) {
        case 0:{
                /* append any remaining part */
                jstring_append_0(*dest, startchunk, srcbuf - startchunk);
                return JXTA_SUCCESS;
            }

        case '<':{
                /* copy everything up to this char */
                jstring_append_0(*dest, startchunk, srcbuf - startchunk);
                startchunk = srcbuf + 1;

                /* append replacement */
                jstring_append_0(*dest, "&lt;", 4);
            }
            break;

            /* 
               FIXME bondolo@jxta.org 20020410 this replacement should not
               be necessary but was added for compatibility with the j2se
               implementation.
             */
        case '>':{
                /* copy everything up to this char */
                jstring_append_0(*dest, startchunk, srcbuf - startchunk);
                startchunk = srcbuf + 1;

                /* append replacement */
                jstring_append_0(*dest, "&gt;", 4);
            }
            break;

        case '&':{
                /* copy everything up to this char */
                jstring_append_0(*dest, startchunk, srcbuf - startchunk);
                startchunk = srcbuf + 1;

                /* append replacement */
                jstring_append_0(*dest, "&amp;", 5);
            }
            break;

        default:
            /* do nothing. */
            break;
        }

        srcbuf++;
    }
    while (1);
}


#ifdef GUNZIP_ENABLED
static int zip_uncompress(Byte * dest, uLong * destLen, Byte * source, uLong sourceLen)
{
    z_stream stream;
    int err;
    stream.next_in = (Bytef *) source;
    stream.avail_in = sourceLen;
    stream.next_out = dest;
    stream.avail_out = *destLen;
    stream.zalloc = 0;
    stream.zfree = 0;

    err = inflateInit2(&stream, MAX_WBITS + 16);
    if (err != Z_OK)
        return err;

    err = inflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "error zlib %s avail_in:%i avail_out:%i\n", stream.msg,
                        stream.avail_in, stream.avail_out);
        inflateEnd(&stream);
        return err;
    }
    *destLen = stream.total_out;
    err = inflateEnd(&stream);
    return err;
}

JXTA_DECLARE(Jxta_status) jxta_xml_util_uncompress(unsigned char *bytes, int size, unsigned char **uncompr, unsigned long *uncomprLen)
{
    Jxta_status res=JXTA_SUCCESS;
    int err;

    *uncomprLen = 40 * size;
    *uncompr = calloc(1, *uncomprLen);
    if (uncompr == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }
    err = zip_uncompress((Byte *) (*uncompr), uncomprLen, (Byte *)bytes, size);
    if (err != Z_OK) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error %d from zlib\n", err);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Received %d bytes, uncompress to %d bytes\n", size, uncomprLen);
    }

FINAL_EXIT:

    return res;
}

#endif /* GUNZIP_ENABLED */

#ifdef GZIP_ENABLED
static int zip_compress(unsigned char **out, size_t * out_len, const char *in, size_t in_len)
{
    struct z_stream_s *stream = NULL;
    size_t out_size;
    off_t offset;
    int err;
    *out = NULL;
    stream = calloc(1, sizeof(struct z_stream_s));
    if (stream == NULL) {
        return -1;
    }
    stream->zalloc = Z_NULL;
    stream->zfree = Z_NULL;
    stream->opaque = NULL;
    stream->next_in = (unsigned char *) in;
    stream->avail_in = in_len;

    out_size = in_len / 2;
    if (out_size < 1024)
        out_size = 1024;
    *out = calloc(1, out_size);
    if (out == NULL) {
        free(stream);
        return -1;
    }
    stream->next_out = *out;
    stream->avail_out = out_size;
    err = deflateInit2(stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY);
    if (err != Z_OK) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error from deflateInit2: %d  %s version: %s\n", err,
                        stream->msg ? stream->msg : "<no message>", ZLIB_VERSION);
        goto errExit;
    }

    while (1) {
        switch (deflate(stream, Z_FINISH)) {
        case Z_STREAM_END:
            goto done;
        case Z_OK:
            if (stream->avail_out >= stream->avail_in + 16) {
                break;
            } else {
            }
        case Z_BUF_ERROR:
            offset = stream->next_out - ((unsigned char *) *out);
            out_size *= 2;
            *out = realloc(*out, out_size);
            stream->next_out = *out + offset;
            stream->avail_out = out_size - offset;
            break;
        default:
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Gzip compression didn't finish: %s",
                            stream->msg ? stream->msg : "<no message>");
            goto errExit;
        }
    }
  done:
    *out_len = stream->total_out;
    if (deflateEnd(stream) != Z_OK) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error freeing gzip structures");
        goto errExit;
    }
    free(stream);

    return 0;
  errExit:
    if (stream) {
        deflateEnd(stream);
        free(stream);
    }
    if (*out) {
        free(*out);
    }
    return -1;
}

JXTA_DECLARE(Jxta_status) jxta_xml_util_compress(unsigned char *tmp, JString *el_name, Jxta_message_element **msgElem)
{
    Jxta_status res=JXTA_SUCCESS;
    unsigned char *zipped = NULL;
    size_t zipped_len = 0;
    int ret = 0;
    unsigned char *bytes=NULL;
    Jxta_bytevector * jSend_buf=NULL;

    ret = zip_compress(&zipped, &zipped_len, (const char *) tmp, (size_t) strlen((char *) tmp));

    if (Z_OK == ret) {
        size_t byte_len = 0;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "length:%d zipped_len:%d \n", strlen((char *) tmp), zipped_len);
        jSend_buf = jxta_bytevector_new_2(zipped, zipped_len, zipped_len);
        if (jSend_buf == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
            res = JXTA_NOMEM;
            goto FINAL_EXIT;
        }
        byte_len = jxta_bytevector_size(jSend_buf);
        bytes = calloc(1, byte_len);
        if (NULL == bytes) {
            res = JXTA_NOMEM;
            goto FINAL_EXIT;
        }
        jxta_bytevector_get_bytes_at(jSend_buf, bytes, 0, byte_len);
        *msgElem =
            jxta_message_element_new_1(jstring_get_string(el_name), "application/gzip", (char *) bytes,
                                       jxta_bytevector_size(jSend_buf), NULL);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "GZip comression error %d \n", ret);
        *msgElem = jxta_message_element_new_1(jstring_get_string(el_name), "text/xml", (char *) tmp, strlen((char *) tmp), NULL);
    }

FINAL_EXIT:
    if (jSend_buf)
        JXTA_OBJECT_RELEASE(jSend_buf);
    if (bytes)
        free(bytes);
    return res;
}

#endif /* GZIP_ENABLED */


/* vim: set ts=4 sw=4 et tw=130: */
