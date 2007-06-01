/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_transport_http_client.h,v 1.10 2006/02/15 01:09:49 slowhog Exp $
 */

#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__


#include <ctype.h>  /* tolower */
#include <string.h> /* for strstr */
#include <stdlib.h> /* for atoi */

#include <apr_uri.h>
#include "jxta_apr.h"

#include "jxta_types.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _HttpClient HttpClient;
typedef struct _HttpRequest HttpRequest;
typedef struct _HttpResponse HttpResponse;

/**
 * Possible value for HttpResponse->status
 */
#define HTTP_NOT_CONNECTED -1

/**
 * Create an http client object for the specified host and port number.
 * @param host         The host to connect to
 * @param port         The port to connect to
*/
JXTA_DECLARE(HttpClient *) http_client_new(const char *proxy_host,
                                           Jxta_port proxy_port, const char *host, Jxta_port port, char *useInterface);

/**
 * Attempt to establish a connection.
 * @return JXTA_SUCCESS if a connection was established.
 */
JXTA_DECLARE(Jxta_status) http_client_connect(HttpClient * con);

/**
 * Close the underlying socket connection.
 */
JXTA_DECLARE(void) http_client_close(HttpClient * con);

/**
 * Free the memory used by the client.
 */
void http_client_free(HttpClient * con);


/**
 * Send an HTTP request over an established connection.
 * @param  method One of GET, POST, PUT, DELETE
 * @param  uri    The URI to fetch
 * @param  body   The body of the http request.  Null if no body is to be sent
 *                 or if you want to set http headers before writing a body.
 * @return An object representing the http request.
 */
JXTA_DECLARE(HttpRequest *) http_client_start_request(HttpClient * con, const char *method, const char *uri, const char *body);

/**
 * Send an http request header.  When called after http_request_write,
 * this function will corrupt the current http request.
 * @param request The current http request.
 * @param name    The name of the http header.
 * @param value   The http header's value.
 * @param JXTA_SUCCESS if the header was set.
 */
JXTA_DECLARE(Jxta_status) http_request_set_header(HttpRequest * req, const char *name, const char *value);
/**
 * Write some of the http request's body.
 * @param request The current http request.
 * @param buf     The buffer containing the characters of the request body.
 * @param size    The number of characters of the buffer to write.
 * @param JXTA_SUCCESS if the header was sent.
 */
JXTA_DECLARE(Jxta_status) http_request_write(HttpRequest * req, const char *buf, size_t size);
/**
 * Finish writing the current http request and obtain a reference
 * to an http response.
 * @param request The current http request.
 * @return An http response object.
 */
JXTA_DECLARE(HttpResponse *) http_request_done(HttpRequest * req);

/**
 * Get the status code with which the http server responded.
 * @param res The http response we received.
 * @return The response status code.
 */
JXTA_DECLARE(int) http_response_get_status(HttpResponse * res);

/**
 * Get the status message with which the http server responded.
 * @param res The http response we received.
 * @return The response status message.
 */
JXTA_DECLARE(char *) http_response_get_status_message(HttpResponse * res);

/**
 * Get the value of a particular http response header.
 * @param res  The http response we received.
 * @param name The name of a response header.
 * @return The value of the response header.
 */
JXTA_DECLARE(char *) http_response_get_header(HttpResponse * res, const char *name);

/**
 * Read some bytes from the http response body.
 * @param res  The http response we received.
 * @param buf  The character buffer into which to read the response.
 * @param size The size of the buffer.  The number of bytes actually
 *             read will be placed into this pointer argument. 
 * @return JXTA_SUCCESS if some number of
 *         bytes have been read.
 */
JXTA_DECLARE(Jxta_status) http_response_read(HttpResponse * res, char *buf, size_t * size);

/**
 * Read an exact number of bytes from the http response body.
 * @param res The http response we received.
 * @param buf The character buffer into which to read the response.
 * @param n   The number of bytes you would like to read.
 * @return JXTA_SUCCESS if the specified
 *         number of bytes has been read.
 */
JXTA_DECLARE(Jxta_status) http_response_read_n(HttpResponse * res, char *buf, size_t n);

/**
 * Read all the data in an http response.
 * @param res  The http response we received.
 * @return The character buffer containing the body of the response.
 */
JXTA_DECLARE(char *) http_response_read_fully(HttpResponse * res);



/**
 * Prepare to send another http request over the same http connection.
 * @param res The current http response.
 */
JXTA_DECLARE(void) http_response_done(HttpResponse * res);

void http_response_free(void *addr);
void http_request_free(void *addr);
JXTA_DECLARE(int) http_get_content_length(HttpResponse * res);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif


#endif
