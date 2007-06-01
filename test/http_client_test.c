
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
 * $Id: http_client_test.c,v 1.12 2005/09/23 20:07:13 slowhog Exp $
 */


#include "jxta.h"
#include "../src/jxta_transport_http_client.h"

int main(int argc, char **argv)
{

    apr_uri_t uri;
    apr_pool_t *pool;
    HttpClient *con;
    HttpRequest *req;
    HttpResponse *res;
    /* char *content_length_header; */
    /* apr_ssize_t content_length; */
    char buf[8192];
    apr_ssize_t buf_size = 8192;
    int i;

    jxta_initialize();

    if (argc < 2) {
        printf("Usage: ./http_test <uri>\n");
        printf("HTTP example: ./http_client http://127.0.0.1:80/~user/index.html\n");
        printf("HTTP example: ./http_client http://127.0.0.1:80/\n");
        exit(0);
    }

    apr_pool_create(&pool, NULL);

    apr_uri_parse(pool, argv[1], &uri);

    con = http_client_new(NULL, 0, uri.hostname, uri.port, NULL);


    if (APR_SUCCESS == http_client_connect(con)) {

        for (i = 0; i < 5; i++) {

            req = http_client_start_request(con, "GET", uri.path, NULL);

            res = http_request_done(req);

/*      printf ("Response status: %s %d %s\n", 
	      res->protocol, 
	      res->status,
	      res->status_msg);
*/
            /*
               if (res->status == HTTP_NOT_CONNECTED) {
             */
            if (http_response_get_status(res) == HTTP_NOT_CONNECTED) {
                http_client_close(con);
                if (APR_SUCCESS == http_client_connect(con)) {
                    printf("reconnected\n");
                    continue;
                } else {
                    printf("couldn't reconnect\n");
                    break;
                }
            } else {

/*	printf ("Response Headers: %s\n", res->headers);
       	printf ("Response Content-Length: %d\n", res->content_length);
*/
                while (APR_SUCCESS == http_response_read(res, buf, &buf_size)) {

                    printf("Response read %d bytes\n", buf_size);

                    buf[buf_size] = '\0';

                    printf("%s\n", buf);

                    buf_size = 8192;
                }
            }

            apr_sleep(10);
        }
    } else {
        printf("Could not connect.");

    }

    jxta_terminate();
    return 0;
}
