

#include <jxta.h>
#include <jxta_cm.h>
#include <jxta_cm_private.h>
#include <jxta_cache_config_adv.h>
#include <apr_general.h>




/**
 *  As of May 6, 2002, no mem leaks.
 */
int test_cm_new()
{
    Jxta_id *peerid;
    Jxta_cm *cm = NULL;

    Jxta_CacheConfigAdvertisement *cache_config;

    cache_config = jxta_CacheConfigAdvertisement_new();
    jxta_CacheConfigAdvertisement_create_default(cache_config, TRUE);

    jxta_id_peerid_new_1(&peerid, jxta_id_worldNetPeerGroupID);
    cm = cm_new("CM", jxta_id_worldNetPeerGroupID, cache_config, peerid, NULL);

    JXTA_OBJECT_RELEASE(cm);

    return 0;
}



int test_cm_create_folder()
{

    Jxta_id *peerid;
    Jxta_cm *cm = NULL;
    Jxta_CacheConfigAdvertisement *cache_config;
    const char *keys[][2] = {
        {"foo", NULL},
        {"bar", NULL},
        {"baz", NULL},
        {NULL, NULL}
    };

    jxta_id_peerid_new_1(&peerid, jxta_id_worldNetPeerGroupID);

    cache_config = jxta_CacheConfigAdvertisement_new();
    jxta_CacheConfigAdvertisement_create_default(cache_config, TRUE);

    cm = cm_new("CM", jxta_id_worldNetPeerGroupID, cache_config, peerid, NULL);


    cm_create_folder(cm, (char *) "MyFolder", keys);

    JXTA_OBJECT_RELEASE(cm);

    return 0;
}




int test_cm()
{

    test_cm_new();
    test_cm_create_folder();

    return 0;

}





int main(int argc, char **argv)
{
    jxta_initialize();
    test_cm();
    jxta_terminate();
    return 0;
}

/* vim: set ts=4 sw=4 tw=130 et: */
