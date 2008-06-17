/**
 * xmltest is for working out details of xml parsing.
 */


#include "jxta.h"



int main(int argc, char **argv)
{

    jxta_initialize();

    jxta_advertisement_register_global_handler("foo", NULL);

    jxta_terminate();

    return 0;
}
