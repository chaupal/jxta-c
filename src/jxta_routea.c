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
 * $Id: jxta_routea.c,v 1.9 2005/02/02 02:58:32 exocetrick Exp $
 */

   
/* 
 * The following command will compile the output from the script 
 * given the apr is installed correctly.
 */
/*
 * gcc -DSTANDALONE jxta_advertisement.c jxta_RouteAdvertisement.c  -o PA \
     `/usr/local/apache2/bin/apr-config --cflags --includes --libs` \
     -lexpat -L/usr/local/apache2/lib/ -lapr
 */
 
#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jdlist.h"
#include "jxta_routea.h"
#include "jxta_apa.h"
#include "jxta_xml_util.h"

#define DEBUG 1
 
#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif
 
/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
                Null_,
                Jxta_RouteAdvertisement_,
		DESTPID_,
                DEST_,
                HOPS_
};
 
/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_RouteAdvertisement {

   Jxta_advertisement jxta_advertisement;
   char          * jxta_RouteAdvertisement;
   Jxta_AccessPointAdvertisement       *dest;
   Jxta_vector* hops;
};
 
/* Forw decl for un-exported function */
static void jxta_RouteAdvertisement_delete(Jxta_RouteAdvertisement *);
char* jxta_RouteAdvertisement_get_DestPID_string(Jxta_RouteAdvertisement * ad); 

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void
handleJxta_RouteAdvertisement(void * userdata, const XML_Char * cd, int len) {
}

static void
handleDest(void * userdata, const XML_Char * cd, int len)
{
  Jxta_AccessPointAdvertisement *dst = NULL;
  Jxta_RouteAdvertisement * ad
    = (Jxta_RouteAdvertisement*)userdata;
  Jxta_id * pid = NULL;

  if (ad->dest == NULL) {
    dst = jxta_AccessPointAdvertisement_new();
    jxta_RouteAdvertisement_set_Dest(ad, dst);
    JXTA_OBJECT_RELEASE(dst);
  } else {
    pid = jxta_AccessPointAdvertisement_get_PID(ad->dest);
  }

  jxta_advertisement_set_handlers((Jxta_advertisement *)  ad->dest,
				  ((Jxta_advertisement *) ad)->parser,
				  (void*)ad);

  if (pid != NULL) {
    jxta_AccessPointAdvertisement_set_PID(ad->dest, pid);
    JXTA_OBJECT_RELEASE(pid);
  }
}
static void
handleDestPID(void * userdata, const XML_Char * cd, int len)
{
  Jxta_id* pid = NULL;
  Jxta_RouteAdvertisement * ad
    = (Jxta_RouteAdvertisement*) userdata;
  JString * tmp;
  Jxta_AccessPointAdvertisement* dst;

  if (len == 0) return;
  
  /* Make room for a final \0 in advance; we'll likely need it. */
  tmp = jstring_new_1(len + 1);

  jstring_append_0(tmp, (char*)cd, len);
  jstring_trim(tmp);

  jxta_id_from_jstring( &pid, tmp);
  JXTA_OBJECT_RELEASE(tmp);

  if (pid != NULL) {
    if (ad->dest == NULL) {
      dst = jxta_AccessPointAdvertisement_new();
      jxta_RouteAdvertisement_set_Dest(ad, dst);
      JXTA_OBJECT_RELEASE(dst);
    }

    jxta_RouteAdvertisement_set_DestPID(ad, pid);
    JXTA_OBJECT_RELEASE(pid);
  }
}

static void
handleHops(void * userdata, const XML_Char * cd, int len)
{

   Jxta_RouteAdvertisement * ad
       = (Jxta_RouteAdvertisement*)userdata;
   Jxta_AccessPointAdvertisement *hop = NULL;
   
   hop = jxta_AccessPointAdvertisement_new();
   JXTA_LOG("Hop element\n");

   jxta_advertisement_set_handlers((Jxta_advertisement *)  hop,
				   ((Jxta_advertisement *) ad)->parser,
				   (void*) ad);
   jxta_vector_add_object_last( ad->hops, (Jxta_object*)hop);
   JXTA_OBJECT_RELEASE(hop); 
}

 
/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */
char *
jxta_RouteAdvertisement_get_Jxta_RouteAdvertisement(Jxta_RouteAdvertisement * ad) {
   return NULL;
}

char *
jxta_RouteAdvertisement_get_Jxta_RouteAdvertisement_string(Jxta_advertisement * ad) {
   return NULL;
}
 
void
jxta_RouteAdvertisement_set_Jxta_RouteAdvertisement(Jxta_RouteAdvertisement * ad, char * name) { 
}
 
Jxta_AccessPointAdvertisement *
jxta_RouteAdvertisement_get_Dest(Jxta_RouteAdvertisement * ad) {
    if (ad->dest != NULL) {
      JXTA_OBJECT_SHARE(ad->dest);
    }
    return ad->dest;
}
 
void
jxta_RouteAdvertisement_set_Dest(Jxta_RouteAdvertisement * ad, Jxta_AccessPointAdvertisement *dst) {

    if (ad->dest != NULL)
      JXTA_OBJECT_RELEASE(ad->dest);
    ad->dest = dst;
    JXTA_OBJECT_SHARE(ad->dest);
}

Jxta_id *
jxta_RouteAdvertisement_get_DestPID(Jxta_RouteAdvertisement * ad) {
    Jxta_id* id = NULL;

    if (ad->dest != NULL) {
      id = jxta_AccessPointAdvertisement_get_PID(ad->dest);
      if (id != NULL)
	JXTA_OBJECT_SHARE(id);
    }
    return id;
}

char*
jxta_RouteAdvertisement_get_DestPID_string(Jxta_RouteAdvertisement * ad) {

    char* res;
    Jxta_id* id = NULL;
    JString* js = NULL;

    if (ad->dest != NULL) {
      id = jxta_AccessPointAdvertisement_get_PID(ad->dest);
      if (id != NULL) {  
	jxta_id_to_jstring(id, &js);
	JXTA_OBJECT_RELEASE(id);
	res = strdup(jstring_get_string(js));
	JXTA_OBJECT_RELEASE(js);
	return res;
      }
    }
    return NULL;
}
 
void
jxta_RouteAdvertisement_set_DestPID(Jxta_RouteAdvertisement * ad, Jxta_id *id) {
    if (ad->dest != NULL)
      jxta_AccessPointAdvertisement_set_PID(ad->dest, id);    
}

Jxta_vector *
jxta_RouteAdvertisement_get_Hops(
	   Jxta_RouteAdvertisement * ad)
{
  JXTA_OBJECT_SHARE(ad->hops);
  return ad->hops;
}

void
jxta_RouteAdvertisement_set_Hops(Jxta_RouteAdvertisement * ad, Jxta_vector * hops) {
    JXTA_OBJECT_SHARE(hops);
    JXTA_OBJECT_RELEASE(ad->hops);
    ad->hops = hops;
}
 
void
jxta_RouteAdvertisement_add_first_hop(Jxta_RouteAdvertisement * ad, Jxta_AccessPointAdvertisement* hop) {

  if (ad->hops == NULL) {
    ad->hops = jxta_vector_new(1);
  }
  jxta_vector_add_object_first(ad->hops, (Jxta_object*) hop);
}
 
/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
const static Kwdtab Jxta_RouteAdvertisement_tags[] = {
{"Null",   Null_,   NULL,   NULL                                             },
{"jxta:RA",Jxta_RouteAdvertisement_,*handleJxta_RouteAdvertisement,jxta_RouteAdvertisement_get_Jxta_RouteAdvertisement_string},
{"Dst",    DEST_,    *handleDest,        NULL},
{"DstPID", DESTPID_, *handleDestPID,     jxta_RouteAdvertisement_get_DestPID_string},
{"Hops",   HOPS_,    *handleHops,        NULL},
{NULL,     0,                  0,                        NULL }
};

/* This being an internal call, we chose a behaviour for handling the jstring
 * that's better suited to our need: we are passed an existing jstring
 * and append to it.
 */

void 
hops_printer(Jxta_RouteAdvertisement * ad,
	     JString * js) { 

    int sz;
    int i;
    Jxta_vector* lists = ad->hops;
    JString* tmp;

    sz = jxta_vector_size(lists);

    for (i = 0; i < sz; ++i) {  
	Jxta_AccessPointAdvertisement* hop;

	jxta_vector_get_object_at(lists, (Jxta_object**) &hop, i);
	jxta_AccessPointAdvertisement_get_xml(hop, &tmp);
	jstring_append_2(js,"<Hops>");
	jstring_append_1(js, tmp);
	jstring_append_2(js,"</Hops>\n");
	JXTA_OBJECT_RELEASE(hop);
	JXTA_OBJECT_RELEASE(tmp);
    }
}
 
Jxta_status
jxta_RouteAdvertisement_get_xml(Jxta_RouteAdvertisement * ad,
                                        JString ** result)
{

   JString* string = jstring_new_0();
   JString* tmpref;
   Jxta_id* pid = NULL;
   JString* sid;

   jstring_append_2(string,"<jxta:RA xmlns:jxta=\"http://jxta.org\" type=\"jxta:RA\">\n");

   if (ad->dest != NULL) {
     pid = jxta_AccessPointAdvertisement_get_PID(ad->dest);
     if (pid != NULL) {
       jstring_append_2(string,"<DstPID>");
       jxta_id_to_jstring(pid, &sid);
       jstring_append_2(string, jstring_get_string(sid));
       jstring_append_2(string,"</DstPID>");
       JXTA_OBJECT_RELEASE(pid);
       JXTA_OBJECT_RELEASE(sid);
     }

     /*
      * FIXME 20040517 tra
      * Since we are sending the peer id in the previous tag
      * we should remove it in the Dst tag.
      *
      */
     jstring_append_2(string,"<Dst>");
     tmpref = NULL;
     jxta_AccessPointAdvertisement_get_xml(ad->dest, &tmpref);
     jstring_append_1 (string, tmpref);
     JXTA_OBJECT_RELEASE(tmpref);
     jstring_append_2(string,"</Dst>\n");
   }

   hops_printer(ad,string);

   jstring_append_2(string,"</jxta:RA>\n");

   *result = string;
   return JXTA_SUCCESS;
}

/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
Jxta_RouteAdvertisement *
jxta_RouteAdvertisement_new(void) {

  Jxta_RouteAdvertisement * ad;

  ad = (Jxta_RouteAdvertisement *) malloc (sizeof (Jxta_RouteAdvertisement));

  memset (ad, 0, sizeof (Jxta_RouteAdvertisement));

  jxta_advertisement_initialize((Jxta_advertisement*)ad,
				"jxta:RA",
				Jxta_RouteAdvertisement_tags,
                                (JxtaAdvertisementGetXMLFunc)jxta_RouteAdvertisement_get_xml,
				(JxtaAdvertisementGetIDFunc)jxta_RouteAdvertisement_get_DestPID, 
				(JxtaAdvertisementGetIndexFunc)jxta_RouteAdvertisement_get_indexes,
				(FreeFunc) jxta_RouteAdvertisement_delete);

  ad->dest   = NULL;
  ad->hops   = jxta_vector_new(0);

  return ad;
}

/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
static void
jxta_RouteAdvertisement_delete (Jxta_RouteAdvertisement * ad)
{
  if (ad->dest != NULL) {
    JXTA_OBJECT_RELEASE(ad->dest);
  }

  JXTA_OBJECT_RELEASE(ad->hops);

  jxta_advertisement_delete((Jxta_advertisement*)ad);

  memset (ad, 0xdd, sizeof (Jxta_RouteAdvertisement));
  free (ad);
}


void 
jxta_RouteAdvertisement_parse_charbuffer(
                             Jxta_RouteAdvertisement * ad,
				 const char * buf,
				 int len) {

  jxta_advertisement_parse_charbuffer((Jxta_advertisement*)ad,buf,len);
}

void 
jxta_RouteAdvertisement_parse_file(Jxta_RouteAdvertisement * ad,
				      FILE * stream) {

   jxta_advertisement_parse_file((Jxta_advertisement*)ad, stream);
}

Jxta_vector * 
jxta_RouteAdvertisement_get_indexes(void) {
        const char * idx[][2] = { 
				{ "DstPID", NULL },
        			{ NULL, NULL } 
				};
    return jxta_advertisement_return_indexes(idx);
}


#ifdef STANDALONE
int
main (int argc, char **argv) {
   Jxta_RouteAdvertisement * ad;
   FILE *testfile;

   if(argc != 2)
     {
       printf("usage: ad <filename>\n");
       return -1;
     }

   ad = jxta_RouteAdvertisement_new();

   testfile = fopen (argv[1], "r");
   jxta_RouteAdvertisement_parse_file(ad, testfile);
   fclose(testfile);

   jxta_RouteAdvertisement_print_xml(ad,fprintf,stdout);
   jxta_RouteAdvertisement_delete(ad);

   return 0;
}
#endif


#ifdef __cplusplus
}
#endif
