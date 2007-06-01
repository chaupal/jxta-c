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
 * $Id: jxta_log.c,v 1.30 2006/05/27 00:12:07 slowhog Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "jxta_apr.h"
#include "jxta_errno.h"

#include "jxta_log.h"
#include "jxta_log_priv.h"


static apr_pool_t *_jxta_log_pool = NULL;

#ifndef JXTA_LOG_CAT_GROWTH_RATE
#define JXTA_LOG_CAT_GROWTH_RATE 5
#endif

static Jxta_log_callback _jxta_log_func = NULL;
static void *_jxta_log_user_data = NULL;
static unsigned int _jxta_log_initialized = 0;
static Jxta_log_selector *_jxta_log_default_selector;

static char const *const _jxta_log_level_labels[JXTA_LOG_LEVEL_MAX] = {
    "fatal",
    "error",
    "warning",
    "info",
    "debug",
    "trace",
    "paranoid"
};

JXTA_DECLARE(Jxta_status) jxta_log_initialize()
{
    Jxta_status rv;

    if (_jxta_log_initialized++) {
        return JXTA_SUCCESS;
    }

    rv = apr_pool_initialize();
    if (APR_SUCCESS == rv) {
        rv = apr_pool_create(&_jxta_log_pool, NULL);
    }

    if (APR_SUCCESS != rv) {
        _jxta_log_initialized = 0;
        return rv;
    }

    _jxta_log_default_selector = jxta_log_selector_new_and_set("*.warning-fatal", &rv);

    if (NULL == _jxta_log_default_selector) {
        apr_pool_destroy(_jxta_log_pool);
        _jxta_log_initialized = 0;
        return rv;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(void) jxta_log_terminate()
{
    if (!_jxta_log_initialized) {
        return;
    }

    _jxta_log_initialized--;
    if (_jxta_log_initialized) {
        return;
    }

    jxta_log_using(NULL, NULL);
    jxta_log_selector_delete(_jxta_log_default_selector);
    apr_pool_destroy(_jxta_log_pool);
    apr_pool_terminate();
    _jxta_log_pool = NULL;
}

JXTA_DECLARE(void) jxta_log_using(Jxta_log_callback log_cb, void *user_data)
{
    _jxta_log_func = log_cb;
    _jxta_log_user_data = user_data;
}

JXTA_DECLARE(Jxta_status) jxta_log_append(const char *cat, int level, const char *fmt, ...)
{
    Jxta_status rv;
    va_list ap;
    char *message;
    int length;
    
    if (NULL == _jxta_log_func) {
        return JXTA_SUCCESS;
    }

    va_start(ap, fmt);
    length = apr_vsnprintf(NULL, 0, fmt, ap) + 1;
    message = malloc(sizeof(char) * length);
    if (message == NULL) {
       return JXTA_NOMEM;
    }
    va_end(ap);
    va_start(ap, fmt);
    apr_vsnprintf(message, length, fmt, ap);
    va_end(ap);

    rv = (*_jxta_log_func) (_jxta_log_user_data, cat, level, message);
    free(message);

    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_log_appendv(const char *cat, int level, const char *fmt, va_list ap)
{
    Jxta_status rv = JXTA_SUCCESS;
    char *message;
    int length;
    va_list ap2;
    
#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)) || defined(va_copy)
    va_copy(ap2, ap);
#elif defined(__va_copy)
    __va_copy(ap2, ap);
#else
    memcpy(&ap2, &ap, sizeof(va_list));
#endif

    if (NULL != _jxta_log_func) {
        length = apr_vsnprintf(NULL, 0, fmt, ap) + 1;
        message = malloc(sizeof(char) * length);
        if (message == NULL) {
           return JXTA_NOMEM;
        }
        apr_vsnprintf(message, length, fmt, ap2);
        va_end(ap2);

        rv = (*_jxta_log_func) (_jxta_log_user_data, cat, level, message);

        free(message);
    }

    return rv;
}


/***********************************************************************
 * Jxta_log_selector implementation
 **********************************************************************/

static Jxta_status decode_level(const char *level, size_t size, Jxta_log_level * rv)
{
    int i;

    for (i = JXTA_LOG_LEVEL_MIN; i < JXTA_LOG_LEVEL_MAX; i++) {
        if ('\0' != _jxta_log_level_labels[i][size])
            continue;
        if (!strncasecmp(_jxta_log_level_labels[i], level, size)) {
            break;
        }
    }

    *rv = i;
    return (JXTA_LOG_LEVEL_MAX == i) ? JXTA_INVALID_ARGUMENT : JXTA_SUCCESS;
}

static void free_categories(Jxta_log_selector * self)
{
    size_t i;

    if (NULL == self->categories)
        return;

    for (i = 0; i < self->category_count; i++) {
        free((void *) self->categories[i]);
    }
    free((void *) self->categories);

    self->categories = NULL;
    self->category_size = 0;
    self->category_count = 0;
}

static int locate_category(Jxta_log_selector * self, const char *cat, size_t size)
{
    size_t i;

    if (0 == size) {
        for (i = 0; i < self->category_count; i++) {
            if (!strcmp(cat, self->categories[i]))
                break;
        }
    } else {
        for (i = 0; i < self->category_count; i++) {
            if (!strncmp(cat, self->categories[i], size)) {
                if ('\0' == self->categories[i][size])
                    break;
            }
        }
    }
    return i;
}

static Jxta_status parse_level(Jxta_log_selector * self, const char *level, size_t size)
{
    unsigned int revert_flag = 0;
    const char *p = NULL;
    const char *e = level + size;
    unsigned int flags = JXTA_LOG_LEVEL_MASK_NONE;
    Jxta_log_level lb, ub;
    unsigned int in_range;
    size_t s;
    Jxta_status rv;

    if (NULL == self) {
        return JXTA_INVALID_ARGUMENT;
    }

    for (p = level; p < e && isspace(*p); ++p);
    if (p >= e) {
        return JXTA_INVALID_ARGUMENT;
    }

    if ('*' == *p) {
        for (++p; p < e && isspace(*p); ++p);
        if (p < e) {
            return JXTA_INVALID_ARGUMENT;
        }
        self->level_flags = JXTA_LOG_LEVEL_MASK_ALL;
        return JXTA_SUCCESS;
    }

    if ('!' == *p) {
        revert_flag = 1;
        for (++p; p < e && isspace(*p); ++p);
    }

    in_range = 0;
    lb = ub = JXTA_LOG_LEVEL_INVALID;

    for (level = p; p < e; level = p) {
        while (p < e && ',' != *p && '-' != *p && !isspace(*p))
            ++p;

        s = p - level;
        while (p < e && isspace(*p))
            ++p;

        rv = decode_level(level, s, &ub);
        if (JXTA_SUCCESS != rv)
            return rv;

        if (p >= e || ',' == *p) {
            for (++p; p < e && isspace(*p); ++p);
        } else if ('-' == *p) {
            if (in_range)
                return JXTA_INVALID_ARGUMENT;
            in_range = 1;
            lb = ub;
            for (++p; p < e && isspace(*p); ++p);
            continue;
        }

        /* boundary level determined */
        if (!in_range) {
            flags |= 1 << ub;
        } else {
            if (lb > ub) {
                lb ^= ub;
                ub ^= lb;
                lb ^= ub;
            }
            for (; lb <= ub; ++lb) {
                flags |= 1 << lb;
            }
            in_range = 0;
        }
    }

    if (in_range)
        return JXTA_INVALID_ARGUMENT;

    if (revert_flag)
        flags = ~flags;
    self->level_flags = flags & JXTA_LOG_LEVEL_MASK_ALL;

    return JXTA_SUCCESS;
}

static Jxta_status add_category(Jxta_log_selector * self, const char *cat)
{
    int i;
    void *p = NULL;

    if (self->category_size <= self->category_count) {
        i = self->category_size + JXTA_LOG_CAT_GROWTH_RATE;
        p = realloc((void *) self->categories, i * sizeof(char *));
        if (NULL == p) {
            return JXTA_NOMEM;
        }
        self->category_size = i;
        self->categories = p;
    }

    self->categories[self->category_count] = cat;
    self->category_count++;

    return JXTA_SUCCESS;
}

static Jxta_status parse_categories(Jxta_log_selector * self, const char *cat, size_t size)
{
    const char *p = NULL;
    const char *e = cat + size;
    const char *q;
    char *r;
    Jxta_status rv;

    if (NULL == self) {
        return JXTA_INVALID_ARGUMENT;
    }

    for (p = cat; p < e && isspace(*p); ++p);
    if (p >= e) {
        return JXTA_INVALID_ARGUMENT;
    }

    if ('*' == *p) {
        for (++p; p < e && isspace(*p); ++p);
        if (p < e) {
            return JXTA_INVALID_ARGUMENT;
        }
        free_categories(self);
        self->negative_category_list = 1;
        return JXTA_SUCCESS;
    }

    if ('!' == *p) {
        self->negative_category_list = 1;
        for (++p; p < e && isspace(*p); ++p);
    } else {
        self->negative_category_list = 0;
    }

    for (cat = p; p < e; cat = p) {
        while (p < e && ',' != *p)
            ++p;
        for (q = p - 1; isspace(*q); --q);

        size = q - cat + 1;
        r = calloc(size + 1, sizeof(char));
        if (r == NULL) {
            return JXTA_NOMEM;
        }
        strncpy(r, cat, size);
        rv = add_category(self, r);
        if (JXTA_SUCCESS != rv) {
            free(r);
            return rv;
        }

        if (',' == *p) {
            ++p;
            while (p < e && isspace(*p))
                ++p;
        }
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_log_selector *)
    jxta_log_selector_new()
{
    Jxta_log_selector *self;
    Jpr_status rv;

    self = calloc(1, sizeof(Jxta_log_selector));

    if (NULL != self) {
        rv = jpr_thread_mutex_create(&self->mutex, JPR_THREAD_MUTEX_NESTED, _jxta_log_pool);
        if (JPR_SUCCESS != rv) {
            free(self);
            self = NULL;
        }
    }

    return self;
}

JXTA_DECLARE(Jxta_log_selector *)
    jxta_log_selector_new_and_set(const char *selector, Jxta_status * rv)
{
    Jxta_log_selector *self;

    self = jxta_log_selector_new();
    if (NULL == self) {
        *rv = JXTA_NOMEM;
        return self;
    }

    *rv = jxta_log_selector_set(self, selector);
    return self;
}

JXTA_DECLARE(void)
    jxta_log_selector_delete(Jxta_log_selector * self)
{
    if (NULL == self) {
        return;
    }

    jpr_thread_mutex_lock(self->mutex);
    free_categories(self);
    jpr_thread_mutex_unlock(self->mutex);
    jpr_thread_mutex_destroy(self->mutex);
    free(self);
}

JXTA_DECLARE(Jxta_status)
    jxta_log_selector_add_category(Jxta_log_selector * self, const char *cat)
{
    unsigned int i;
    void *p = NULL;
    Jxta_status rv;

    if (NULL == self) {
        return JXTA_INVALID_ARGUMENT;
    }

    jpr_thread_mutex_lock(self->mutex);

    i = locate_category(self, cat, 0);
    if (i < self->category_count) {
        jpr_thread_mutex_unlock(self->mutex);
        return JXTA_SUCCESS;
    }

    p = calloc(strlen(cat) + 1, sizeof(char));
    if (NULL == p) {
        jpr_thread_mutex_unlock(self->mutex);
        return JXTA_NOMEM;
    }

    strcpy(p, cat);
    rv = add_category(self, p);
    if (JXTA_SUCCESS != rv) {
        free(p);
        jpr_thread_mutex_unlock(self->mutex);
        return rv;
    }

    jpr_thread_mutex_unlock(self->mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status)
    jxta_log_selector_remove_category(Jxta_log_selector * self, const char *cat)
{
    unsigned int i;

    if (NULL == self) {
        return JXTA_INVALID_ARGUMENT;
    }

    jpr_thread_mutex_lock(self->mutex);
    i = locate_category(self, cat, 0);

    if (i == self->category_count) {
        jpr_thread_mutex_unlock(self->mutex);
        return JXTA_SUCCESS;
    }

    free((void *) self->categories[i]);
    self->category_count--;
    for (; i < self->category_count; i++) {
        self->categories[i] = self->categories[i + 1];
    }
    self->categories[i] = NULL;

    jpr_thread_mutex_unlock(self->mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status)
    jxta_log_selector_list_positive(Jxta_log_selector * self, Jxta_boolean positive)
{
    if (NULL == self) {
        return JXTA_INVALID_ARGUMENT;
    }

    jpr_thread_mutex_lock(self->mutex);
    self->negative_category_list = (TRUE == positive) ? 0 : 1;
    jpr_thread_mutex_unlock(self->mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status)
    jxta_log_selector_set_level_mask(Jxta_log_selector * self, unsigned int mask)
{
    if (NULL == self) {
        return JXTA_INVALID_ARGUMENT;
    }

    jpr_thread_mutex_lock(self->mutex);
    self->level_flags |= mask;
    jpr_thread_mutex_unlock(self->mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status)
    jxta_log_selector_clear_level_mask(Jxta_log_selector * self, unsigned int mask)
{
    if (NULL == self) {
        return JXTA_INVALID_ARGUMENT;
    }

    jpr_thread_mutex_lock(self->mutex);
    self->level_flags &= ~mask;
    jpr_thread_mutex_unlock(self->mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status)
    jxta_log_selector_set(Jxta_log_selector * self, const char *selector)
{
    Jxta_status rv;
    const char *cat = NULL;
    const char *level = NULL;
    const char *p = NULL;
    size_t len_cat = 0;
    size_t len_level = 0;

    if (NULL == selector) {
        return JXTA_INVALID_ARGUMENT;
    }

    for (p = selector; isspace(*p); ++p);
    if ('\0' == *p || '.' == *p) {
        return JXTA_INVALID_ARGUMENT;
    }
    cat = p;

    for (++p; '\0' != *p && '.' != *p; ++p);
    if ('\0' == *p) {
        return JXTA_INVALID_ARGUMENT;
    }
    len_cat = p - cat;

    level = ++p;
    for (; '\0' != *p && '.' != *p; ++p);
    if ('.' == *p) {
        return JXTA_INVALID_ARGUMENT;
    }

    len_level = p - level;
    if (0 == len_level) {
        return JXTA_INVALID_ARGUMENT;
    }

    jpr_thread_mutex_lock(self->mutex);

    free_categories(self);
    rv = parse_categories(self, cat, len_cat);
    if (JXTA_SUCCESS != rv) {
        jpr_thread_mutex_unlock(self->mutex);
        return rv;
    }

    rv = parse_level(self, level, len_level);
    jpr_thread_mutex_unlock(self->mutex);
    return rv;
}

JXTA_DECLARE(Jxta_boolean)
    jxta_log_selector_is_selected(Jxta_log_selector * self, const char *cat, Jxta_log_level level)
{
    Jxta_boolean rv;

    if (NULL == self) {
        return FALSE;
    }

    if (level < JXTA_LOG_LEVEL_MIN || level >= JXTA_LOG_LEVEL_MAX) {
        return FALSE;
    }

    jpr_thread_mutex_lock(self->mutex);

    if (0 == (self->level_flags & (1 << level))) {
        jpr_thread_mutex_unlock(self->mutex);
        return FALSE;
    }

    rv = ((size_t) (locate_category(self, cat, 0)) == self->category_count) ?
        ((self->negative_category_list) ? TRUE : FALSE) : ((self->negative_category_list) ? FALSE : TRUE);

    jpr_thread_mutex_unlock(self->mutex);
    return rv;
}

/***********************************************************************
 * Jxta_log_file implementation
 **********************************************************************/

JXTA_DECLARE(Jxta_status)
    jxta_log_file_open(Jxta_log_file ** newf, const char *fname)
{
    Jxta_log_file *self;
    Jxta_status rv;

    self = calloc(1, sizeof(Jxta_log_file));
    if (NULL == self) {
        *newf = NULL;
        return JXTA_NOMEM;
    }

    rv = jpr_thread_mutex_create(&self->mutex, JPR_THREAD_MUTEX_NESTED, _jxta_log_pool);
    if (JPR_SUCCESS != rv) {
        free(self);
        self = NULL;
        *newf = NULL;
        return rv;  /* really bad, should I cover it up with JXTA_FILED? */
    }

    if (!strcmp(fname, "-")) {
        self->thefile = stdout;
    } else {
        self->thefile = fopen(fname, "a+");
        if (NULL == self->thefile) {
            free(self);
            *newf = NULL;
            return JXTA_FAILED;
        }
    }

    *newf = self;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status)
    jxta_log_file_close(Jxta_log_file * self)
{
    if (NULL == self)
        return JXTA_INVALID_ARGUMENT;
    if (self == _jxta_log_user_data) {
        jxta_log_using(NULL, NULL);
    }

    jpr_thread_mutex_lock(self->mutex);

    if (NULL != self->thefile && stdout != self->thefile) {
        fclose(self->thefile);
    }

    jpr_thread_mutex_unlock(self->mutex);
    jpr_thread_mutex_destroy(self->mutex);
    free(self);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status)
    jxta_log_file_attach_selector(Jxta_log_file * self, Jxta_log_selector * selector, Jxta_log_selector ** orig_sel)
{
    if (NULL == self)
        return JXTA_INVALID_ARGUMENT;

    jpr_thread_mutex_lock(self->mutex);

    if (NULL != orig_sel) {
        *orig_sel = self->selector;
    }
    self->selector = selector;

    jpr_thread_mutex_unlock(self->mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status)
    jxta_log_file_append(void *user_data, const char *cat, int level, const char *msg)
{
    Jxta_log_file *self;
    Jxta_log_selector *sel;
    Jxta_status rv = JXTA_SUCCESS;
    apr_time_exp_t tm;
    char tm_str[16];
    apr_size_t tm_str_sz;

    if (NULL == user_data)
        return JXTA_INVALID_ARGUMENT;

    self = (Jxta_log_file *) user_data;

    if (NULL == self->thefile)
        return JXTA_INVALID_ARGUMENT;
    if (NULL == self->mutex)
        return JXTA_INVALID_ARGUMENT;

    jpr_thread_mutex_lock(self->mutex);

    sel = (NULL == self->selector) ? _jxta_log_default_selector : self->selector;

    if (!jxta_log_selector_is_selected(sel, cat, level)) {
        jpr_thread_mutex_unlock(self->mutex);
        return JXTA_SUCCESS;
    }

    apr_time_exp_lt(&tm, apr_time_now());
    apr_strftime(tm_str, &tm_str_sz, 32, "%m/%d %H:%M:%S", &tm);
#ifdef WIN32
    fprintf(self->thefile, "[%s]-%s-[%s:%d][TID: %u] - %s", cat, _jxta_log_level_labels[level], tm_str, tm.tm_usec,
            GetCurrentThreadId(), msg);
#else
    fprintf(self->thefile, "[%s]-%s-[%s:%d][TID: %p] - %s", cat, _jxta_log_level_labels[level], tm_str, tm.tm_usec,
            apr_os_thread_current(), msg);
#endif
    fflush(self->thefile);

    jpr_thread_mutex_unlock(self->mutex);
    return rv;
}

/* vi: set sw=4 ts=4 tw=130 et: */
