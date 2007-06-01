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
 * $Id: jxta_tcp_message_packet_header.c,v 1.12 2006/02/15 01:09:48 slowhog Exp $
 */

#include <stdlib.h>

#include "jxta_apr.h"

#include "jxta_debug.h"
#include "jxta_errno.h"
#include "jxta_tcp_message_packet_header.h"
#include "jxta_log.h"

/**
 * New rules are:
 * header[0] -> content-type
 * header[1] -> content-length
 * header[2] -> srcEA
 */
#define CONTENT_TYPE_HEADER 0
#define CONTENT_LENGTH_HEADER 1
#define SRC_EA_HEADER 2

#define MESSAGE_PACKET_HEADER_COUNT 3

typedef unsigned char BYTE;
typedef unsigned short int BYTE_2;

typedef struct _message_packet_header {
    BYTE *name;
    BYTE name_size;             /* 1 byte */
    BYTE *value;
    BYTE_2 value_size;          /* 2 bytes */
} MessagePacketHeader;

const char *MESSAGE_PACKET_HEADER[MESSAGE_PACKET_HEADER_COUNT] = { "content-type", "content-length", "srcEA" };
const char *__log_cat = "TCP_MSG_PKT";

Jxta_status JXTA_STDCALL message_packet_header_read(char *header_buf, ReadFunc read_func, void *stream, JXTA_LONG_LONG * msg_size,
                                                    Jxta_boolean is_multicast, char **src_addr)
{
    MessagePacketHeader header[MESSAGE_PACKET_HEADER_COUNT];    /* for extra if there exists content-coding */
    Jxta_boolean saw_empty, saw_length, saw_type, saw_srcEA;
    int i, length_index = -1, srcEA_index = -1;
    int header_count, packet_header_nb;

    Jxta_status res;

    header_count = is_multicast ? MESSAGE_PACKET_HEADER_COUNT : MESSAGE_PACKET_HEADER_COUNT - 1;

    saw_empty = FALSE;
    saw_length = FALSE;
    saw_type = FALSE;
    saw_srcEA = FALSE;
    i = 0;

    do {
        BYTE header_name_length;
        /* read header name size */
        res = read_func(stream, (char *) &header_name_length, 1);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Error during read_func() \n");
            return JXTA_IOERR;  /* socket receiving error */
        }
        if (header_name_length == 0) {
            if (saw_type && saw_length) {
                if (header_count == 2)
                    saw_empty = TRUE;
                else if (header_count == 3 && saw_srcEA)
                    saw_empty = TRUE;
            }
        } else {
            if (header_name_length > HEADER_BUFSIZE) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Too large header name\n");
                return JXTA_FAILED;
            }

            res = read_func(stream, header_buf, header_name_length);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Error during read_func() \n");
                return JXTA_IOERR;  /* socket receiving error */
            }
            packet_header_nb = -1;
            for (i = 0; i < MESSAGE_PACKET_HEADER_COUNT; i++) {
                if (strncasecmp(header_buf, MESSAGE_PACKET_HEADER[i], header_name_length) == 0) {
                    packet_header_nb = i;
                    break;
                }
            }

            if (packet_header_nb == -1)
                return JXTA_FAILED;

            switch (packet_header_nb) {
            case CONTENT_TYPE_HEADER:  /* content-type */
                if (saw_type == TRUE) {
                    JXTA_LOG("Duplicate content-type header\n");
                    return JXTA_FAILED;
                }
                saw_type = TRUE;
                break;
            case CONTENT_LENGTH_HEADER:    /* content-length */
                if (saw_length == TRUE) {
                    JXTA_LOG("Duplicate content-length header\n");
                    return JXTA_FAILED;
                }
                saw_length = TRUE;
                length_index = i;
                break;
            case SRC_EA_HEADER:    /* srcEA */
                if (saw_srcEA == TRUE) {
                    JXTA_LOG("Duplicate srcEA header\n");
                    return JXTA_FAILED;
                }
                saw_srcEA = TRUE;
                srcEA_index = i;
                break;
            }

            /* read header body size */
            res = read_func(stream, (char *) &header[i].value_size, 2);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Error during read_func() \n");
                return JXTA_FAILED;
            }
            header[i].value_size = ntohs(header[i].value_size);
            header[i].value = (BYTE *) malloc(header[i].value_size + 1);
            if (header[i].value == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error header body malloc()\n");
                return JXTA_FAILED;
            }
            /* read header body value */
            res = read_func(stream, (char *) header[i].value, header[i].value_size);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Error during read_func() \n");
                return JXTA_FAILED;
            }
            header[i].value[header[i].value_size] = '\0';

            if (i == srcEA_index) {
                if (NULL != src_addr) {
                    *src_addr = strdup((char *) header[i].value);
                }
            }

            i++;
            if (i > header_count) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error i:%i > header count:%i\n", i, header_count);
                for (i = 0; i < header_count; i++) {
                    if (header[i].value != NULL)
                        free(header[i].value);
                }
                return JXTA_FAILED;
            }
        }
    } while (!saw_empty);

    /* get message size */
    for (i = 0, *msg_size = 0; i < 8; i++)
        *msg_size |= ((JXTA_LONG_LONG) (header[length_index].value[i] & 0xFF)) << ((7 - i) * 8L);

    /* free */
    for (i = 0; i < header_count; i++) {
        if (header[i].value != NULL)
            free(header[i].value);
    }

    return JXTA_SUCCESS;
}

Jxta_status JXTA_STDCALL message_packet_header_write(WriteFunc write_func, void *stream, JXTA_LONG_LONG msg_size,
                                                     Jxta_boolean is_multicast, char *src_addr)
{
    MessagePacketHeader header[MESSAGE_PACKET_HEADER_COUNT];
    BYTE empty_header = 0;
    int i, j;
    int header_count;

    header_count = is_multicast ? MESSAGE_PACKET_HEADER_COUNT : MESSAGE_PACKET_HEADER_COUNT - 1;

    /* write message header */
    for (i = 0; i < header_count; i++) {
        header[i].name = (BYTE *) MESSAGE_PACKET_HEADER[i];
        header[i].name_size = strlen((char *) header[i].name);

        switch (i) {
        case CONTENT_TYPE_HEADER:
            header[i].value = (BYTE *) APP_MSG; /* "application/x-jxta-msg" */
            header[i].value_size = strlen((char *) header[i].value);
            break;
        case CONTENT_LENGTH_HEADER:
            header[i].value = (BYTE *) malloc(8);
            for (j = 0; j < 8; j++) {
                header[i].value[j] = (BYTE) (msg_size >> ((7 - j) * 8L));   /* JXTA_LONG_LONG to bytes */
            }
            header[i].value_size = 8;
            break;
        case SRC_EA_HEADER:
            header[i].value = (BYTE *) src_addr;
            header[i].value_size = strlen((char *) header[i].value);
            break;
        }

        header[i].value_size = htons(header[i].value_size);

        write_func(stream, (char *) &header[i].name_size, 1);   /* size = 1 */
        write_func(stream, (char *) header[i].name, header[i].name_size);
        write_func(stream, (char *) &header[i].value_size, 2);  /* size = 2 */
        write_func(stream, (char *) header[i].value, ntohs(header[i].value_size));
    }

    write_func(stream, (char *) &empty_header, 1);

    /** Free message size */
    free(header[CONTENT_LENGTH_HEADER].value);

    return JXTA_SUCCESS;
}

/* vi: set ts=4 sw=4 tw=130 et: */
