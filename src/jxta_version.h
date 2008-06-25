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
