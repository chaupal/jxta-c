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
 * $Id: peers.c,v 1.9 2002/03/21 19:10:10 hamada Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include "jxta.h"
#include "jxta_resolver_service.h"
#include "jxta_errno.h"
#include "jxta_peergroup.h"
#include "jxta_object.h"
#include "jxtaapr.h"
/*
 *
 * peers command 
 *
 */

static void help (void) {
	fprintf(stdout,"peers - discover peers \n\n");
	fprintf(stdout,"SYNOPSIS\n");
	fprintf(stdout,"    peers [-p peerid name attribute]\n");
	fprintf(stdout,"           [-n n] limit the number of responses to n from a single peer\n");
	fprintf(stdout,"           [-r] discovers peer groups using remote propagation\n");
	fprintf(stdout,"           [-l] displays peer id as a hex string\n");
	fprintf(stdout,"           [-a] specify Attribute name to limit discovery to\n");
	fprintf(stdout,"           [-v] specify Attribute value to limit discovery to. wild card is allowed\n");
	fprintf(stdout,"           [-f] flush peer advertisements\n");
}

int main(int argc, char *argv[])
{
	Jxta_PG* pg;
	Jxta_discovery_service* discovery;
	Jxta_status status;
	Jxta_endpoint_address* addr = NULL;
	int c;
	boolean rf=FALSE;
	boolean pf=FALSE;
	JString * pid = jstring_new_0();
	JString * attr= jstring_new_0();
	JString * value= jstring_new_0();

	long responses = 10;

	long qid=-1;

	while (1) {
		c = getopt(argc,argv,"rflhp:n:a:v:");
		if (c == -1) {
			break;
		}
		switch (c) {
		case 'r' : rf=TRUE;
			break;

		case 'f' : break;
		case 'l' : break;

		case 'p' : pf=TRUE;
			jstring_append_2(pid,optarg);
			/*fprintf(stdout,"%s\n",optarg);*/
			break;

		case 'n' : responses = strtol(optarg, NULL, 0);
			/*fprintf(stdout,"%ld\n",responses);*/
			break;

		case 'a' : jstring_append_2(attr, optarg);
			/*fprintf(stdout,"Attribute :%s\n",optarg);*/
			break;
		case 'v' : jstring_append_2(value,optarg);
			/*fprintf(stdout,"Value :%s\n",optarg);*/
			break;
		case 'h' :
		case '?' : help();
			exit(0);
			break;

		default: fprintf(stderr,"getopt error 0%o \n",c);
		}
	}


	apr_initialize();
	status = jxta_PG_new_netpg(&pg);
	if (status != JXTA_SUCCESS) {
		fprintf(stderr,"peers: jxta_PG_netpg_new failed with error: %ld\n", status);
	}

	jxta_PG_get_discovery_service(pg, &discovery);

	if (rf) {
                jpr_thread_delay ((Jpr_interval_time) 5 *1000 *1000);
		fprintf(stdout,"sending a discovery query");
		qid=discovery_service_get_remote_advertisements(discovery,
		                jxta_id_nullID, DISC_PEER, jstring_get_string(attr),
		                jstring_get_string(value), responses, NULL);

		fprintf(stderr,"Sent discovery query qid = %ld\n", qid);
                jpr_thread_delay ((Jpr_interval_time) 7 *1000*1000);
	}
	JXTA_OBJECT_RELEASE(discovery);
	jxta_module_stop((Jxta_module*) pg);
	JXTA_OBJECT_RELEASE(pg);
	if (pid)   JXTA_OBJECT_RELEASE(pid);
	if (attr)  JXTA_OBJECT_RELEASE(attr);
	if (value) JXTA_OBJECT_RELEASE(value);
	return 0;
}
