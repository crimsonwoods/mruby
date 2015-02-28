#ifdef MRB_USE_THREAD_API

#include "mruby.h"
#include "mruby/thread.h"
#ifdef MRB_USE_GVL_API
#include "mruby/gvl.h"
#endif
#include <windows.h>
#include <errno.h>
#include <process.h>

struct mrb_threadattr_t {
  LPSECURITY_ATTRIBUTES security_attr;
  DWORD stack_size;
  DWORD flags;
};

struct mrb_thread_t {
  HANDLE thread;
};

typedef struct mrb_thread_params_t {
  mrb_state        *mrb;
  mrb_thread_proc_t proc;
  void             *arg;
} mrb_thread_params_t;

static DWORD thread_entry_proc(LPVOID arg);

static mrb_threadattr_t*
mrb_win32_thread_attr_create(mrb_state *mrb)
{
  mrb_threadattr_t *attr = (mrb_threadattr_t *)mrb_malloc(mrb, sizeof(mrb_threadattr_t));
  attr->security_attr = NULL;
  attr->stack_size = 0;
  attr->flags = 0;
  return attr;
}

static void
mrb_win32_thread_attr_destroy(mrb_state *mrb, mrb_threadattr_t *attr)
{
  if (!attr) {
    return;
  }
  mrb_free(mrb, attr);
}

static mrb_thread_t*
mrb_win32_thread_create(mrb_state *mrb, mrb_threadattr_t *attr, mrb_thread_proc_t proc, void *arg)
{
  mrb_thread_t *thread = (mrb_thread_t*)mrb_malloc(mrb, sizeof(mrb_thread_t));
  mrb_thread_params_t *params = (mrb_thread_params_t*)mrb_malloc(mrb, sizeof(mrb_thread_params_t));

  params->mrb  = mrb;
  params->proc = proc;
  params->arg  = arg;

  if (attr == NULL) {
    thread->thread = (HANDLE)_beginthreadex(NULL, 0, thread_entry_proc, params, 0, NULL);
  } else {
    thread->thread = (HANDLE)_beginthreadex(attr->security_attr, attr->stack_size, thread_entry_proc, params, attr->flags, NULL);
  }

  if ((thread->thread == NULL) || (thread->thread == INVALID_HANDLE_VALUE)) {
    mrb_free(mrb, params);
    mrb_free(mrb, thread);
    mrb_raise(mrb, E_RUNTIME_ERROR, "cannot create a new thread.");
    return NULL;
  }

  return thread;
}

static int
mrb_win32_thread_join(mrb_state *mrb, mrb_thread_t *thread, void **result)
{
  if (thread == NULL) {
    return EINVAL;
  } else {
    DWORD err;
#ifdef MRB_USE_GVL_API
    const mrb_bool is_gvl_acquired = mrb_gvl_is_acquired(mrb);
    if (is_gvl_acquired) {
      mrb_gvl_release(mrb);
    }
    err = WaitForSingleObject(thread->thread, INFINITE);
    if (is_gvl_acquired) {
      mrb_gvl_acquire(mrb);
    }
#else
    err = WaitForSingleObject(thread->thread, INFINITE);
#endif
    if ((err == WAIT_OBJECT_0) && result) {
      DWORD code = 0;
      if (GetExitCodeThread(thread->thread, &code)) {
        *result = (void*)code;
      }
    }
    return err == WAIT_OBJECT_0 ? 0 : EINVAL;
  }
}

static void
mrb_win32_thread_destroy(mrb_state *mrb, mrb_thread_t *thread)
{
  if (thread != NULL) {
    if (thread->thread != NULL) {
      CloseHandle(thread->thread);
      thread->thread = NULL;
    }
    mrb_free(mrb, thread);
  }
}

static int
mrb_win32_sleep(mrb_state *mrb, uint32_t millis) {
  Sleep(millis);
  return 0;
}

static DWORD
thread_entry_proc(LPVOID arg)
{
  mrb_thread_params_t * const params = (mrb_thread_params_t*)arg;
  mrb_state        *mrb;
  mrb_thread_proc_t proc;

  mrb  = params->mrb;
  proc = params->proc;
  arg  = params->arg;

  mrb_free(mrb, params);

  return (DWORD)proc(mrb, arg);
}

static const mrb_thread_api_t pthread_api = {
  .thread_attr_create  = mrb_win32_thread_attr_create,
  .thread_attr_destroy = mrb_win32_thread_attr_destroy,
  .thread_create       = mrb_win32_thread_create,
  .thread_destroy      = mrb_win32_thread_destroy,
  .thread_join         = mrb_win32_thread_join,
  .thread_sleep        = mrb_win32_sleep,
};

extern void mrb_win32_mutex_init_api(mrb_state *mrb);

void
mrb_mruby_win32_thread_gem_init(mrb_state *mrb)
{
  mrb_thread_init_api(mrb, &pthread_api);
#ifdef MRB_USE_MUTEX_API
  mrb_win32_mutex_init_api(mrb);
#endif
}

void
mrb_mruby_win32_thread_gem_final(mrb_state *mrb)
{
}

#endif
