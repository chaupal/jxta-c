

#include <jxta.h>
#include <jxta_cm.h>
#include <apr_general.h>




/**
 *  As of May 6, 2002, no mem leaks.
 */
int 
test_cm_new() {
  Jxta_id *peerid;
  Jxta_cm * cm =NULL;

  jxta_id_peerid_new_1(&peerid, jxta_id_worldNetPeerGroupID);
  cm = jxta_cm_new("CM", jxta_id_worldNetPeerGroupID, peerid);

  JXTA_OBJECT_RELEASE(cm);

  return 0;
}



int
test_cm_create_folder() {

  Jxta_id *peerid;
  Jxta_cm * cm = NULL;
  char * keys[][2] = { 
                 { "foo", NULL },
                 {  "bar", NULL },
                 {  "baz", NULL },
                 {  NULL, NULL}
                 };

  jxta_id_peerid_new_1(&peerid, jxta_id_worldNetPeerGroupID);

  cm = jxta_cm_new("CM", jxta_id_worldNetPeerGroupID, peerid);
 

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
    jxta_initialize();
    test_cm();
    jxta_terminate();
    return 0;
}

/* vim: set ts=4 sw=4 tw=130 et: */

