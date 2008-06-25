static const char *const __log_cat = "VERSION";


#include "jxta_log.h"
#include "jxta_errno.h"
#include "jxta_version.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define CURRENT_MAJOR_VERSION 3
#define CURRENT_MINOR_VERSION 1

struct _jxta_version {
    JXTA_OBJECT_HANDLE;
	unsigned int protocol_major_version;
    unsigned int protocol_minor_version;
};


/**
 * Forw decl of unexported function.
 */
static void version_delete(Jxta_object *);

JXTA_DECLARE(Jxta_version *) jxta_version_new()
{
    Jxta_version *version = (Jxta_version *) calloc(1, sizeof(Jxta_version));
    if (NULL == version) {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_version NEW [%pp]\n", version);

    JXTA_OBJECT_INIT(version, version_delete, NULL);

    version->protocol_major_version = 0;
	version->protocol_minor_version = 0;
    return version;
}

static void version_delete(Jxta_object * me)
{
    Jxta_version *version = (Jxta_version *) me;

    memset(version, 0xdd, sizeof(Jxta_version));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_version FREE [%pp]\n", version);

    free(version);
}

/*
*	Is version2 at least to the level of version1
*/
JXTA_DECLARE(Jxta_boolean) jxta_version_compatible(Jxta_version * version1, Jxta_version * version2)
{
    if (version1->protocol_major_version != version2->protocol_major_version) {
        return JXTA_FALSE;
    } else if (version1->protocol_minor_version > version2->protocol_minor_version) {
        return JXTA_FALSE;
    } else {
        return JXTA_TRUE;
    }
}

/*
*	Is version at least to the level of versionString
*/
JXTA_DECLARE(Jxta_boolean) jxta_version_compatible_1(char *versionString, Jxta_version * version)
{
	unsigned int majorVersion = atoi(versionString);
	unsigned int minorVersion;

	char * minorPointer = strstr(versionString, ".");
	if(minorPointer == NULL) {
		return JXTA_FALSE;
	}
	minorVersion = atoi(&(minorPointer[1]));

	if(majorVersion != version->protocol_major_version) {
		return JXTA_FALSE;
	}
	else if (minorVersion > version->protocol_minor_version){
		return JXTA_FALSE;
	}
	else {
		return JXTA_TRUE;
	}
}

JXTA_DECLARE(int) jxta_version_get_major_version(Jxta_version *ver)
{
	return ver->protocol_major_version;
}

JXTA_DECLARE(int) jxta_version_get_minor_version(Jxta_version *ver)
{
	return ver->protocol_minor_version;
}

JXTA_DECLARE(void) jxta_version_set_major_version(Jxta_version *ver, int major_version)
{
	ver->protocol_major_version = major_version;
}

JXTA_DECLARE(void) jxta_version_set_minor_version(Jxta_version *ver, int minor_version)
{
	ver->protocol_minor_version = minor_version;
}

JXTA_DECLARE(Jxta_version*) jxta_version_get_current_version()
{
	Jxta_version * newVersion = jxta_version_new();
	newVersion->protocol_major_version = CURRENT_MAJOR_VERSION ;
	newVersion->protocol_minor_version = CURRENT_MINOR_VERSION;
	return newVersion; 
}

