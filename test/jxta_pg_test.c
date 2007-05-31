/*
 * Copyright (c) 2002-204 Sun Microsystems, Inc.  All rights
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
 * $Id: jxta_pg_test.c,v 1.4 2004/05/20 01:27:37 tra Exp $
 */

#include "jxta.h"
#include "jxta_peergroup.h"
#include "jxta_endpoint_service.h"
#include "jxta_rdv_service.h"
#include "jxta_resolver_service.h"
#include "jxta_object.h"
#include "jxtaapr.h"

int main(int argc, char *argv[])
{
    Jxta_PG* netpg;
    Jxta_PG* pg;
    Jxta_PGA* pga;
    JString* pgas;
    Jxta_endpoint_service* endp;
    Jxta_rdv_service* rdv;
    Jxta_resolver_service* resolver;
    Jxta_status res;
    JString* newpg_name;
    JString* newpg_desc;
    Jxta_MIA* newpg_impl;
    Jxta_MIA* mia;
    JString* mias;
    JString* compat1;
    JString* compat2;
    Jxta_id* test_gid = NULL;
    Jxta_PG* same_pg = NULL;

    apr_initialize();

    res = jxta_PG_new_netpg(&netpg);
    if (res != JXTA_SUCCESS) {
	printf("jxta_PG_new_netpg failed with error: %ld\n", res);
    }

    jxta_PG_get_genericpeergroupMIA(netpg, &newpg_impl);
    newpg_name = jstring_new_2("jice_test_pg");
    newpg_desc = jstring_new_2("testing generic pg creation in C");
    res = jxta_PG_newfromimpl(netpg,
			      NULL,
			      (Jxta_advertisement*) newpg_impl,
			      newpg_name,
			      newpg_desc,
			      NULL,
			      &pg);

    if (res != JXTA_SUCCESS) {
	printf("jxta_PG_newfromimpl failed with error: %ld\n", res);
    }

    jxta_PG_get_rendezvous_service(pg, &rdv);
    jxta_PG_get_endpoint_service(pg, &endp);
    jxta_PG_get_resolver_service(pg, &resolver);

    jxta_PG_get_PGA(pg, &pga);
    jxta_PG_get_GID(pg, &test_gid);
    jxta_PGA_get_xml(pga, &pgas);
    jxta_PG_get_compatstatement(pg, &compat1);
    jxta_service_get_MIA((Jxta_service*) pg, (Jxta_advertisement**) &mia);
    jxta_MIA_get_xml(mia, &mias);
    compat2 = jxta_MIA_get_Comp(mia);

    jxta_PG_newfromimpl(netpg,
			test_gid,
			(Jxta_advertisement*) newpg_impl,
			newpg_name,
			newpg_desc,
			NULL,
			&same_pg);

    printf("PGA:\n%s\n", jstring_get_string(pgas));
    printf("Compat:\n%s\n", jstring_get_string(compat1));
    printf("MIA:\n%s\n", jstring_get_string(mias));
    printf("Compatibility test: %s\n", jxta_PG_is_compatible(netpg, compat2) ? "Compat" : "Incompat");
    printf("Registry test: %s\n", same_pg == pg ? "passed" : "failed");

    if (same_pg != NULL) JXTA_OBJECT_RELEASE(same_pg);
    JXTA_OBJECT_RELEASE(test_gid);
    JXTA_OBJECT_RELEASE(endp);
    JXTA_OBJECT_RELEASE(rdv);
    JXTA_OBJECT_RELEASE(resolver);
    JXTA_OBJECT_RELEASE(pga);
    JXTA_OBJECT_RELEASE(pgas);
    JXTA_OBJECT_RELEASE(newpg_name);
    JXTA_OBJECT_RELEASE(newpg_desc);
    JXTA_OBJECT_RELEASE(newpg_impl);
    JXTA_OBJECT_RELEASE(mia);
    JXTA_OBJECT_RELEASE(mias);
    JXTA_OBJECT_RELEASE(compat1);
    JXTA_OBJECT_RELEASE(compat2);

    printf("start another peer and look for groups.\n"
	   "type q<return> when done\n");
    while (getchar() != 'q');
    
    jxta_module_stop((Jxta_module*) pg);
    JXTA_OBJECT_RELEASE(pg);

    jxta_module_stop((Jxta_module*) netpg);
    JXTA_OBJECT_RELEASE(netpg);
    return 0;
}
