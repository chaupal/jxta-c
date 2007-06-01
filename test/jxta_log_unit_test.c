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

#include "jxta_log.h"
#include "jxta_log_priv.h"

JXTA_DECLARE(Jxta_log_selector *) jxta_log_selector_new_and_set(const char *selector, Jxta_status * rv);

JXTA_DECLARE(Jxta_boolean) jxta_log_selector_is_selected(Jxta_log_selector * self, const char *cat, Jxta_log_level level);

#include <stdio.h>

#define TEST_SEL(s, x, y, line) \
    if (FALSE == jxta_log_selector_is_selected(s, x, y)) \
        printf("Failed at line %u\n", line);

#define TEST_NOSEL(s, x, y, line) \
    if (TRUE == jxta_log_selector_is_selected(s, x, y)) \
        printf("Failed at line %u\n", line);

static const char *_log_level_labels[JXTA_LOG_LEVEL_MAX] = {
    "fatal",
    "error",
    "warning",
    "info",
    "debug",
    "trace"
};

void dump_sel(Jxta_log_selector * s)
{
    unsigned int i;

    printf("-------- Selector status -------\n");
    printf("Categories: %s\n", (s->negative_category_list) ? "NEG" : "POS");
    for (i = 0; i < s->category_count; ++i) {
        printf("\t%s\n", s->categories[i]);
    }
    printf("Mask: 0x%X\n\t", s->level_flags);
    for (i = JXTA_LOG_LEVEL_MIN; i < JXTA_LOG_LEVEL_MAX; i++) {
        if (0 != (s->level_flags & (1 << i)))
            printf(" %s", _log_level_labels[i]);
    }
    printf("\n--------------------------------\n");
}

int main(int argc, char **argv)
{
    Jxta_log_selector *s;
    Jxta_log_file *f;
    Jxta_status rv;

    rv = jxta_log_initialize();
    if (JXTA_SUCCESS != rv)
        return -1;

    s = jxta_log_selector_new_and_set("a aa, b , ccc,dddd ,ee, ff.fatal - warning,debug, warning- error info , trace", &rv);
    if (JXTA_SUCCESS != rv) {
        printf("Failed at line %u\n", __LINE__);
    }

    dump_sel(s);

    TEST_NOSEL(s, "x", JXTA_LOG_LEVEL_WARNING, __LINE__);
    TEST_SEL(s, "dddd", JXTA_LOG_LEVEL_TRACE, __LINE__);

    jxta_log_selector_set(s, "*.!info, trace");
    dump_sel(s);
    TEST_SEL(s, "x", JXTA_LOG_LEVEL_WARNING, __LINE__);
    TEST_NOSEL(s, "dddd", JXTA_LOG_LEVEL_TRACE, __LINE__);

    jxta_log_selector_set(s, "! dddd.info");
    dump_sel(s);
    TEST_SEL(s, "x", JXTA_LOG_LEVEL_INFO, __LINE__);
    TEST_NOSEL(s, "dddd", JXTA_LOG_LEVEL_INFO, __LINE__);

    rv = jxta_log_file_open(&f, "jprlog.log");

    if (JXTA_SUCCESS != rv)
        return -1;
    jxta_log_file_attach_selector(f, s, NULL);
    jxta_log_using(jxta_log_file_append, f);

    jxta_log_append("transport", JXTA_LOG_LEVEL_INFO, "Line %d should be logged.\n", __LINE__);
    jxta_log_append("dddd", JXTA_LOG_LEVEL_INFO, "Line %d should not be logged.\n", __LINE__);
    jxta_log_terminate();
    jxta_log_file_close(f);

    return 0;
}

/* vi: set sw=4 ts=4 et: */
