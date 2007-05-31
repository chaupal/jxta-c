
#include <stdio.h>
#include "jxta_srdi.h"
#include "jxta_id.h"

boolean
srdi_ttl_test(Jxta_SRDIMessage * ad) {

    boolean passed = FALSE;

    int ttl = jxta_srdi_message_get_ttl(ad);
    if (ttl == 10) {
        fprintf(stdout,"srdi_ttl_test TTL=%d, test passed\n", ttl);
        passed = TRUE;
    } else {
        fprintf(stderr,"srdi_ttl_test Failed\n");
    }
    return passed;
}
boolean
srdi_get_primaryKey_test(Jxta_SRDIMessage * ad) {

    boolean passed = TRUE;
    JString * pkey = jstring_new_0();
    jxta_srdi_message_get_primaryKey(ad, &pkey);
    fprintf(stdout,"Primary Key = %s\n",jstring_get_string(pkey));
    return passed;
}

boolean
srdi_get_peerid_test(Jxta_SRDIMessage * ad) {

    boolean passed = TRUE;
    JString * tmps = NULL;
    Jxta_id     * peerid = jxta_id_nullID;

    jxta_srdi_message_get_peerID(ad, &peerid);
    jxta_id_to_jstring(peerid, &tmps);
    fprintf(stdout,"PeerID = %s\n",jstring_get_string(tmps));
    return passed;
}

boolean
srdi_entry_test(Jxta_SRDIMessage * ad) {
    Jxta_vector* entries;
    int i;
    int num;
    jxta_srdi_message_get_entries(ad, &entries);
    num = jxta_vector_size(entries);
    printf("Message contains %d entries\n",num);
    for(i=0; i<num; i++) {
        Jxta_SRDIEntryElement* entry;
        jxta_vector_get_object_at(entries, (Jxta_object**) &entry, i);
        printf("Exp[%d]: %d\n", i, entry->expiration);
        printf("key[%d]: %s\n", i, jstring_get_string(entry->key));
        printf("value[%d]: %s\n", i, jstring_get_string(entry->value));
        JXTA_OBJECT_RELEASE(entry);
    }
    JXTA_OBJECT_RELEASE(entries);
    return TRUE;
}

int main (int argc, char **argv) {
    int retval;
    jxta_initialize();
    Jxta_SRDIMessage * ad = jxta_srdi_message_new();
    FILE *testfile;

    testfile = fopen ("srdi.xml", "r");
    jxta_srdi_message_parse_file(ad, testfile);
    fclose(testfile);

    retval = srdi_ttl_test(ad);
    if (!retval) {
        printf("test failed srdi_ttl_test\n");
        return -1;
    }
    retval = srdi_get_primaryKey_test(ad);
    if (!retval) {
        printf("test failed srdi_get_primaryKey_test\n");
        return -1;
    }
    retval = srdi_get_peerid_test(ad);
    if (!retval) {
        printf("test failed srdi_get_peerid_test\n");
        return -1;
    }

    retval = srdi_entry_test(ad);
    if (!retval) {
        printf("test failed srdi_entry_test\n");
        return -1;
    }
    jxta_terminate();
    return 0;
}
