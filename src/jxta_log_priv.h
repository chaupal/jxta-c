/*
 * Copyright (c) 2004 Henry Jen. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __JXTA_LOG_PRIV_H__
#define __JXTA_LOG_PRIV_H__

#include <stdio.h>

#include "jpr/jpr_apr_wrapper.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

struct Jxta_log_selector {
    size_t category_count;
    size_t category_size;
    const char **categories;
    unsigned int negative_category_list;
    unsigned int level_flags;
#if JPR_HAS_THREADS
    jpr_thread_mutex_t *mutex;
#endif
};

struct Jxta_log_file {
    FILE *thefile;
    Jxta_log_selector *selector;
#if JPR_HAS_THREADS
    jpr_thread_mutex_t *mutex;
#endif
};

#ifdef __cplusplus
#if 0
{
#endif
}
#endif /* extern "C" */

#endif /* __JXTA_LOG_PRIV_H__ */

/* vi: set ts=4 sw=4 tw=130 et: */
