/*
 * Copyright (c) 2005 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: npg_test.c,v 1.1.2.4 2005/05/27 20:42:30 slowhog Exp $
 */

#include <apr_time.h>

#include "jxta.h"
#include "jxta_peergroup.h"
#include "jxta_rdv_service.h"
#include <sys/stat.h>

Jxta_boolean CreateAndWritePipeAdvToFile(Jxta_PG *pg, const char* szFile);
Jxta_boolean LoadPipeAdvFromFile(const char* szFile, Jxta_pipe_adv** pPipeAdv);
Jxta_boolean Connect(Jxta_PG *pg, Jxta_pipe_adv* pPipeAdv, Jxta_pipe** pPipe);
Jxta_boolean Disconnect(Jxta_pipe* pPipe);
Jxta_boolean SendJxtaMessage(Jxta_pipe* pPipe, const char* caMessage, unsigned long ulSize);
Jxta_boolean CreateListenerPipe(Jxta_PG *pg, Jxta_pipe_adv* pPipeAdv, Jxta_pipe** pPipe);
Jxta_boolean DestroyListenerPipe(Jxta_pipe* pPipe);
Jxta_boolean StartListeningForMessages(Jxta_pipe* pPipe, Jxta_listener** pListener, Jxta_inputpipe** InputPipe);
Jxta_boolean StopListeningForMessages(Jxta_listener* pListener, Jxta_inputpipe* InputPipe);
Jxta_boolean GeneratePipeID(Jxta_PG *pg, char* szPipeId);
static void ListenerProc(Jxta_object* pJxtaObj, void* pvArg);

/* rerutn number of connected peers, minus number in case of errors */
int display_peers(Jxta_rdv_service* rdv) {
    int res = 0;
    Jxta_peer* peer = NULL;
    Jxta_id* pid = NULL;
    Jxta_PA*      adv = NULL;
    JString*  string = NULL;
    Jxta_time expires = 0;
    Jxta_boolean connected = FALSE;
    Jxta_status err;
    Jxta_vector* peers = NULL;
    Jxta_time currentTime;
    int i;
    char  linebuff[1024];
    JString * outputLine = jstring_new_0();
    JString * disconnectedMessage = jstring_new_2("Status: NOT CONNECTED\n");

    /* Get the list of peers */
    err = jxta_rdv_service_get_peers(rdv, &peers);
    if (err != JXTA_SUCCESS) {
        jstring_append_2(outputLine, "Failed getting the peers.\n");
        res = -1;
        goto Common_Exit;
    }

    for (i = 0; i < jxta_vector_size(peers); ++i) {
        err = jxta_vector_get_object_at(peers,(Jxta_object**) &peer, i);
        if (err != JXTA_SUCCESS) {
            jstring_append_2(outputLine, "Failed getting a peer.\n");
            res = -2;
            goto Common_Exit;
        }

        connected = jxta_rdv_service_peer_is_connected(rdv, peer);
        if (connected) {
            res++;
            err = jxta_peer_get_adv(peer, &adv);
            if (err == JXTA_SUCCESS) {
                string = jxta_PA_get_Name(adv);
                JXTA_OBJECT_RELEASE(adv);
                sprintf(linebuff, "Name: [%s]\n", jstring_get_string(string));
                jstring_append_2(outputLine, linebuff);
                JXTA_OBJECT_RELEASE(string);
            }
            err = jxta_peer_get_peerid(peer, &pid);
            if (err == JXTA_SUCCESS) {
                jxta_id_to_jstring(pid, &string);
                JXTA_OBJECT_RELEASE(pid);
                sprintf(linebuff, "PeerId: [%s]\n", jstring_get_string(string));
                jstring_append_2(outputLine, linebuff);
                JXTA_OBJECT_RELEASE(string);
            }

            sprintf(linebuff, "Status: %s\n", connected ? "CONNECTED" : "NOT CONNECTED");
            jstring_append_2(outputLine, linebuff);
            expires = jxta_rdv_service_peer_get_expires(rdv, peer);

            if (connected && (expires >= 0)) {
                Jxta_time hours = 0;
                Jxta_time minutes = 0;
                Jxta_time seconds = 0;

                currentTime = jpr_time_now();
                if (expires < currentTime) {
                    expires = 0;
                } else {
                    expires -= currentTime;
                }

                seconds = expires / (1000);

                hours = (Jxta_time) (seconds / (Jxta_time) (60 * 60));
                seconds -= hours * 60 * 60;

                minutes = seconds / 60;
                seconds -= minutes * 60;

                /* This produces a compiler warning about L not being ansi.
                 * But long longs aren't ansi either. 
                 */
#ifndef WIN32

                sprintf(linebuff, "\nLease expires in %lld hour(s) %lld minute(s) %lld second(s)\n",
                         (Jxta_time) hours,
                         (Jxta_time) minutes,
                         (Jxta_time) seconds);
#else

                sprintf(linebuff, "\nLease expires in %I64d hour(s) %I64d minute(s) %I64d second(s)\n",
                         (Jxta_time) hours,
                         (Jxta_time) minutes,
                         (Jxta_time) seconds);
#endif

                jstring_append_2(outputLine, linebuff);
            }
            jstring_append_2(outputLine, "-----------------------------------------------------------------------------\n");
        }
        JXTA_OBJECT_RELEASE(peer);
    }
    JXTA_OBJECT_RELEASE(peers);

Common_Exit:
    if(jstring_length(outputLine) > 0) {
        printf(jstring_get_string(outputLine));
    } else {
        printf(jstring_get_string(disconnectedMessage));
    }

    JXTA_OBJECT_RELEASE(outputLine);
    JXTA_OBJECT_RELEASE(disconnectedMessage);
    return res;
}

static int shell_loop(Jxta_PG *pg, const char *pipe_adv_fname)
{
    const char* szMyMessage = "Hellow, How are you?";
    Jxta_pipe_adv* pPipeAdv = NULL;
    Jxta_pipe* pPipe = NULL;
    Jxta_listener* pListener = NULL;
    Jxta_inputpipe* InputPipe = NULL;
    int nOption = 10;

    while(nOption)
    {
        printf("********************JXTA CONNECTION MENU********\n");
        printf("                                                 \n");
        printf("    Enter 1 to create a pipe advertsiement      \n");
        printf("    Enter 2 to read pipe advertisement          \n");
        printf("    Enter 3 to connect to remote pipe           \n");
        printf("    Enter 4 to disconnect from remote pipe      \n");
        printf("    Enter 5 to send message                  \n");
        printf("    Enter 6 to create listening pipe             \n");
        printf("    Enter 7 to destroy listening pipe            \n");
        printf("    Enter 8 to start listening                  \n");
        printf("    Enter 9 to stop listening                    \n");
        printf("                                                 \n");
        printf("************************************************\n");
        scanf("%d", &nOption);
        switch(nOption)
        {
        case 1:
            CreateAndWritePipeAdvToFile(pg, pipe_adv_fname);
            break;
        case 2:
            LoadPipeAdvFromFile(pipe_adv_fname, &pPipeAdv);
            break;
        case 3:
            if (pPipeAdv) {
                Connect(pg, pPipeAdv, &pPipe);
            } else {
                printf("Please specify a pipe advertisement.\n");
            }
            break;
        case 4:
            if (pPipe) {
                Disconnect(pPipe);
                pPipe = NULL;
            } else {
                printf("Pipe is not connected.\n");
            }
            break;
        case 5:
            if (pPipe) {
                SendJxtaMessage(pPipe, szMyMessage, strlen(szMyMessage));
            } else {
                printf("Pipe is not connected.\n");
            }
            break;
        case 6:
            CreateListenerPipe(pg, pPipeAdv, &pPipe);
            break;
        case 7:
            DestroyListenerPipe(pPipe);
            pPipe = NULL;
            break;
        case 8:
            StartListeningForMessages(pPipe, &pListener, &InputPipe);
            break;
        case 9:
            StopListeningForMessages(pListener, InputPipe);
            pListener = NULL;
            InputPipe = NULL;
            break; 
        default:
            break;
        }
    }

    if ( pPipeAdv )
    {
        JXTA_OBJECT_RELEASE(pPipeAdv);
        pPipeAdv = NULL;
    }

    if ( InputPipe )
    {
        StopListeningForMessages(pListener, InputPipe);
        pListener = NULL;
        InputPipe = NULL;
    }

    if ( pPipe )
    {
        JXTA_OBJECT_RELEASE(pPipe);
        pPipe = NULL;
    }
    return 0;
}

int main(int argc, char **argv)
{
	int a, rval; 
    Jxta_PG *pg;
    int peer_cnt, retry;
    Jxta_rdv_service *rdv;
    Jxta_log_file *f;
    Jxta_log_selector *s;

    Jxta_status rv;

    if (argc < 2) {
        printf("Usage: %s <pipe_adv_filename> [iterations]\n", argv[0]);
        return 1;
    }

    jxta_initialize();
    jxta_log_file_open(&f, "npg_test.log");
    s = jxta_log_selector_new_and_set("*.*", &rv);
    jxta_log_file_attach_selector(f, s, NULL);
    jxta_log_using(jxta_log_file_append, f);

    if (argc < 3) {
        a = 1;
    } else {
        a = atoi(argv[2]);
    }

    rval = 0;
	while(a--) 
	{
        /* Create and start peer group */
        jxta_PG_new_netpg(&pg);

        jxta_PG_get_rendezvous_service(pg, &rdv);
        peer_cnt = 0;

        for (retry = 3; 0 == peer_cnt && retry > 0; --retry) {
            printf("waiting for 5 seconds to see if RDV connects...\n");
            apr_sleep((Jxta_time)5000000L);
            peer_cnt = display_peers(rdv);
        }

        JXTA_OBJECT_RELEASE(rdv);

        rval = shell_loop(pg, argv[1]);

        /* stop peer group */
        printf("Stopping PeerGroup ...\n");
        jxta_module_stop((Jxta_module *)pg);
        /* release jxta instance here */
        printf("PeerGroup Ref Count after stop :%d.\n", JXTA_OBJECT_GET_REFCOUNT(pg));
        JXTA_OBJECT_RELEASE(pg);
        pg = NULL;

        if (rval) break;
	}

    jxta_log_using(NULL, NULL);
    jxta_log_file_close(f);
    jxta_log_selector_delete(s);

    jxta_terminate();
    return rval;
}

Jxta_boolean CreateAndWritePipeAdvToFile(Jxta_PG *pg, const char* szFile)
{
    Jxta_pipe_adv* pPipeAdv = NULL;
    JString* jstrPipeAdv;
    char szPipeId[256];
    FILE *advfile;
    const char* pstrPipeAdv = NULL;

    if ( FALSE == GeneratePipeID(pg, szPipeId) )
    {
        return FALSE;
    }

    /* Create a pipe advertisment instance */
    pPipeAdv = jxta_pipe_adv_new();

    /* Sets the unique identifier of the Pipe Advertisement. */
    jxta_pipe_adv_set_Id(pPipeAdv, szPipeId);

    /*  Sets the type of the Pipe Advertisement. */
    jxta_pipe_adv_set_Type(pPipeAdv, JXTA_UNICAST_PIPE);

    /* Sets the name of the Pipe Advertisement. */
    jxta_pipe_adv_set_Name(pPipeAdv, "Jxta_C_NPG_Test_Pipe");

    jxta_pipe_adv_get_xml(pPipeAdv, &jstrPipeAdv);
    pstrPipeAdv = jstring_get_string (jstrPipeAdv);

    advfile = fopen (szFile, "w");
    if (advfile == NULL) 
    {
        return FALSE;
    }

    fwrite(pstrPipeAdv, strlen(pstrPipeAdv), 1, advfile);
    JXTA_OBJECT_RELEASE(jstrPipeAdv);
    JXTA_OBJECT_RELEASE(pPipeAdv);

    fclose(advfile);
    return TRUE;
}
/******************************************************************************
    FUNCTION NAME : 
          PURPOSE : 
            INPUT : 
           OUTPUT : 
           RETURN : 
******************************************************************************/
Jxta_boolean LoadPipeAdvFromFile(const char* szFile, Jxta_pipe_adv** pPipeAdv)
{
    struct stat buf;
    int nFileSize;
    int stat_result;
    FILE * fp = NULL;
    char *pBuffer;
    size_t nSize;

    stat_result = stat(szFile, &buf);
    if ( 0 != stat_result ) 
    {
        return FALSE;
    }

    nFileSize = buf.st_size;
    fp = fopen(szFile,"rb");
    if (0==fp) 
    {
        return FALSE;
    }

    pBuffer = NULL;
    pBuffer = calloc(nFileSize + 1, sizeof(char));
    nSize = fread(pBuffer, nFileSize, 1, fp);
    if ( nSize != 1 ) 
    {
        return FALSE;
    }
    pBuffer[nFileSize] = '\0';
    fclose(fp);

    *pPipeAdv = jxta_pipe_adv_new();
    jxta_pipe_adv_parse_charbuffer(*pPipeAdv, pBuffer, nFileSize);

    free(pBuffer);

    return TRUE;
}
/******************************************************************************
    FUNCTION NAME : 
          PURPOSE : 
            INPUT : 
           OUTPUT : 
           RETURN : 
******************************************************************************/
Jxta_boolean Connect(Jxta_PG *pg, Jxta_pipe_adv* pPipeAdv, Jxta_pipe** pPipe)
{
    Jxta_pipe_service* PipeService;
    jxta_PG_get_pipe_service(pg, &PipeService);
    /* wait to connect in 60 secs */
    jxta_pipe_service_timed_connect(PipeService, pPipeAdv, (Jxta_time_diff) 60000000L, NULL, pPipe);
    JXTA_OBJECT_RELEASE(PipeService);
    return TRUE;
}
/******************************************************************************
    FUNCTION NAME : 
          PURPOSE : 
            INPUT : 
           OUTPUT : 
           RETURN : 
******************************************************************************/
Jxta_boolean Disconnect(Jxta_pipe* pPipe)
{
    JXTA_OBJECT_RELEASE(pPipe);
    return TRUE;
}
/******************************************************************************
    FUNCTION NAME : 
          PURPOSE : 
            INPUT : 
           OUTPUT : 
           RETURN : 
******************************************************************************/
Jxta_boolean CreateListenerPipe(Jxta_PG *pg, Jxta_pipe_adv* pPipeAdv, Jxta_pipe** pPipe)
{
    Jxta_pipe_service* PipeService;
    Jxta_status res = 0;

    jxta_PG_get_pipe_service(pg, &PipeService);
    /* wait for connection for 60 secs */
    res = jxta_pipe_service_timed_accept(PipeService, pPipeAdv, 60000000L, pPipe);
    JXTA_OBJECT_RELEASE(PipeService);
    if ( res != JXTA_SUCCESS) 
    {
        return FALSE;
    }

    return TRUE;
}
/******************************************************************************
    FUNCTION NAME : 
          PURPOSE : 
            INPUT : 
           OUTPUT : 
           RETURN : 
******************************************************************************/
Jxta_boolean DestroyListenerPipe(Jxta_pipe* pPipe)
{
    JXTA_OBJECT_RELEASE(pPipe);
    return TRUE;
}
/******************************************************************************
    FUNCTION NAME : 
          PURPOSE : 
            INPUT : 
           OUTPUT : 
           RETURN : 
******************************************************************************/
Jxta_boolean SendJxtaMessage(Jxta_pipe* pPipe, const char* caMessage, unsigned long ulSize)
{
    Jxta_status res;
    Jxta_outputpipe* op = NULL;
    Jxta_message* msg = NULL;
    Jxta_message_element*  el = NULL;

    if ( NULL == pPipe )
    {
        return FALSE;
    }
  
    res = jxta_pipe_get_outputpipe(pPipe, &op);
    if ( JXTA_SUCCESS != res )
    {
        return FALSE;
    }

    msg = jxta_message_new();
    el = jxta_message_element_new_1(
    "MY_JXTA_MESSAGE",
    "text/plain",
    caMessage,
    ulSize,
    NULL);

    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    res = jxta_outputpipe_send(op, msg);
    JXTA_OBJECT_RELEASE(msg);
    JXTA_OBJECT_RELEASE(op);
    if ( JXTA_SUCCESS != res )
    {
        return FALSE;
    }

    return TRUE;
}
/******************************************************************************
    FUNCTION NAME : 
          PURPOSE : 
            INPUT : 
           OUTPUT : 
           RETURN : 
******************************************************************************/
Jxta_boolean GeneratePipeID(Jxta_PG *pg, char* szPipeId)
{
    /* Get group id from Pipe Manager. */
    /* Create new pipe ID */
    Jxta_id* jGID = NULL;
    Jxta_id* jPipeId = NULL;
    JString* jstrPipeId = NULL;

    jxta_PG_get_GID(pg, &jGID);
    jxta_id_pipeid_new_1(&jPipeId, jGID);
    JXTA_OBJECT_RELEASE(jGID);
    JXTA_OBJECT_CHECK_VALID(jPipeId);

    /* Convert to JXTA string and validate */
    jxta_id_to_jstring(jPipeId, &jstrPipeId);
    JXTA_OBJECT_CHECK_VALID (jstrPipeId);

    strcpy(szPipeId, (char*)jstring_get_string(jstrPipeId));
    JXTA_OBJECT_RELEASE(jPipeId);
    JXTA_OBJECT_RELEASE(jstrPipeId);
    return TRUE;
}
/******************************************************************************
    FUNCTION NAME : 
          PURPOSE : 
            INPUT : 
           OUTPUT : 
           RETURN : 
******************************************************************************/
Jxta_boolean StartListeningForMessages(
    Jxta_pipe* pPipe, 
    Jxta_listener** pListener, 
    Jxta_inputpipe** InputPipe)
{
    Jxta_status res;

    if ( NULL == pPipe ) return FALSE;

    res = jxta_pipe_get_inputpipe(pPipe, InputPipe);
    if ( JXTA_SUCCESS != res )
    {
        return FALSE;
    }

    *pListener = jxta_listener_new(ListenerProc, NULL, 1, 200);
    res = jxta_inputpipe_add_listener(*InputPipe, *pListener);
    if ( JXTA_SUCCESS != res )
    {
        JXTA_OBJECT_RELEASE(*pListener);
        *pListener = NULL;
        JXTA_OBJECT_RELEASE(*InputPipe);
        *InputPipe = NULL;
        return FALSE;
    }

    jxta_listener_start(*pListener);
    return TRUE;
}
/******************************************************************************
    FUNCTION NAME : 
          PURPOSE : 
            INPUT : 
           OUTPUT : 
           RETURN : 
******************************************************************************/
Jxta_boolean StopListeningForMessages(Jxta_listener* pListener, Jxta_inputpipe* InputPipe)
{
    Jxta_status res;
    if ( NULL == pListener || NULL == InputPipe ) return FALSE;
    res = jxta_inputpipe_remove_listener(InputPipe, pListener);
    if ( JXTA_SUCCESS != res )
    {
        return FALSE;
    }
    jxta_listener_stop(pListener);
    JXTA_OBJECT_RELEASE(pListener);
    JXTA_OBJECT_RELEASE(InputPipe);
    return TRUE;
}
/******************************************************************************
    FUNCTION NAME : 
          PURPOSE : 
            INPUT : 
           OUTPUT : 
           RETURN : 
******************************************************************************/
void ListenerProc(Jxta_object* pJxtaObj, void* pvArg)
{
    Jxta_status res;
    Jxta_message* msg = (Jxta_message*)pJxtaObj;
    Jxta_message_element* el;
    res = jxta_message_get_element_1(msg, "MY_JXTA_MESSAGE", &el);
    if ( JXTA_SUCCESS == res )
    {
        Jxta_bytevector* byteVector = jxta_message_element_get_value(el);
        unsigned long ulLen = (unsigned long)jxta_bytevector_size(byteVector);
        unsigned long ulIndex = 0;
        while ( ulIndex < ulLen ) 
        {
            unsigned char byteValue;
            res = jxta_bytevector_get_byte_at(byteVector, &byteValue, ulIndex);
            if ( JXTA_SUCCESS == res )
                putchar(byteValue);
            ulIndex++;
        }
        JXTA_OBJECT_RELEASE(byteVector);
        JXTA_OBJECT_RELEASE(el);
    }
}
/* vim: set ts=4 sw=4 tw=130 et: */
