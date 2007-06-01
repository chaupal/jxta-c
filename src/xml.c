/**
 * test file for xml processing.
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




Jxta_boolean pa_test(int argc, char **argv)
{
    Jxta_PA *ad;
    FILE *testfile;
    JString *js;

    if (argc != 2) {

        printf("usage: ad <filename>\n");
        return FALSE;
    }

    ad = jxta_PA_new();
    testfile = fopen(argv[1], "r");
    jxta_PA_parse_file(ad, testfile);
    fclose(testfile);

    jxta_PA_get_xml(ad, &js);
    fprintf(stdout, "%s\n", jstring_get_string(js));

    JXTA_OBJECT_RELEASE(js);

    return FALSE;
}



/**
 * Stuff some *_new() functions into a hash table 
 * and see if the can be extracted.
 */
void jxta_advertisement_new_hash_test()
{

    Jxta_PA *ad = jxta_PA_new();

    /*
       jxta_advertisement_set_new_func("PA",jxta_PA_new);
     */
    jxta_advertisement_register_global_handler("PA", jxta_PA_new);
    jxta_advertisement_global_lookup((Jxta_advertisement *) ad, "PA");
    /*
       jxta_advertisement_get_new_func("PA");
     */

    /* Try parsing...
     */
}




int main(int argc, char **argv)
{

    Jxta_PA *pa = jxta_PA_new();
    JXTA_OBJECT_RELEASE(pa);

    jxta_advertisement_new_hash_test();

}
