#ifdef MRB_USE_MUTEX_API
#include "mruby.h"
#include "mruby/mutex.h"
#include <windows.h>
#include <errno.h>

#ifndef MRB_DEFAULT_SPIN_COUNT
#define MRB_DEFAULT_SPIN_COUNT 0u
#endif

struct mrb_mutexattr_t {
  DWORD spin_count;
};

struct mrb_mutex_t {
  CRITICAL_SECTION critical_section;
};

static mrb_mutexattr_t*
mrb_win32_mutexattr_create(mrb_state *mrb)
{
  mrb_mutexattr_t *attr = (mrb_mutexattr_t *)mrb_malloc(mrb, sizeof(mrb_mutexattr_t));
  attr->spin_count = MRB_DEFAULT_SPIN_COUNT;
  return attr;
}

static void
mrb_win32_mutexattr_destroy(mrb_state *mrb, mrb_mutexattr_t *attr)
{
  if (attr == NULL) {
    return;
  }
  mrb_free(mrb, attr);
}

static mrb_mutex_t*
mrb_win32_mutex_create(mrb_state *mrb, mrb_mutexattr_t *attr)
{
  mrb_mutex_t *mutex = (mrb_mutex_t*)mrb_malloc(mrb, sizeof(mrb_mutex_t));
  if (attr == NULL) {
    InitializeCriticalSection(&mutex->critical_section);
  } else {
    if (!InitializeCriticalSectionAndSpinCount(&mutex->critical_section, attr->spin_count)) {
      mrb_free(mrb, mutex);
      return NULL;
    }
  }
  return mutex;
}

static void
mrb_win32_mutex_destroy(mrb_state *mrb, mrb_mutex_t *mutex)
{
  if (mutex == NULL) {
    return;
  }
  DeleteCriticalSection(&mutex->critical_section);
  mrb_free(mrb, mutex);
}

static int
mrb_win32_mutex_lock(mrb_state *mrb, mrb_mutex_t *mutex)
{
  if (mutex == NULL) {
    return EINVAL;
  }
  EnterCriticalSection(&mutex->critical_section);
  return 0;
}

static int
mrb_win32_mutex_unlock(mrb_state *mrb, mrb_mutex_t *mutex)
{
  if (mutex == NULL) {
    return EINVAL;
  }
  LeaveCriticalSection(&mutex->critical_section);
  return 0;
}

static int
mrb_win32_mutex_trylock(mrb_state *mrb, mrb_mutex_t *mutex)
{
  if (mutex == NULL) {
    return EINVAL;
  }
  if (!TryEnterCriticalSection(&mutex->critical_section)) {
    return EBUSY;
  }
  return 0;
}

static const mrb_mutex_api_t api = {
  .mutexattr_create  = mrb_win32_mutexattr_create,
  .mutexattr_destroy = mrb_win32_mutexattr_destroy,
  .mutex_create      = mrb_win32_mutex_create,
  .mutex_destroy     = mrb_win32_mutex_destroy,
  .mutex_lock        = mrb_win32_mutex_lock,
  .mutex_unlock      = mrb_win32_mutex_unlock,
  .mutex_trylock     = mrb_win32_mutex_trylock,
};

void
mrb_win32_mutex_init_api(mrb_state *mrb) {
  mrb_mutex_init_api(mrb, &api);
}

#endif

