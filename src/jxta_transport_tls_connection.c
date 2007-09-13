/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights
 * reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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

static const char *__log_cat = "TLS_CONNECTION";

#include "jxta_endpoint_service.h"
#include "jxta_object_type.h"
#include "jxta_peergroup.h"

#include "jxta_transport_tls_private.h"
#include <openssl/ssl.h>

#define TLS_MIMETYPE "application/x-jxta-tls-block"
#define TLS_ACK "application/x-jxta-tls-ack"
#define APP_MSG "application/x-jxta-msg"

#define TLS_RESEND_DELAY 1000000
#define TLS_TIMEOUT 10000000

#define MAX_RESENTS 10

#define MESSAGE_BUFFER_SIZE 100

typedef struct _jxta_tls_connection_thread {
    Jxta_boolean running;
    Jxta_boolean abort;
    void *data;
    apr_thread_mutex_t *mutex;
} _jxta_tls_connection_thread;

/* TLS_connection-structure */
struct _jxta_transport_tls_connection {
    Extends(Jxta_object);

    apr_pool_t *pool;
    apr_thread_mutex_t *global_mutex;

    Jxta_endpoint_address *dest_addr;

    Jxta_endpoint_service *endpoint;
    Jxta_PG *group;

    int out_seq_number;
    unsigned int in_seq_number;

    Jxta_hashtable *input_table;

    apr_queue_t *message_buffer;

    _jxta_tls_connection_thread retry_thread;

    _jxta_tls_connection_thread receive_thread;

    Jxta_transport_tls_connections *connections;

    apr_time_t last_feedback;

    apr_thread_cond_t *handshake_done;

    Jxta_boolean is_connected;

/* openSSL-stuff */
    char *server_cert;
    SSL *ssl;

    BIO *ssl_bio;
    BIO *internal_bio;
    BIO *transport_bio;
};

struct _jxta_transport_tls_connections {
    Extends(Jxta_object);

    Jxta_hashtable *connection_table;
    Jxta_endpoint_service *endpoint;
    Jxta_PG *group;
};

/* TODO: fix naming: self -> me */
static void tls_connections_construct(Jxta_transport_tls_connections * self);
static void tls_connections_free(Jxta_object * me);

static void tls_connection_free(Jxta_object * me);

static void tls_connection_decrypt_demux(Jxta_transport_tls_connection * self, _tls_connection_buffer * buffer);
static void tls_connection_send_ack(Jxta_transport_tls_connection * self);
static void tls_connection_send_ciphertext(Jxta_transport_tls_connection * self, _tls_connection_buffer * data);

static void tls_connection_async_receive(Jxta_transport_tls_connection * self, Jxta_object * data);
static Jxta_boolean tls_connection_receive_next_item(Jxta_transport_tls_connection * self, Jxta_object ** data);

static void tls_connection_async_send(Jxta_transport_tls_connection * self, Jxta_object * data);
static Jxta_boolean tls_connection_send_next_item(Jxta_transport_tls_connection * self, Jxta_object ** data);

static void tls_connection_start_receive_thread(Jxta_transport_tls_connection * me);

void tls_connection_log_buffer(char *buffer, int size);

/* ********************************************************* */
/* ********* *  TLS Connections  * ************************* */
/* ********************************************************* */

/* public methods */

Jxta_transport_tls_connections *tls_connections_new(Jxta_PG * group, Jxta_endpoint_service * endpoint)
{
    Jxta_transport_tls_connections *self;

    /* create object */
    self = (Jxta_transport_tls_connections *) malloc(sizeof(Jxta_transport_tls_connections));
    memset(self, 0, sizeof(Jxta_transport_tls_connections));

    JXTA_OBJECT_INIT(self, tls_connections_free, NULL);
    tls_connections_construct(self);

    /* initialize it */
    self->connection_table = jxta_hashtable_new(0);
    if (self->connection_table == NULL) {
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    self->endpoint = JXTA_OBJECT_SHARE(endpoint);
    self->group = JXTA_OBJECT_SHARE(group);

    return self;
}

Jxta_transport_tls_connection *tls_connections_get_connection(Jxta_transport_tls_connections * self, const char *dest_address)
{
    Jxta_transport_tls_connection *connection = NULL;
    PTValid(self, Jxta_transport_tls_connections);

    jxta_hashtable_get(self->connection_table, dest_address, strlen(dest_address), (Jxta_object **) & connection);

    return connection;
}

/* private methods */

static void tls_connections_construct(Jxta_transport_tls_connections * self)
{
    self->thisType = "Jxta_transport_tls_connections";

    self->connection_table = NULL;
    self->endpoint = NULL;
    self->group = NULL;
}

static void tls_connections_free(Jxta_object * connections)
{
    Jxta_transport_tls_connections *self = (Jxta_transport_tls_connections *) connections;

    JXTA_OBJECT_RELEASE(self->endpoint);
    JXTA_OBJECT_RELEASE(self->group);

    jxta_hashtable_clear(self->connection_table);

    JXTA_OBJECT_RELEASE(self->connection_table);

    free((void *) self);
}

void tls_connections_add_connection(Jxta_transport_tls_connections * self, Jxta_transport_tls_connection * connection)
{
    const char *dest_str = NULL;

    PTValid(self, Jxta_transport_tls_connections);

    dest_str = jxta_endpoint_address_get_protocol_address(connection->dest_addr);

    jxta_hashtable_put(self->connection_table, dest_str, strlen(dest_str), (void *) connection);
}

void tls_connections_remove_connection(Jxta_transport_tls_connections * self, const char *dest_addr)
{
    PTValid(self, Jxta_transport_tls_connections);

    jxta_hashtable_del(self->connection_table, dest_addr, strlen(dest_addr), NULL);
}

/* **********************************************************/
/* ********* *  TLS Connection  * ***************************/
/* **********************************************************/

Jxta_transport_tls_connection *tls_connection_new(Jxta_PG * group, Jxta_endpoint_service * endpoint, const char *dest_address,
                                                  Jxta_transport_tls_connections * connections, SSL_CTX * ctx)
{
    Jxta_transport_tls_connection *self;
    apr_status_t status;

    /* create object */
    self = (Jxta_transport_tls_connection *) malloc(sizeof(Jxta_transport_tls_connection));
    memset(self, 0, sizeof(Jxta_transport_tls_connection));

    JXTA_OBJECT_INIT(self, tls_connection_free, NULL);

    self->thisType = "Jxta_transport_tls_connection";

    /* initialize it */

    /* Allocate a pool for our apr needs */
    status = apr_pool_create(&self->pool, jxta_PG_pool_get(group));
    if (status != APR_SUCCESS) {
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    /* Create our mutex. */
    status = apr_thread_mutex_create(&(self->global_mutex), APR_THREAD_MUTEX_NESTED, self->pool);
    if (status != APR_SUCCESS) {
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    /* setting */
    self->dest_addr = jxta_endpoint_address_new_2("jxta", dest_address, TLS_SERVICE_NAME, NULL);

    self->endpoint = JXTA_OBJECT_SHARE(endpoint);
    self->group = JXTA_OBJECT_SHARE(group);

    self->out_seq_number = 1;
    self->in_seq_number = 1;

    apr_queue_create(&self->message_buffer, MESSAGE_BUFFER_SIZE, self->pool);

    self->input_table = jxta_hashtable_new(0);

    self->is_connected = FALSE;


    self->connections = connections;

    /* retry-stuff */
    self->retry_thread.running = FALSE;
    self->retry_thread.abort = FALSE;
    apr_thread_mutex_create(&(self->retry_thread.mutex), APR_THREAD_MUTEX_NESTED, self->pool);

    {
        Jxta_message *message = jxta_message_new();

        Jxta_message_element *element =
            jxta_message_element_new_2(TLS_NAMESPACE, "MARKRetr", "text/plain;charset=\"UTF-8\"", "TLSRET", 7, NULL);
        jxta_message_add_element(message, element);
        JXTA_OBJECT_RELEASE(element);

        self->retry_thread.data = message;
    }

    self->receive_thread.abort = FALSE;
    self->receive_thread.running = FALSE;
    apr_thread_mutex_create(&(self->receive_thread.mutex), APR_THREAD_MUTEX_NESTED, self->pool);

    self->last_feedback = apr_time_now();

    apr_thread_cond_create(&self->handshake_done, self->pool);

    /* SSL-stuff */

    self->ssl = SSL_new(ctx);

    BIO_new_bio_pair(&self->internal_bio, 0, &self->transport_bio, 0);

    if ((self->internal_bio == NULL) || (self->transport_bio == NULL)) {
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    self->ssl_bio = BIO_new(BIO_f_ssl());
    if (self->ssl_bio == NULL) {
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    SSL_set_bio(self->ssl, self->internal_bio, self->internal_bio);
    BIO_set_ssl(self->ssl_bio, self->ssl, BIO_NOCLOSE);

    SSL_set_accept_state(self->ssl);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "SSL state: %s\n", SSL_state_string_long(self->ssl));

    return self;
}

static void tls_connection_free(Jxta_object * connection)
{
    Jxta_transport_tls_connection *self = PTValid(connection, Jxta_transport_tls_connection);

    apr_thread_mutex_lock(self->retry_thread.mutex);
    {
        self->receive_thread.abort = TRUE;
    }
    apr_thread_mutex_unlock(self->retry_thread.mutex);

    apr_thread_mutex_lock(self->receive_thread.mutex);
    {
        self->receive_thread.abort = TRUE;
    }
    apr_thread_mutex_unlock(self->receive_thread.mutex);

    apr_thread_pool_tasks_cancel(jxta_PG_thread_pool_get(self->group), self);

    JXTA_OBJECT_RELEASE(self->retry_thread.data);

    {
        Jxta_object *obj;

        while (apr_queue_trypop(self->message_buffer, (void **) &obj) != APR_EAGAIN) {
            JXTA_OBJECT_RELEASE(obj);
        }
    }

    apr_thread_mutex_destroy(self->retry_thread.mutex);

    JXTA_OBJECT_RELEASE(self->endpoint);
    JXTA_OBJECT_RELEASE(self->dest_addr);

    if (self->pool) {
        apr_thread_mutex_destroy(self->global_mutex);

        apr_thread_cond_destroy(self->handshake_done);

        apr_pool_destroy(self->pool);
    }

    JXTA_OBJECT_RELEASE(self->group);
    JXTA_OBJECT_RELEASE(self->input_table);

    SSL_free(self->ssl);

    free(self);
}

/* ********************************************************* */
/* ********* *  TLS connection thread-functions  * ********* */
/* ********************************************************* */

static void *APR_THREAD_FUNC tls_connection_disconnect_thread(apr_thread_t * thread, void *param)
{
    Jxta_transport_tls_connection *myself = PTValid(param, Jxta_transport_tls_connection);

    JXTA_OBJECT_RELEASE(myself);

    return NULL;
}

static void tls_connection_disconnect(Jxta_transport_tls_connection * me)
{
    Jxta_transport_tls_connection *myself = PTValid(me, Jxta_transport_tls_connection);

    /* This thread is not allowed to call tls_connection_free because tls_connection might wait for this thread to end; */
    /* so we share this connection and start another, save thread, which will release this connection and maybe free it. */
    JXTA_OBJECT_SHARE(myself);

    {
        const char *dest_str = jxta_endpoint_address_get_protocol_address(myself->dest_addr);
        tls_connections_remove_connection(myself->connections, dest_str);
    }

    apr_thread_pool_push(jxta_PG_thread_pool_get(myself->group), tls_connection_disconnect_thread, myself,
                         APR_THREAD_TASK_PRIORITY_NORMAL, NULL);
}

/* this thread-pool-method resends unacknowledged messages */
static void *APR_THREAD_FUNC tls_retry_thread(apr_thread_t * thread, void *param)
{
    Jxta_transport_tls_connection *myself = PTValid(param, Jxta_transport_tls_connection);

    apr_thread_mutex_lock(myself->retry_thread.mutex);
    {
        if (myself->retry_thread.abort == FALSE) {
            int element_count = 0;

            Jxta_message *message = NULL;

            if (myself->last_feedback + TLS_TIMEOUT < apr_time_now()) {
                tls_connection_disconnect(myself);

                goto FINALLY;
            }

            if (myself->retry_thread.data != NULL) {
                Jxta_vector *elements = NULL;

                message = (Jxta_message *) myself->retry_thread.data;

                elements = jxta_message_get_elements_of_namespace(message, TLS_NAMESPACE);

                element_count = jxta_vector_size(elements);

                JXTA_OBJECT_RELEASE(elements);

                if (element_count <= 1) {
                    /* all sends were acknowledged */

                    goto FINALLY;
                }

                /* if none of the exit conditions above was true, send the retry-message */
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "retry elements: %i\n", element_count);

                jxta_endpoint_service_send(myself->group, myself->endpoint, message, myself->dest_addr);

                /* reschedule the thread for further resends */

                apr_thread_pool_schedule(jxta_PG_thread_pool_get(myself->group), tls_retry_thread, myself, TLS_RESEND_DELAY,
                                         myself);
            }
        }
    }
  FINALLY:
    myself->retry_thread.running = FALSE;
    apr_thread_mutex_unlock(myself->retry_thread.mutex);

    return NULL;
}

static void *APR_THREAD_FUNC tls_connection_receive_thread(apr_thread_t * thread, void *param)
{
    Jxta_transport_tls_connection *myself = PTValid(param, Jxta_transport_tls_connection);

    _tls_connection_buffer *buffer = NULL;

    apr_thread_mutex_lock(myself->receive_thread.mutex);
    {
        tls_connection_decrypt_demux(myself, myself->receive_thread.data);

        JXTA_OBJECT_RELEASE(myself->receive_thread.data);

        myself->receive_thread.running = FALSE;
    }
    apr_thread_mutex_unlock(myself->receive_thread.mutex);

    tls_connection_start_receive_thread(myself);

    return NULL;
}

static void tls_connection_start_receive_thread(Jxta_transport_tls_connection * me)
{
    Jxta_transport_tls_connection *myself = PTValid(me, Jxta_transport_tls_connection);

    apr_thread_mutex_lock(myself->receive_thread.mutex);
    {
        if ((myself->receive_thread.abort == FALSE) && (myself->receive_thread.running == FALSE)) {
            if (jxta_hashtable_del
                (myself->input_table, &(myself->in_seq_number), sizeof(int),
                 (Jxta_object **) & myself->receive_thread.data) != JXTA_ITEM_NOTFOUND) {
                myself->in_seq_number = myself->in_seq_number + 1;

                myself->receive_thread.running = TRUE;

                apr_thread_pool_push(jxta_PG_thread_pool_get(myself->group), tls_connection_receive_thread, myself,
                                     APR_THREAD_TASK_PRIORITY_NORMAL, myself);
            }
        }
    }
    apr_thread_mutex_unlock(myself->receive_thread.mutex);
}

/* ******************************************************* */
/* ******** *  TLS connection retry-functions  * ********* */
/* ******************************************************* */

/* retries in network-byte-order! */
static void tls_connection_remove_retries(Jxta_transport_tls_connection * me, int *retries, int size)
{
    Jxta_transport_tls_connection *myself = PTValid(me, Jxta_transport_tls_connection);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "removing ACKs\n");

    apr_thread_mutex_lock(myself->retry_thread.mutex);
    {
        /* remove explicit sequence-numbers */

        Jxta_message *message = (Jxta_message *) myself->retry_thread.data;

        {
            int i;

            for (i = 0; i < size; i++) {
                int s_size;
                char *s_seq;

                int ret;

                s_size = apr_snprintf(NULL, 0, "%d", ntohl(retries[i])) + 1;
                s_seq = malloc(s_size);
                apr_snprintf(s_seq, s_size, "%d", ntohl(retries[i]));

                ret = jxta_message_remove_element_2(message, TLS_NAMESPACE, s_seq);

                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "removed ACK %s\n", s_seq);

                free(s_seq);
            }
        }

        /* remove message-numbers smaller then retries[0] */
        {
            unsigned int i;
            Jxta_vector *elements = NULL;

            int ret;

            elements = jxta_message_get_elements_of_namespace(message, TLS_NAMESPACE);

            for (i = 0; i < jxta_vector_size(elements); i++) {
                Jxta_message_element *element = NULL;

                int element_index = 0;

                jxta_vector_get_object_at(elements, (Jxta_object **) & element, i);

                if (strcmp(jxta_message_element_get_name(element), "MARKRetr") == 0)
                    continue;

                element_index = atoi(jxta_message_element_get_name(element));

                if (element_index > 0) {
                    if ((unsigned int) element_index < ntohl(retries[0])) {
                        ret = jxta_message_remove_element(message, element);
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "removed ACK %s\n",
                                        jxta_message_element_get_name(element));
                    }
                }

                JXTA_OBJECT_RELEASE(element);
            }

            JXTA_OBJECT_RELEASE(elements);
        }
    }
    apr_thread_mutex_unlock(myself->retry_thread.mutex);
}

static void tls_connection_add_retry(Jxta_transport_tls_connection * me, Jxta_message_element * element)
{
    Jxta_transport_tls_connection *myself = PTValid(me, Jxta_transport_tls_connection);

    apr_thread_mutex_lock(myself->retry_thread.mutex);
    {
        Jxta_message *message = myself->retry_thread.data;

        int element_count = 0;
        {
            Jxta_vector *elements = jxta_message_get_elements(message);

            element_count = jxta_vector_size(elements);

            JXTA_OBJECT_RELEASE(elements);
        }

        /* if there was now tls-message-element, restart the thread */
        if ((element_count == 1) && (myself->retry_thread.running == FALSE)) {
            myself->retry_thread.running = TRUE;

            /* reset feedback-time since there was no communication for some time... */
            myself->last_feedback = apr_time_now();

            apr_thread_pool_schedule(jxta_PG_thread_pool_get(myself->group), tls_retry_thread, myself, TLS_RESEND_DELAY, myself);
        }

        jxta_message_add_element(message, element);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "added retry Nr.: %s\n", jxta_message_element_get_name(element));
    }
    apr_thread_mutex_unlock(myself->retry_thread.mutex);
}

/* ******************************************************* */
/* ********* *  TLS connection send-functions  * ********* */
/* ******************************************************* */

/* creates a TLS-message from the buffer sends and enqueues it */
static void tls_connection_send_ciphertext(Jxta_transport_tls_connection * me, _tls_connection_buffer * data)
{
    Jxta_transport_tls_connection *myself = PTValid(me, Jxta_transport_tls_connection);

    {
        Jxta_message *message = jxta_message_new();
        int seq_number;

        /* get actual sequence number and increase over-all sequence number safed by lock! */
        apr_thread_mutex_lock(myself->global_mutex);
        {
            seq_number = myself->out_seq_number;
            myself->out_seq_number++;
        }
        apr_thread_mutex_unlock(myself->global_mutex);

        /* create and send message */
        {
            int s_size = 0;
            char *s_seq = NULL;
            Jxta_message_element *element = NULL;

            s_size = apr_snprintf(NULL, 0, "%d", seq_number) + 1;
            s_seq = malloc(s_size);
            apr_snprintf(s_seq, s_size, "%d", seq_number);

            element = jxta_message_element_new_2(TLS_NAMESPACE, s_seq, TLS_MIMETYPE, data->bytes, data->size, NULL);
            jxta_message_add_element(message, element);

            tls_connection_add_retry(myself, element);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "send TLSMessage Nr.: %s\n", s_seq);
            jxta_endpoint_service_send(myself->group, myself->endpoint, message, myself->dest_addr);

            free(s_seq);
            JXTA_OBJECT_RELEASE(element);
        }

        JXTA_OBJECT_RELEASE(message);
    }
}

/* encrypt the message */
Jxta_status tls_connection_send(Jxta_transport_tls_connection * me, Jxta_message * message, apr_interval_time_t timeout)
{
    Jxta_transport_tls_connection *myself = PTValid(me, Jxta_transport_tls_connection);

    apr_thread_mutex_lock(myself->global_mutex);
    {
        if (myself->is_connected == FALSE) {
            if (apr_thread_cond_timedwait(myself->handshake_done, myself->global_mutex, timeout) == APR_TIMEUP)
                return JXTA_TIMEOUT;
        }
    }
    apr_thread_mutex_unlock(myself->global_mutex);

    /* push the plaintext into the ssl-object */
    {
        _tls_connection_buffer *plaintext = NULL;

        JString *msgString = jstring_new_0();

        jxta_message_to_jstring(message, NULL, msgString);

        plaintext = transport_tls_buffer_new_1(jstring_length(msgString));

        memcpy(plaintext->bytes, (char *) jstring_get_string(msgString), plaintext->size);

        BIO_write(myself->ssl_bio, plaintext->bytes, plaintext->size);
        BIO_flush(myself->ssl_bio);

        JXTA_OBJECT_RELEASE(msgString);
        JXTA_OBJECT_RELEASE(plaintext);
    }

    /* pop the encrypted values from the ssl-object */
    {
        int size = BIO_ctrl_pending(myself->transport_bio);

        if (size != 0) {
            _tls_connection_buffer *ciphertext = transport_tls_buffer_new_1(size);

            BIO_read(myself->transport_bio, ciphertext->bytes, ciphertext->size);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "encrypting; size: %i\n", ciphertext->size);
            tls_connection_log_buffer(ciphertext->bytes, ciphertext->size);

            tls_connection_send_ciphertext(myself, ciphertext);

            JXTA_OBJECT_RELEASE(ciphertext);
        }
    }

    return JXTA_SUCCESS;
}

/* ******************************************************* */
/* ********* *  TLS connection ack-functions  * ********** */
/* ******************************************************* */

static int tls_seq_buffer_compare(const void *p1, const void *p2)
{
    const int *obj1 = p1;
    const int *obj2 = p2;

    if (ntohl(*obj1) < ntohl(*obj2))
        return -1;

    if (ntohl(*obj1) == ntohl(*obj2))
        return 0;

    return 1;
}

/* create tls-ack-message and send */
static void tls_connection_send_ack(Jxta_transport_tls_connection * me)
{
    Jxta_transport_tls_connection *myself = PTValid(me, Jxta_transport_tls_connection);

    Jxta_vector *received_messages = jxta_hashtable_values_get(myself->input_table);

    if (jxta_vector_size(received_messages) > 0) {
        Jxta_message *message;
        Jxta_message_element *element;
        unsigned int i;
        unsigned int index = 1;

        unsigned int debug = jxta_vector_size(received_messages);

        _tls_connection_buffer *buffer;

        unsigned int *seq_array = malloc((jxta_vector_size(received_messages) + 1) * sizeof(int));

        for (i = 0; i < jxta_vector_size(received_messages); i++) {
            jxta_vector_get_object_at(received_messages, (Jxta_object **) & buffer, i);
            seq_array[i + 1] = htonl(buffer->seq_number);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "acknowleged TLSMessage: %i\n", buffer->seq_number);

            JXTA_OBJECT_RELEASE(buffer);
        }

        qsort(seq_array + 1, jxta_vector_size(received_messages), sizeof(int), tls_seq_buffer_compare);

        apr_thread_mutex_lock(myself->global_mutex);
        {
            if (ntohl(seq_array[1]) > myself->in_seq_number) {
                seq_array[0] = htonl(myself->in_seq_number - 1);
                index = 0;
            }
        }
        apr_thread_mutex_unlock(myself->global_mutex);

        /* TODO: first all computation in host-byteorder, then convert to network-bytorder? */
        for (i = index; i < (jxta_vector_size(received_messages)); i++) {
            if (ntohl(seq_array[i]) + 1 == ntohl(seq_array[i + 1]))
                index++;
            else
                break;
        }

        message = jxta_message_new();

        element = jxta_message_element_new_2(TLS_NAMESPACE, "TLSACK", TLS_ACK, (char *) &seq_array[index],
                                             (jxta_vector_size(received_messages) + 1 - index) * sizeof(int), NULL);

        jxta_message_add_element(message, element);

        jxta_endpoint_service_send(myself->group, myself->endpoint, message, myself->dest_addr);

        free(seq_array);

        JXTA_OBJECT_RELEASE(message);
        JXTA_OBJECT_RELEASE(element);
    } else {
        Jxta_message *message;
        Jxta_message_element *element;

        int nr = myself->in_seq_number - 1;

        message = jxta_message_new();

        apr_thread_mutex_lock(myself->global_mutex);
        {
            element = jxta_message_element_new_2(TLS_NAMESPACE, "TLSACK", TLS_ACK, (char *) &nr, sizeof(int), NULL);
        }
        apr_thread_mutex_unlock(myself->global_mutex);

        jxta_message_add_element(message, element);

        jxta_endpoint_service_send(myself->group, myself->endpoint, message, myself->dest_addr);

        JXTA_OBJECT_RELEASE(message);
        JXTA_OBJECT_RELEASE(element);
    }

    JXTA_OBJECT_RELEASE(received_messages);
}

/* ********************************************************** */
/* ********* *  TLS connection receive-functions  * ********* */
/* ********************************************************** */

/* helper function to read from a _tls_connection_buffer needed for jxta_message_read */
static Jxta_status JXTA_STDCALL read_from_tls_connection(void *me, char *buf, apr_size_t len)
{
    _tls_connection_buffer *buffer = (_tls_connection_buffer *) me;

    if (buffer->pos + len > buffer->size)
        return JXTA_FAILED;

    memcpy(buf, buffer->bytes + buffer->pos, len);

    buffer->pos += len;

    return JXTA_SUCCESS;
}

/* decrypt as message and demux via endpoint service */
static void tls_connection_decrypt_demux(Jxta_transport_tls_connection * self, _tls_connection_buffer * ciphertext)
{
    int state = SSL_state(self->ssl);
    int res = 0;

    PTValid(self, Jxta_transport_tls_connection);

    {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "decrypting; size: %i\n", ciphertext->size);
        tls_connection_log_buffer(ciphertext->bytes, ciphertext->size);

        res = BIO_write(self->transport_bio, ciphertext->bytes, ciphertext->size);
        res = BIO_flush(self->transport_bio);
    }

    {
        int size = BIO_ctrl_pending(self->ssl_bio);

        if (size > 0) {
            _tls_connection_buffer *plaintext = transport_tls_buffer_new_1(size);

            res = BIO_read(self->ssl_bio, plaintext->bytes, plaintext->size);

            if ((state == SSL_ST_OK) && (res == -1)) {
                tls_connection_disconnect(self);

                return;
            }

            if (state == SSL_ST_OK) {
                Jxta_message *tlsmsg = jxta_message_new();

                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "decrypted TLSMessage:\n");
                tls_connection_log_buffer(plaintext->bytes, plaintext->size);

                jxta_message_read(tlsmsg, APP_MSG, read_from_tls_connection, plaintext);

                jxta_endpoint_service_demux(self->endpoint, tlsmsg);

                JXTA_OBJECT_RELEASE(tlsmsg);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "received tls-internal\n");
            }


            JXTA_OBJECT_RELEASE(plaintext);
        }
    }

    {
        int size = BIO_ctrl_pending(self->transport_bio);

        if (size > 0) {
            _tls_connection_buffer *output_data = transport_tls_buffer_new_1(size);

            res = BIO_read(self->transport_bio, output_data->bytes, output_data->size);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "send tls-internal\n");
            tls_connection_log_buffer(output_data->bytes, output_data->size);

            tls_connection_send_ciphertext(self, output_data);

            JXTA_OBJECT_RELEASE(output_data);
        }
    }

    if ((state != SSL_ST_OK) && (SSL_state(self->ssl) == SSL_ST_OK)) {
        apr_thread_mutex_lock(self->global_mutex);
        {
            apr_thread_cond_signal(self->handshake_done);

            self->is_connected = TRUE;
        }
        apr_thread_mutex_unlock(self->global_mutex);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "SSL state: %s\n", SSL_state_string_long(self->ssl));
}

void tls_connection_receive(Jxta_transport_tls_connection * me, Jxta_message * msg)
{
    _tls_connection_buffer *data = NULL;
    unsigned int i = 0;

    Jxta_vector *elements = NULL;
    Jxta_message_element *el = NULL;

    Jxta_transport_tls_connection *myself = PTValid(me, Jxta_transport_tls_connection);


    elements = jxta_message_get_elements_of_namespace(msg, TLS_NAMESPACE);

    for (i = 0; i < jxta_vector_size(elements); i++) {

        jxta_vector_get_object_at(elements, JXTA_OBJECT_PPTR(&el), i);

        if (el != NULL) {
            char *el_name = (char *) jxta_message_element_get_name(el);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "received TLSMessage; name: %s\n", el_name);

            if (strcmp(el_name, "TLSACK") == 0) {
                unsigned int j = 0;
                Jxta_bytevector *bytes = jxta_message_element_get_value(el);

                tls_connection_remove_retries(myself, (int *) jxta_bytevector_content_ptr(bytes),
                                              jxta_bytevector_size(bytes) / sizeof(int));

                JXTA_OBJECT_RELEASE(bytes);

                myself->last_feedback = apr_time_now();

                continue;
            }

            if (strcmp(el_name, "MARKRetr") != 0) {
                Jxta_bytevector *bytes = jxta_message_element_get_value(el);

                data = transport_tls_buffer_new_1(jxta_bytevector_size(bytes));

                memcpy(data->bytes, jxta_bytevector_content_ptr(bytes), data->size);

                data->seq_number = atoi(el_name);

                JXTA_OBJECT_RELEASE(bytes);

                if (data != NULL) {
                    apr_thread_mutex_lock(myself->global_mutex);
                    {
                        if (data->seq_number >= myself->in_seq_number) {
                            if (!jxta_hashtable_putnoreplace
                                (myself->input_table, &data->seq_number, sizeof(int), JXTA_OBJECT(data))) {
                                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "received duplicate message %d; skipped...\n",
                                                data->seq_number);
                            }
                        } else {
                            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "wrong sequence number: %d; expected: %d",
                                            data->seq_number, myself->in_seq_number);
                        }
                    }
                    apr_thread_mutex_unlock(myself->global_mutex);

                    JXTA_OBJECT_RELEASE(data);
                }
            }
        }   /* if (el != NULL) */
    }   /* for */

    tls_connection_send_ack(myself);

    tls_connection_start_receive_thread(myself);

    JXTA_OBJECT_RELEASE(elements);
    JXTA_OBJECT_RELEASE(el);
}

/* ************************************************** */
/* ********* *  TLS connection handshake  * ********* */
/* ************************************************** */

/* perform handshake */
void tls_connection_initiate_handshake(Jxta_transport_tls_connection * me)
{
    Jxta_transport_tls_connection *myself = PTValid(me, Jxta_transport_tls_connection);

    SSL_set_connect_state(myself->ssl);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "SSL state: %s\n", SSL_state_string_long(myself->ssl));

    /* initiate client handshake */
    BIO_write(myself->ssl_bio, "handshake", 10);

    {
        int size = BIO_ctrl_pending(myself->transport_bio);

        if (size > 0) {
            _tls_connection_buffer *output_data = transport_tls_buffer_new_1(size);

            BIO_read(myself->transport_bio, output_data->bytes, output_data->size);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "handshake: encrypting\n");
            tls_connection_log_buffer(output_data->bytes, output_data->size);

            tls_connection_send_ciphertext(myself, output_data);

            JXTA_OBJECT_RELEASE(output_data);
        }
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "SSL state: %s\n", SSL_state_string_long(myself->ssl));
}

Jxta_status tls_connection_wait_for_handshake(Jxta_transport_tls_connection * me, apr_interval_time_t timeout)
{
    Jxta_transport_tls_connection *myself = PTValid(me, Jxta_transport_tls_connection);

    Jxta_status ret = JXTA_SUCCESS;

    apr_thread_mutex_lock(myself->global_mutex);
    {
        ret = apr_thread_cond_timedwait(myself->handshake_done, myself->global_mutex, timeout);
    }
    apr_thread_mutex_unlock(myself->global_mutex);

    return ret;
}

void tls_connection_log_buffer(char *buffer, int size)
{
    int j;
    int i;
    char *logstring = malloc(size * 3 + 1 + (size / 32));

    for (j = 0, i = 0; i < size; j = j + 3, i++) {
        apr_snprintf(logstring + j, 4, " %.2X", buffer[i] & 0xFF);

        if ((i > 0) && ((i + 1) % 32 == 0)) {
            logstring[j + 3] = '\n';
            j = j + 1;
        }
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s\n", logstring);

    free(logstring);
}

/* **********************************************************/
/* ********* *  BUFFER  * ***********************************/
/* **********************************************************/

static void transport_tls_buffer_free(Jxta_object * me)
{
    _tls_connection_buffer *myself = PTValid(me, _tls_connection_buffer);
    free(myself->bytes);

    free(myself);
}

_tls_connection_buffer *transport_tls_buffer_new()
{
    _tls_connection_buffer *self = malloc(sizeof(_tls_connection_buffer));

    JXTA_OBJECT_INIT(self, transport_tls_buffer_free, NULL);

    self->thisType = "_tls_connection_buffer";

    self->bytes = NULL;
    self->pos = 0;
    self->size = 0;

    return self;
}

_tls_connection_buffer *transport_tls_buffer_new_1(int size)
{
    _tls_connection_buffer *self = transport_tls_buffer_new();

    self->size = size;
    self->bytes = malloc(size);

    return self;
}

/* vim: set ts=4 sw=4 tw=130 et: */
