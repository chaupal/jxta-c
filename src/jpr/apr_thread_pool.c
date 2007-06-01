/* Copyright 2006 Sun Microsystems, Inc.
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
 */

#include <assert.h>
#include "apr_thread_pool.h"
#include "apr_ring.h"
#include "apr_thread_cond.h"

#if APR_HAS_THREADS

#define TASK_PRIORITY_SEGS 4
#define TASK_PRIORITY_SEG(x) (((x)->priority & 0xFF) / 64)

typedef struct apr_thread_pool_task
{
    APR_RING_ENTRY(apr_thread_pool_task) link;
    apr_thread_start_t func;
    void *param;
    apr_byte_t priority;
} apr_thread_pool_task_t;

APR_RING_HEAD(apr_thread_pool_tasks, apr_thread_pool_task);

struct apr_thread_list_elt
{
    APR_RING_ENTRY(apr_thread_list_elt) link;
    apr_thread_t *thd;
    volatile int stop;
};

APR_RING_HEAD(apr_thread_list, apr_thread_list_elt);

struct apr_thread_pool
{
    apr_pool_t *pool;
    volatile apr_size_t thd_max;
    volatile apr_size_t idle_max;
    volatile apr_size_t thd_cnt;
    volatile apr_size_t idle_cnt;
    volatile apr_size_t task_cnt;
    volatile apr_size_t threshold;
    struct apr_thread_pool_tasks *tasks;
    struct apr_thread_list *busy_thds;
    struct apr_thread_list *idle_thds;
    apr_thread_mutex_t *lock;
    apr_thread_mutex_t *cond_lock;
    apr_thread_cond_t *cond;
    volatile int terminated;
    struct apr_thread_pool_tasks *recycled_tasks;
    apr_thread_pool_task_t *task_idx[TASK_PRIORITY_SEGS];
};

static apr_status_t thread_pool_construct(apr_thread_pool_t * me,
                                          apr_size_t init_threads,
                                          apr_size_t max_threads)
{
    apr_status_t rv;
    int i;

    me->thd_max = max_threads;
    me->idle_max = init_threads;
    me->threshold = init_threads / 2;
    rv = apr_thread_mutex_create(&me->lock, APR_THREAD_MUTEX_NESTED,
                                 me->pool);
    if (APR_SUCCESS != rv) {
        return rv;
    }
    rv = apr_thread_mutex_create(&me->cond_lock, APR_THREAD_MUTEX_UNNESTED,
                                 me->pool);
    if (APR_SUCCESS != rv) {
        apr_thread_mutex_destroy(me->lock);
        return rv;
    }
    rv = apr_thread_cond_create(&me->cond, me->pool);
    if (APR_SUCCESS != rv) {
        apr_thread_mutex_destroy(me->lock);
        apr_thread_mutex_destroy(me->cond_lock);
        return rv;
    }
    me->tasks = apr_palloc(me->pool, sizeof(*me->tasks));
    if (!me->tasks) {
        goto CATCH_ENOMEM;
    }
    APR_RING_INIT(me->tasks, apr_thread_pool_task, link);
    me->recycled_tasks = apr_palloc(me->pool, sizeof(*me->recycled_tasks));
    if (!me->recycled_tasks) {
        goto CATCH_ENOMEM;
    }
    APR_RING_INIT(me->recycled_tasks, apr_thread_pool_task, link);
    me->busy_thds = apr_palloc(me->pool, sizeof(*me->busy_thds));
    if (!me->busy_thds) {
        goto CATCH_ENOMEM;
    }
    APR_RING_INIT(me->busy_thds, apr_thread_list_elt, link);
    me->idle_thds = apr_palloc(me->pool, sizeof(*me->idle_thds));
    if (!me->idle_thds) {
        goto CATCH_ENOMEM;
    }
    APR_RING_INIT(me->idle_thds, apr_thread_list_elt, link);
    me->thd_cnt = me->idle_cnt = me->task_cnt = 0;
    me->terminated = 0;
    for (i = 0; i < TASK_PRIORITY_SEGS; i++) {
        me->task_idx[i] = NULL;
    }
    goto FINAL_EXIT;
  CATCH_ENOMEM:
    rv = APR_ENOMEM;
    apr_thread_mutex_destroy(me->lock);
    apr_thread_mutex_destroy(me->cond_lock);
    apr_thread_cond_destroy(me->cond);
  FINAL_EXIT:
    return rv;
}

/*
 * NOTE: This function is not thread safe by itself. Caller should hold the lock
 */
static apr_thread_pool_task_t *pop_task(apr_thread_pool_t * me)
{
    apr_thread_pool_task_t *task;
    int seg;

    if (0 == me->task_cnt) {
        return NULL;
    }

    task = APR_RING_FIRST(me->tasks);
    --me->task_cnt;
    seg = TASK_PRIORITY_SEG(task);
    if (task == me->task_idx[seg]) {
        me->task_idx[seg] = APR_RING_NEXT(task, link);
        if (me->task_idx[seg] == APR_RING_SENTINEL(me->tasks,
                                                   apr_thread_pool_task, link)
            || TASK_PRIORITY_SEG(me->task_idx[seg]) != seg) {
            me->task_idx[seg] = NULL;
        }
    }

    APR_RING_REMOVE(task, link);
    return task;
}

/*
 * The worker thread function. Take a task from the queue and perform it if
 * there is any. Otherwise, put itself into the idle thread list and waiting
 * for signal to wake up.
 * The thread terminate directly by detach and exit when it is asked to stop
 * after finishing a task. Otherwise, the thread should be in idle thread list
 * and should be joined.
 */
static void *APR_THREAD_FUNC thread_pool_func(apr_thread_t * t, void *param)
{
    apr_thread_pool_t *me = param;
    apr_thread_pool_task_t *task;
    struct apr_thread_list_elt *elt;

    elt = apr_pcalloc(me->pool, sizeof(*elt));
    if (!elt) {
        apr_thread_exit(t, APR_ENOMEM);
    }
    APR_RING_ELEM_INIT(elt, link);
    elt->thd = t;
    elt->stop = 0;

    apr_thread_mutex_lock(me->lock);
    while (!me->terminated && !elt->stop) {
        /* if not new element, it is awakened from idle */
        if (APR_RING_NEXT(elt, link) != elt) {
            --me->idle_cnt;
            APR_RING_REMOVE(elt, link);
        }
        APR_RING_INSERT_TAIL(me->busy_thds, elt, apr_thread_list_elt, link);
        task = pop_task(me);
        while (NULL != task && !me->terminated) {
            apr_thread_mutex_unlock(me->lock);
            task->func(t, task->param);
            apr_thread_mutex_lock(me->lock);
            APR_RING_INSERT_TAIL(me->recycled_tasks, task,
                                 apr_thread_pool_task, link);
            if (elt->stop) {
                break;
            }
            task = pop_task(me);
        }
        APR_RING_REMOVE(elt, link);

        /* busy thread been asked to stop, not joinable */
        if (me->idle_cnt >= me->idle_max || me->terminated || elt->stop) {
            --me->thd_cnt;
            apr_thread_mutex_unlock(me->lock);
            apr_thread_detach(t);
            apr_thread_exit(t, APR_SUCCESS);
            return NULL;        /* should not be here, safe net */
        }

        ++me->idle_cnt;
        APR_RING_INSERT_TAIL(me->idle_thds, elt, apr_thread_list_elt, link);
        apr_thread_mutex_unlock(me->lock);

        apr_thread_mutex_lock(me->cond_lock);
        apr_thread_cond_wait(me->cond, me->cond_lock);
        apr_thread_mutex_unlock(me->cond_lock);

        apr_thread_mutex_lock(me->lock);
    }

    /* idle thread been asked to stop, will be joined */
    --me->thd_cnt;
    apr_thread_mutex_unlock(me->lock);
    apr_thread_exit(t, APR_SUCCESS);
    return NULL;                /* should not be here, safe net */
}

static apr_status_t thread_pool_cleanup(void *me)
{
    apr_thread_pool_t *_self = me;

    _self->terminated = 1;
    apr_thread_pool_idle_max_set(_self, 0);
    while (_self->thd_cnt) {
        apr_sleep(20 * 1000);   /* spin lock with 20 ms */
    }
    apr_thread_mutex_destroy(_self->lock);
    apr_thread_mutex_destroy(_self->cond_lock);
    apr_thread_cond_destroy(_self->cond);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_pool_create(apr_thread_pool_t ** me,
                                                 apr_size_t init_threads,
                                                 apr_size_t max_threads,
                                                 apr_pool_t * pool)
{
    apr_thread_t *t;
    apr_status_t rv = APR_SUCCESS;

    *me = apr_pcalloc(pool, sizeof(**me));
    if (!*me) {
        return APR_ENOMEM;
    }

    (*me)->pool = pool;

    rv = thread_pool_construct(*me, init_threads, max_threads);
    if (APR_SUCCESS != rv) {
        *me = NULL;
        return rv;
    }
    apr_pool_cleanup_register(pool, *me, thread_pool_cleanup,
                              apr_pool_cleanup_null);

    while (init_threads) {
        rv = apr_thread_create(&t, NULL, thread_pool_func, *me, (*me)->pool);
        if (APR_SUCCESS != rv) {
            break;
        }
        ++(*me)->thd_cnt;
        --init_threads;
    }

    return rv;
}

APR_DECLARE(apr_status_t) apr_thread_pool_destroy(apr_thread_pool_t * me)
{
    return apr_pool_cleanup_run(me->pool, me, thread_pool_cleanup);
}

/*
 * NOTE: This function is not thread safe by itself. Caller should hold the lock
 */
static apr_thread_pool_task_t *task_new(apr_thread_pool_t * me,
                                        apr_thread_start_t func,
                                        void *param, apr_byte_t priority)
{
    apr_thread_pool_task_t *t;

    if (APR_RING_EMPTY(me->recycled_tasks, apr_thread_pool_task, link)) {
        t = apr_pcalloc(me->pool, sizeof(*t));
        if (NULL == t) {
            return NULL;
        }
    }
    else {
        t = APR_RING_FIRST(me->recycled_tasks);
        APR_RING_REMOVE(t, link);
    }

    APR_RING_ELEM_INIT(t, link);
    t->func = func;
    t->priority = priority;
    t->param = param;

    return t;
}

/*
 * Test it the task is the only one within the priority segment. 
 * If it is not, return the first element with same or lower priority. 
 * Otherwise, add the task into the queue and return NULL.
 *
 * NOTE: This function is not thread safe by itself. Caller should hold the lock
 */
static apr_thread_pool_task_t *add_if_empty(apr_thread_pool_t * me,
                                            apr_thread_pool_task_t * const t)
{
    int seg;
    int next;
    apr_thread_pool_task_t *t_next;

    seg = TASK_PRIORITY_SEG(t);
    if (me->task_idx[seg]) {
        assert(APR_RING_SENTINEL(me->tasks, apr_thread_pool_task, link) !=
               me->task_idx[seg]);
        t_next = me->task_idx[seg];
        while (t_next->priority > t->priority) {
            t_next = APR_RING_NEXT(t_next, link);
            if (APR_RING_SENTINEL(me->tasks, apr_thread_pool_task, link) ==
                t_next) {
                return t_next;
            }
        }
        return t_next;
    }

    for (next = seg - 1; next >= 0; next--) {
        if (me->task_idx[next]) {
            APR_RING_INSERT_BEFORE(me->task_idx[next], t, link);
            break;
        }
    }
    if (0 > next) {
        APR_RING_INSERT_TAIL(me->tasks, t, apr_thread_pool_task, link);
    }
    me->task_idx[seg] = t;
    return NULL;
}

static apr_status_t add_task(apr_thread_pool_t * me, apr_thread_start_t func,
                             void *param, apr_byte_t priority, int push)
{
    apr_thread_pool_task_t *t;
    apr_thread_pool_task_t *t_loc;
    apr_thread_t *thd;
    apr_status_t rv = APR_SUCCESS;

    apr_thread_mutex_lock(me->lock);

    t = task_new(me, func, param, priority);
    if (NULL == t) {
        apr_thread_mutex_unlock(me->lock);
        return APR_ENOMEM;
    }

    t_loc = add_if_empty(me, t);
    if (NULL == t_loc) {
        goto FINAL_EXIT;
    }

    if (push) {
        while (APR_RING_SENTINEL(me->tasks, apr_thread_pool_task, link) !=
               t_loc && t_loc->priority >= t->priority) {
            t_loc = APR_RING_NEXT(t_loc, link);
        }
    }
    APR_RING_INSERT_BEFORE(t_loc, t, link);
    if (!push) {
        if (t_loc == me->task_idx[TASK_PRIORITY_SEG(t)]) {
            me->task_idx[TASK_PRIORITY_SEG(t)] = t;
        }
    }

  FINAL_EXIT:
    me->task_cnt++;
    if (0 == me->thd_cnt || (0 == me->idle_cnt && me->thd_cnt < me->thd_max &&
                             me->task_cnt > me->threshold)) {
        rv = apr_thread_create(&thd, NULL, thread_pool_func, me, me->pool);
        if (APR_SUCCESS == rv) {
            ++me->thd_cnt;
        }
    }
    apr_thread_mutex_unlock(me->lock);

    apr_thread_mutex_lock(me->cond_lock);
    apr_thread_cond_signal(me->cond);
    apr_thread_mutex_unlock(me->cond_lock);

    return rv;
}

APR_DECLARE(apr_status_t) apr_thread_pool_push(apr_thread_pool_t * me,
                                               apr_thread_start_t func,
                                               void *param,
                                               apr_byte_t priority)
{
    return add_task(me, func, param, priority, 1);
}

APR_DECLARE(apr_status_t) apr_thread_pool_top(apr_thread_pool_t * me,
                                              apr_thread_start_t func,
                                              void *param,
                                              apr_byte_t priority)
{
    return add_task(me, func, param, priority, 0);
}

APR_DECLARE(apr_size_t) apr_thread_pool_tasks_count(apr_thread_pool_t * me)
{
    return me->task_cnt;
}

APR_DECLARE(apr_size_t) apr_thread_pool_threads_count(apr_thread_pool_t * me)
{
    return me->thd_cnt;
}

APR_DECLARE(apr_size_t) apr_thread_pool_busy_count(apr_thread_pool_t * me)
{
    return me->thd_cnt - me->idle_cnt;
}

APR_DECLARE(apr_size_t) apr_thread_pool_idle_count(apr_thread_pool_t * me)
{
    return me->idle_cnt;
}

APR_DECLARE(apr_size_t) apr_thread_pool_idle_max_get(apr_thread_pool_t * me)
{
    return me->idle_max;
}

/*
 * This function stop extra idle threads to the cnt.
 * @return the number of threads stopped
 * NOTE: There could be busy threads become idle during this function
 */
static struct apr_thread_list_elt *trim_threads(apr_thread_pool_t * me,
                                                apr_size_t * cnt, int idle)
{
    struct apr_thread_list *thds;
    apr_size_t n, n_dbg, i;
    struct apr_thread_list_elt *head, *tail, *elt;

    apr_thread_mutex_lock(me->lock);
    if (idle) {
        thds = me->idle_thds;
        n = me->idle_cnt;
    }
    else {
        thds = me->busy_thds;
        n = me->thd_cnt - me->idle_cnt;
    }
    if (n <= *cnt) {
        apr_thread_mutex_unlock(me->lock);
        *cnt = 0;
        return NULL;
    }
    n -= *cnt;

    head = APR_RING_FIRST(thds);
    for (i = 0; i < *cnt; i++) {
        head = APR_RING_NEXT(head, link);
    }
    tail = APR_RING_LAST(thds);
    APR_RING_UNSPLICE(head, tail, link);
    if (idle) {
        me->idle_cnt = *cnt;
    }
    apr_thread_mutex_unlock(me->lock);

    n_dbg = 0;
    for (elt = head; elt != tail; elt = APR_RING_NEXT(elt, link)) {
        elt->stop = 1;
        n_dbg++;
    }
    elt->stop = 1;
    n_dbg++;
    assert(n == n_dbg);
    *cnt = n;

    APR_RING_PREV(head, link) = NULL;
    APR_RING_NEXT(tail, link) = NULL;
    return head;
}

static apr_size_t trim_idle_threads(apr_thread_pool_t * me, apr_size_t cnt)
{
    apr_size_t n_dbg;
    struct apr_thread_list_elt *elt;
    apr_status_t rv;

    elt = trim_threads(me, &cnt, 1);

    apr_thread_mutex_lock(me->cond_lock);
    apr_thread_cond_broadcast(me->cond);
    apr_thread_mutex_unlock(me->cond_lock);

    n_dbg = 0;
    while (elt) {
        apr_thread_join(&rv, elt->thd);
        elt = APR_RING_NEXT(elt, link);
        ++n_dbg;
    }
    assert(cnt == n_dbg);

    return cnt;
}

/* don't join on busy threads for performance reasons, who knows how long will
 * the task takes to perform
 */
static apr_size_t trim_busy_threads(apr_thread_pool_t * me, apr_size_t cnt)
{
    trim_threads(me, &cnt, 0);
    return cnt;
}

APR_DECLARE(apr_size_t) apr_thread_pool_idle_max_set(apr_thread_pool_t * me,
                                                     apr_size_t cnt)
{
    me->idle_max = cnt;
    cnt = trim_idle_threads(me, cnt);
    return cnt;
}

APR_DECLARE(apr_size_t) apr_thread_pool_thread_max_get(apr_thread_pool_t * me)
{
    return me->thd_max;
}

/*
 * This function stop extra working threads to the new limit.
 * NOTE: There could be busy threads become idle during this function
 */
APR_DECLARE(apr_size_t) apr_thread_pool_thread_max_set(apr_thread_pool_t * me,
                                                       apr_size_t cnt)
{
    unsigned int n;

    me->thd_max = cnt;
    if (0 == cnt || me->thd_cnt <= cnt) {
        return 0;
    }

    n = me->thd_cnt - cnt;
    if (n >= me->idle_cnt) {
        trim_busy_threads(me, n - me->idle_cnt);
        trim_idle_threads(me, 0);
    }
    else {
        trim_idle_threads(me, me->idle_cnt - n);
    }
    return n;
}

APR_DECLARE(apr_size_t) apr_thread_pool_threshold_get(apr_thread_pool_t * me)
{
    return me->threshold;
}

APR_DECLARE(apr_size_t) apr_thread_pool_threshold_set(apr_thread_pool_t * me,
                                                      apr_size_t val)
{
    apr_size_t ov;

    ov = me->threshold;
    me->threshold = val;
    return ov;
}

#endif /* APR_HAS_THREADS */

/* vim: set ts=4 sw=4 et cin tw=80: */
