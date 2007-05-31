

#include <jxta.h>
#include <jxta_cm.h>
#include <apr_general.h>




/**
 *  As of May 6, 2002, no mem leaks.
 */
int 
test_cm_new() {

  Jxta_cm * cm = jxta_cm_new("CM", jxta_id_worldNetPeerGroupID);

  JXTA_OBJECT_RELEASE(cm);

  return 0;
}



int
test_cm_create_folder() {

  Jxta_cm * cm = jxta_cm_new("CM", jxta_id_worldNetPeerGroupID);
  char * keys[] = {"foo", "bar", "baz", NULL};

  jxta_cm_create_folder(cm,"MyFolder",keys);

  JXTA_OBJECT_RELEASE(cm);

  return 0;
}




int
test_cm() {

  test_cm_new();
  test_cm_create_folder();

  return 0;

}





int
main(int argc, char ** argv) {

#ifdef WIN32 
    apr_app_initialize(&argc, &argv, NULL);
#else
    apr_initialize();
#endif

  test_cm();

  //apr_terminate();

  return 0;
}



