
#include <stdio.h>
#include "jxta.h"

#include "jxta_pa.h"
#include "jxta_pga.h"
#include "jxta_mia.h"

int
main (int argc, char **argv) {

   Jxta_advertisement * ad;
   FILE *testfile;
   JString* dump;
   Jxta_vector* advs;

   if(argc != 2)
     {
       printf("usage: any_adv_test <pa, pga, or mia file>\n");
       printf("note: a file containing many of these enclosed in a document of"
	      " any name will work too (see many.xml)\n");
       return -1;
     }

   jxta_initialize();

   /*
    * register a few common doc types.
    * to enable testing.
    */

   
   jxta_advertisement_register_global_handler("jxta:PA", (void*) jxta_PA_new);
   jxta_advertisement_register_global_handler("jxta:PGA",(void*) jxta_PGA_new);
   jxta_advertisement_register_global_handler("jxta:MIA",(void*) jxta_MIA_new);

   ad = jxta_advertisement_new();

   testfile = fopen (argv[1], "r");
   jxta_advertisement_parse_file(ad, testfile);
   fclose(testfile);

   jxta_advertisement_get_advs(ad, &advs);
   if (advs == NULL) {
       printf("Abnormal, the vector can be empty, but not NULL\n");
   } else {
       int i;
       int sz = jxta_vector_size(advs);
       printf("parsed %d advertisements\n", sz);
       for (i = 0; i<sz; i++) {
	   Jxta_advertisement* adv;
	   jxta_vector_get_object_at(advs, (Jxta_object**) &adv, i);
	   jxta_advertisement_get_xml(adv, &dump);
	   printf("Found:\n%s\n\n", jstring_get_string(dump));
	   JXTA_OBJECT_RELEASE(dump);
	   JXTA_OBJECT_RELEASE(adv);
       }
       JXTA_OBJECT_RELEASE(advs);
   }

   JXTA_OBJECT_RELEASE(ad);

   jxta_terminate();

  return 0;

}
