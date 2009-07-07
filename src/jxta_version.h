#ifndef JXTA_VERSION_H
#define JXTA_VERSION_H

#include "jstring.h"
#include "jxta_object.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
};
#endif

#define CURRENT_MAJOR_VERSION 3
#define CURRENT_MINOR_VERSION 7 

#define SRDI_DELTA_OPTIMIZATIONS  "3.1"
#define PEERVIEW_UUID_IMPLEMENTATION "3.2"
#define LEASE_RESPONSE_WITH_PVID "3.3"
#define SRDI_DUPLICATE_ENTRIES "3.4"
#define DISCOVERY_EXT_QUERY_STATES "3.5"
#define DISCOVERY_EXT_QUERY_WALK_STATE "3.6"
#define EXPLICIT_DISCONNECT "3.7"
#define SRDI_UPDATE_ONLY "3.8"

typedef struct _jxta_version Jxta_version;

JXTA_DECLARE(Jxta_version *) jxta_version_new(void);

JXTA_DECLARE(Jxta_boolean) jxta_version_compatible(Jxta_version *, Jxta_version *);

JXTA_DECLARE(Jxta_boolean) jxta_version_compatible_1(char *, Jxta_version *);

JXTA_DECLARE(Jxta_version*) jxta_version_get_current_version(void);

JXTA_DECLARE(int) jxta_version_get_major_version(Jxta_version *);

JXTA_DECLARE(void) jxta_version_set_major_version(Jxta_version *, int);

JXTA_DECLARE(int) jxta_version_get_minor_version(Jxta_version *);

JXTA_DECLARE(void) jxta_version_set_minor_version(Jxta_version *, int);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_VERSION_H */
