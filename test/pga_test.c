
#include <stdio.h>
#include "jxta.h"




int main(int argc, char **argv)
{

    PGA *ad;
    FILE *testfile;

    if (argc != 2) {
        printf("usage: ad <filename>\n");
        return -1;
    }

    ad = PGA_new();

    testfile = fopen(argv[1], "r");
    PGA_parse_file(ad, testfile);
    fclose(testfile);

    PGA_print_xml(ad, (PrintFunc) fprintf, stdout);
    PGA_delete(ad);


    return 0;

}
