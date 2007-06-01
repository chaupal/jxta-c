
#include <stdio.h>
#include "jxta.h"




int main(int argc, char **argv)
{

    Jxta_PGA *ad;
    FILE *testfile;
    JString *dump;

    jxta_initialize();

    if (argc != 2) {
        printf("usage: ad <filename>\n");
        return -1;
    }

    ad = jxta_PGA_new();

    testfile = fopen(argv[1], "r");
    jxta_PGA_parse_file(ad, testfile);
    fclose(testfile);

    jxta_PGA_get_xml(ad, &dump);
    printf("%s\n", jstring_get_string(dump));

    JXTA_OBJECT_RELEASE(ad);
    JXTA_OBJECT_RELEASE(dump);

    jxta_terminate();

    return 0;

}
