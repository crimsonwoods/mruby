#ifdef MRB_USE_THREAD_API

#include "mruby.h"
#include "mruby/thread.h"
#ifdef MRB_USE_GVL_API
#include "mruby/gvl.h"
#endif
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

struct mrb_threadattr_t {
  pthread_attr_t attr;
};

struct mrb_thread_t {
  pthread_t thread;
};

typedef struct mrb_thread_params_t {
  mrb_state        *mrb;
  mrb_thread_proc_t proc;
  void             *arg;
} mrb_thread_params_t;

static void *thread_entry_proc(void *arg);

static mrb_threadattr_t*
mrb_pthread_attr_create(mrb_state *mrb)
{
  mrb_threadattr_t *attr = (mrb_threadattr_t *)mrb_malloc(mrb, sizeof(mrb_threadattr_t));
  (void)pthread_attr_init(&attr->attr);
  return attr;
}

static void
mrb_pthread_attr_destroy(mrb_state *mrb, mrb_threadattr_t *attr)
{
  if (!attr) {
    return;
  }
  (void)pthread_attr_destroy(&attr->attr);
  mrb_free(mrb, attr);
}

static mrb_thread_t*
mrb_pthread_create(mrb_state *mrb, mrb_threadattr_t *attr, mrb_thread_proc_t proc, void *arg)
{
  int err;
  mrb_thread_t *thread = (mrb_thread_t*)mrb_malloc(mrb, sizeof(mrb_thread_t));
  mrb_thread_params_t *params = (mrb_thread_params_t*)mrb_malloc(mrb, sizeof(mrb_thread_params_t));

  params->mrb  = mrb;
  params->proc = proc;
  params->arg  = arg;

  if (attr == NULL) {
    err = pthread_create(&thread->thread, NULL, thread_entry_proc, params);
  } else {
    err = pthread_create(&thread->thread, &attr->attr, thread_entry_proc, params);
  }

  if (err != 0) {
    mrb_free(mrb, params);
    mrb_free(mrb, thread);
    mrb_raise(mrb, E_RUNTIME_ERROR, "cannot create a new thread.");
    return NULL;
  }

  return thread;
}

static int
mrb_pthread_join(mrb_state *mrb, mrb_thread_t *thread, void **result)
{
  if (thread == NULL) {
    return EINVAL;
  } else {
    int err;
#ifdef MRB_USE_GVL_API
    const mrb_bool is_gvl_acquired = mrb_gvl_is_acquired(mrb);
    if (is_gvl_acquired) {
      mrb_gvl_release(mrb);
    }
    err = pthread_join(thread->thread, result);
    if (is_gvl_acquired) {
      mrb_gvl_acquire(mrb);
    }
#else
    err = pthread_join(thread->thread, result);
#endif
    return err;
  }
}

static void
mrb_pthread_destroy(mrb_state *mrb, mrb_thread_t *thread)
{
  mrb_free(mrb, thread);
}

static int
mrb_usleep(mrb_state *mrb, uint32_t millis) {
  return usleep(millis * 1000u);
}

static void*
thread_entry_proc(void *arg)
{
  mrb_thread_params_t * const params = (mrb_thread_params_t*)arg;
  mrb_state        *mrb;
  mrb_thread_proc_t proc;

  mrb  = params->mrb;
  proc = params->proc;
  arg  = params->arg;

  mrb_free(mrb, params);

  return proc(mrb, arg);
}

static const mrb_thread_api_t pthread_api = {
  .thread_attr_create  = mrb_pthread_attr_create,
  .thread_attr_destroy = mrb_pthread_attr_destroy,
  .thread_create       = mrb_pthread_create,
  .thread_destroy      = mrb_pthread_destroy,
  .thread_join         = mrb_pthread_join,
  .thread_sleep        = mrb_usleep,
};

extern void mrb_pthread_mutex_init_api(mrb_state *mrb);

void
mrb_mruby_pthread_gem_init(mrb_state *mrb)
{
  mrb_thread_init_api(mrb, &pthread_api);
#ifdef MRB_USE_MUTEX_API
  mrb_pthread_mutex_init_api(mrb);
#ifdef MRB_USE_GVL_API
  mrb_gvl_init(mrb);
#endif
#endif
}

void
mrb_mruby_pthread_gem_final(mrb_state *mrb)
{
}

#endif
