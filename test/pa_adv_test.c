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
 * $Id: pa_adv_test.c,v 1.19 2005/04/07 22:58:55 slowhog Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include <apr_general.h>

#include <jxta.h>
#include <jxta_pa.h>
#include <jxta_svc.h>
#include <jxta_tta.h>
#include <jxta_hta.h>
#include <jxta_id.h>
#include <jxta_routea.h>

Jxta_boolean
pa_test(int argc, char **argv)
{
   Jxta_vector* services;
   Jxta_PA * ad;
   FILE *testfile;
   JString * js;
   int sz;
   int i;

   if(argc != 2) {

       printf("usage: ad <filename>\n");
       return FALSE;
   }

   ad = jxta_PA_new();

   testfile = fopen (argv[1], "r");

   jxta_PA_parse_file(ad,testfile);

   fclose(testfile);


   jxta_PA_get_xml(ad, &js);

   fprintf(stdout,"%s\n",jstring_get_string(js));


   JXTA_OBJECT_RELEASE(js);


   /*
    * Get svcs, to check accessors.
    */
   services = jxta_PA_get_Svc(ad);
   sz = jxta_vector_size(services);

    /* Svc is a self standing obj, so its get_xml routine has to
     * have the expected behaviour wrt to the allocation of the return
     * jstring. It creates one itself. Until we decide on generalizing
     * an append option, we have to take it and append it manually.
     * That'll do for now.
     */
   for (i = 0; i < sz; ++i) {  
       Jxta_svc * svc;
       Jxta_id* clid;
       JString* cert;
       Jxta_vector* addrs;  
       Jxta_HTTPTransportAdvertisement* hta;
       Jxta_TCPTransportAdvertisement* tta;
       Jxta_RouteAdvertisement* routea;

       jxta_vector_get_object_at(services, (Jxta_object**) &svc, i);
       clid = jxta_svc_get_MCID(svc);
       cert = jxta_svc_get_RootCert(svc);
       addrs = jxta_svc_get_Addr(svc);
       hta = jxta_svc_get_HTTPTransportAdvertisement(svc);
       tta = jxta_svc_get_TCPTransportAdvertisement(svc);
       routea = jxta_svc_get_RouteAdvertisement(svc);

       if (clid != NULL) {
	   JString* ids = NULL;
	   jxta_id_to_jstring( clid, &ids );
	   printf("class id: %s\n", jstring_get_string(ids));
	   JXTA_OBJECT_RELEASE(ids);
	   JXTA_OBJECT_RELEASE(clid);
       }

       if (cert != NULL) {
	   printf("cert: %s\n", jstring_get_string(cert));
	   JXTA_OBJECT_RELEASE(cert);
       }

       if (addrs != NULL) {
	   int i;
	   int nad = jxta_vector_size(addrs);
	   for(i = 0; i < nad; i++) {
	       JString* addr;
	       jxta_vector_get_object_at(addrs, (Jxta_object**) &addr, i);
	       printf("addr[%d]: %s\n", i, jstring_get_string(addr));
	       JXTA_OBJECT_RELEASE(addr);
	   }
	   JXTA_OBJECT_RELEASE(addrs);
       }

       /* Tired, I'll just dump the hta and tta as is for now */
       if (hta != 0) {
	   JString* dump;
	   jxta_HTTPTransportAdvertisement_get_xml(hta, &dump);
	   printf("Http Transport Adv:\n%s\n", jstring_get_string(dump));
	   JXTA_OBJECT_RELEASE(dump);
	   JXTA_OBJECT_RELEASE(hta);
       }

       if (tta != 0) {
	   JString* dump;
	   jxta_TCPTransportAdvertisement_get_xml(tta, &dump);
	   printf("Tcp Transport Adv:\n%s\n", jstring_get_string(dump));
	   JXTA_OBJECT_RELEASE(dump);
	   JXTA_OBJECT_RELEASE(tta);
       }

       if (routea != 0) {
	 JString* dump;
	 jxta_RouteAdvertisement_get_xml(routea, &dump);
	 printf("Route Adv:\n%s\n", jstring_get_string(dump));
	 JXTA_OBJECT_RELEASE(dump);
	 JXTA_OBJECT_RELEASE(routea);
       }

       JXTA_OBJECT_RELEASE(svc);
   }
   JXTA_OBJECT_RELEASE(services);
   JXTA_OBJECT_RELEASE(ad);

   /* FIXME: Figure out a sensible way to test xml processing. */
   return FALSE;
}







int
main (int argc, char **argv) {
    int rv;

    jxta_initialize();
    rv = pa_test(argc,argv);
    jxta_terminate();
    return rv;
}
