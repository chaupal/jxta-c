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
 * $Id: jxta_transport_http_client.c,v 1.23 2005/08/18 19:01:51 slowhog Exp $
 */

static const char *__log_cat = "HTTP_CLIENT";

#include <stdio.h>

#include "jpr/jpr_apr_wrapper.h"
#include "jxta_debug.h"
#include "jxta_errno.h"
#include "jxta_string.h"
#include "jxta_transport_http_client.h"
#include "jxtaapr.h"

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
struct _HttpClient {
    const char *proxy_host;
    apr_port_t proxy_port;
    const char *host;
    apr_port_t port;
    const char *useInterface;

    apr_socket_t *socket;
    apr_sockaddr_t *sockaddr;
    apr_sockaddr_t *intfaddr;

    int connected;
    int keep_alive;
    int timeout;
    apr_pool_t *pool;
};

struct _HttpRequest {
    HttpClient *con;
};

struct _HttpResponse {
    HttpClient *con;
    char *protocol;
    int status;
    char *status_msg;
    char *headers;
    int content_length;

    char *data_buf;
    apr_ssize_t data_buf_size;
};

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(HttpClient *) http_client_new(const char *proxy_host, Jxta_port proxy_port, const char *host, Jxta_port port,
                                           char *useInterface)
{
    apr_status_t status;

    HttpClient *con = (HttpClient *) calloc(1, sizeof(HttpClient));

    if (NULL == con)
        return NULL;

    apr_pool_create(&con->pool, 0);
    con->proxy_host = proxy_host ? strdup(proxy_host) : NULL;
    con->proxy_port = proxy_port;
    con->host = strdup(host);
    con->port = (apr_port_t) port;
    con->useInterface = useInterface ? strdup(useInterface) : strdup(APR_ANYADDR);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Create new client %s:%d\n", con->host, con->port);

    if (con->proxy_host)
        status = apr_sockaddr_info_get(&con->sockaddr, con->proxy_host, APR_INET, con->proxy_port, 0, con->pool);
    else {
        status = apr_sockaddr_info_get(&con->sockaddr, con->host, APR_INET, con->port, 0, con->pool);
    }
    if (APR_SUCCESS != status)
        goto Error_Exit;

    status = apr_sockaddr_info_get(&con->intfaddr, con->useInterface, APR_INET, 0, 0, con->pool);

    if (APR_SUCCESS != status)
        goto Error_Exit;


    goto Common_Exit;

  Error_Exit:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error creating new client connection %d \n", status);
    if (NULL != con)
        http_client_free(con);
    con = NULL;

  Common_Exit:

    return con;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_status) http_client_connect(HttpClient * con)
{
    apr_status_t status;
    apr_sockaddr_t *sa;
    char *addr;

    if (con == NULL)
        return JXTA_FAILED;

    if (con->proxy_host != NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "connect to %s:%d via %s:%d on intf %s\n", con->host, con->port,
                        con->proxy_host, con->proxy_port, con->useInterface);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "connect to %s:%d on intf %s\n", con->host, con->port,
                        con->useInterface);
    }

#if CHECK_APR_VERSION(1, 0, 0)
    status = apr_socket_create(&con->socket, APR_INET, SOCK_STREAM, APR_PROTO_TCP, con->pool);
#else
    status = apr_socket_create_ex(&con->socket, APR_INET, SOCK_STREAM, APR_PROTO_TCP, con->pool);
#endif

    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "socket create failed : %s\n", msg);
        goto Common_Exit;
    }

    status = apr_socket_bind(con->socket, con->intfaddr);

    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "socket bind failed : %s\n", msg);
        goto Common_Exit;
    }

    status = apr_socket_connect(con->socket, con->sockaddr);

    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "socket connect failed : %s\n", msg);
        http_client_close(con);
    } else {
        apr_socket_addr_get(&sa, APR_LOCAL, con->socket);
        apr_sockaddr_ip_get(&addr, sa);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "connected to %s:%d on intf %s:%u\n", con->host, con->port,
                        addr, sa->port);
    }

  Common_Exit:
    return APR_SUCCESS == status ? JXTA_SUCCESS : JXTA_FAILED;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(void) http_client_close(HttpClient * con)
{
    if (con->socket) {
        apr_socket_shutdown(con->socket, APR_SHUTDOWN_READWRITE);
        apr_socket_close(con->socket);
        con->socket = NULL;
    }
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void http_client_free(HttpClient * con)
{
    http_client_close(con);

    if (con->proxy_host) {
        free((char *) con->proxy_host);
        con->proxy_host = NULL;
    }

    if (con->host) {
        free((char *) con->host);
        con->host = NULL;
    }

    if (con->useInterface) {
        free((char *) con->useInterface);
        con->useInterface = NULL;
    }

    apr_pool_destroy(con->pool);
    free((void *) con);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void http_request_free(void *addr)
{
    HttpRequest *self = (HttpRequest *) addr;
    free(self);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(HttpRequest *) http_client_start_request(HttpClient * con, const char *method, const char *uri, const char *body)
{
    Jxta_status res;
    HttpRequest *req = (HttpRequest *) calloc(1, sizeof(HttpRequest));
    char s[1024];               /* Should be large enough */

    if (NULL == req) {
        return NULL;
    }

    if (con->proxy_host)
        apr_snprintf(s, sizeof(s), "%s http://%s:%d%s HTTP/1.0\r\n", method, con->host, con->port, uri);
    else
        apr_snprintf(s, sizeof(s), "%s %s HTTP/1.0\r\n", method, uri);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "request: %s\n", s);

    req->con = con;

    res = http_request_write(req, s, strlen(s));
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Connection failure need to re-open connection\n");
        free(req);
        return NULL;
    }

    if (body) {
        apr_snprintf(s, sizeof(s), "%d", strlen(body));
        http_request_set_header(req, "Content-Length", s);
    }

    apr_snprintf(s, sizeof(s), "%s:%d", con->host, con->port);
    http_request_set_header(req, "Host", s);
    http_request_set_header(req, "Connection", "Keep-Alive");

    if (body) {
        http_request_write(req, "\r\n", 2);
        http_request_write(req, body, strlen(body));
    }

    return req;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_status) http_request_set_header(HttpRequest * req, const char *name, const char *value)
{
    char *s = (char *) malloc(1024);    /* Should be large enough */
    apr_status_t res;

    apr_snprintf(s, 1024, "%s: %s\r\n", name, value);
    res = http_request_write(req, s, strlen(s));
    free(s);
    return APR_SUCCESS == res ? JXTA_SUCCESS : JXTA_FAILED;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_status) http_request_write(HttpRequest * req, const char *buf, size_t size)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t written = size;

    while (size > 0) {
        status = apr_socket_send(req->con->socket, buf, &written);
        if (written > 0) {
            buf += written;
            size -= written;
            written = size;
        } else {
            break;
        }
    }

    return APR_SUCCESS == status ? JXTA_SUCCESS : JXTA_FAILED;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void http_response_parse(HttpResponse * res, char *line)
{
    char *state;
    char *pt;

    res->protocol = apr_strtok(line, " ", &state);

    if (res->protocol != NULL) {
        pt = (char *) malloc(strlen(res->protocol) + 1);
        strcpy(pt, res->protocol);
        res->protocol = pt;
        *(state - 1) = ' ';

        pt = apr_strtok(NULL, " ", &state);

        if (pt != NULL) {
            res->status = atoi(pt);
            *(state - 1) = ' ';
        } else {
            return;
        }

        res->status_msg = apr_strtok(NULL, " ", &state);
        if (res->status_msg != NULL) {
            pt = (char *) malloc(strlen(res->status_msg) + 1);
            strcpy(pt, res->status_msg);
            res->status_msg = pt;
            *(state - 1) = ' ';
        }
    }
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void http_response_free(void *addr)
{
    HttpResponse *self = (HttpResponse *) addr;

    if (self->headers != NULL) {
        free(self->headers);
        self->headers = NULL;
    }
    if (self->protocol != NULL) {
        free(self->protocol);
        self->protocol = NULL;
    }
    if (self->status_msg != NULL) {
        free(self->status_msg);
        self->status_msg = NULL;
    }
    free(self);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(HttpResponse *) http_request_done(HttpRequest * req)
{

    HttpResponse *res = (HttpResponse *) malloc(sizeof(HttpResponse));
    apr_ssize_t header_buf_size = 8192;
    char *header_buf = (char *) malloc(header_buf_size * sizeof(char) + 1);
    apr_size_t read = header_buf_size;
    apr_size_t total_read = 0;
    apr_size_t i, pos = 0;
    int past_header = 0;
    int count_cr = 0;
    int count_lf = 0;
    char *pt;

    memset(res, 0, sizeof(HttpResponse));

    /* apr_status_t status; */
    res->protocol = NULL;
    res->status_msg = NULL;
    res->headers = NULL;
    res->con = req->con;

    pt = header_buf;

    while (!past_header) {

        if (apr_socket_recv(req->con->socket, header_buf + total_read, &read) == 0) {
            header_buf_size -= read;
            total_read += read;

            read = header_buf_size - 1; /* amount to read next.  leve room for null */

            while (!past_header && pos++ < total_read) {
                switch (header_buf[pos]) {
                case '\n':
                    count_lf++;
                    break;
                case '\r':
                    count_cr++;
                    break;
                case ' ':
                    break;
                case '\t':
                    break;
                default:
                    count_lf = 0;
                    count_cr = 0;
                    break;
                }
                past_header = ((count_cr == 2 && count_lf == 0) ||
                               (count_cr == 0 && count_lf == 2) || (count_cr == 2 && count_lf == 2));
                if (past_header) {
                    /**
                     ** We need to check if there is only one HTTP1/1 status
                     ** in the reply.
                     **/
                    http_response_parse(res, pt);
                    if (res->status == 100) {
                        /* Another header is following */
                        pt = &header_buf[pos + 1];
                        past_header = FALSE;
                    } else {
                        header_buf[pos] = 0;
                    }
                }
            }
        } else {
            /* end-of-stream in header */
            res->status = HTTP_NOT_CONNECTED;
            break;
        }
    }


    if (past_header) {
        for (i = 0; i < pos; i++) {
            header_buf[i] = tolower(header_buf[i]);
        }

        res->headers = header_buf;

        {
            char *content_length_header = http_response_get_header(res, "content-length");

            if (content_length_header == NULL) {
                res->content_length = -1;
            } else {
                res->content_length = atoi(content_length_header);
                free(content_length_header);
            }
        }

        if (res->content_length > 0) {
            ++pos;
            /* Just in case. Strip off extra <crlf> */
            while ((header_buf[pos] == '\n') || (header_buf[pos] == '\r'))
                ++pos;

            res->data_buf = &header_buf[pos];
            res->data_buf_size = total_read - pos;
            res->content_length -= res->data_buf_size;
        } else {
            res->data_buf = NULL;
            res->data_buf_size = 0;
        }
    } else {

        free(header_buf);
        res->headers = NULL;
    }

    return res;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(int) http_get_content_length(HttpResponse * res)
{
    return (res->content_length + res->data_buf_size);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(char *) http_response_get_header(HttpResponse * res, const char *name)
{
    char *line = strstr(res->headers, name);
    char *c;

    if (line != NULL) {

        /* skip "header_name: " */
        c = line + strlen(name);

        if (*c++ == ':' && *c++ == ' ') {

            while (*c++) {
                if (*c == '\r' || *c == '\n') {

                    /* possible continuation */
                    if (*(c + 1) == ' ' || *(c + 1) == '\t') {
                        *c = ' ';
                        continue;
                    } else {
                        /* if not */
                        apr_ssize_t value_size = c - line;
                        char *value = (char *) malloc(value_size + 1);

                        strncpy(value, line + strlen(name) + 2, value_size);

                        return value;
                    }
                }
            }
        }
    }

    return NULL;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(int) http_response_get_status(HttpResponse * res)
{
    return res->status;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(char *) http_response_get_status_message(HttpResponse * res)
{
    return res->status_msg;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_status) http_response_read(HttpResponse * res, char *buf, size_t * size)
{
    apr_status_t status = APR_SUCCESS;

    /* do we have anything buffered from when we were reading headers? */
    if (res->data_buf_size > 0) {
        apr_size_t read;

        read = (*size > res->data_buf_size ? res->data_buf_size : *size);
        memcpy(buf, res->data_buf, read);

        *size = read;

        res->data_buf += read;
        res->data_buf_size -= read;

        /* return right away, even if we are returning less than expected */

    } else {

        /* if we have nothing in the buffer, read from the network */
        apr_size_t read;

        /* if server didn't provide content-length, just read till close */
        if (res->content_length == -1)
            read = *size;
        else    /* or minimum of what was requested or content-length */
            read = (*size > res->content_length ? res->content_length : *size);

        status = apr_socket_recv(res->con->socket, buf, &read);

        if (read > 0) {

            *size = read;

            if (res->content_length != -1)
                res->content_length -= read;
        }
    }

    return APR_SUCCESS == status ? JXTA_SUCCESS : JXTA_FAILED;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_status) http_response_read_n(HttpResponse * res, char *buf, size_t n)
{
    apr_size_t bytesread;
    int offset;
    apr_status_t status;

    for (bytesread = n, offset = 0; n > 0;) {
        status = http_response_read(res, buf + offset, &bytesread);
        if (APR_SUCCESS != status) {
            return APR_SUCCESS == status ? JXTA_SUCCESS : JXTA_FAILED;
        }
        offset += bytesread;
        n -= bytesread;
        bytesread = n;
    }
    return JXTA_SUCCESS;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(char *) http_response_read_fully(HttpResponse * res)
{
    apr_size_t buf_size;
    char *buf = NULL;
    int read = 0;
    apr_size_t remain;

    if (res->status == HTTP_NOT_CONNECTED)
        return NULL;

    if (res->content_length >= 0) {
        buf_size = res->data_buf_size + res->content_length;
    } else {
        /* We don't know how much will be needed. Make a guess and hopes that works */
        /* This situation should not happen anyway */
        buf_size = res->data_buf_size + 8192;
    }

    buf = (char *) malloc(buf_size + 1);

    remain = buf_size;

    while ((remain > 0) && APR_SUCCESS == http_response_read(res, buf + read, &remain)) {
        read += remain;
        remain = buf_size - read;
    }
    buf[read] = 0;
    return buf;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void http_buffer_display(char *msgbuf, size_t size)
{
    unsigned int i;

    for (i = 0; i < size; ++i) {
        if ((msgbuf[i] < 32) && (msgbuf[i] != '\n') && (msgbuf[i] != '\r') && (msgbuf[i] != '\t')) {
            char buf[10];
            apr_snprintf(buf, sizeof(buf),"\\%d", msgbuf[i]);
            fwrite(buf, strlen(buf), 1, stderr);
            fwrite(" ", 1, 1, stderr);
        } else {
            fwrite(&msgbuf[i], 1, 1, stderr);
        }
    }
}

/* vim: set ts=4 sw=4 tw=130 et: */
