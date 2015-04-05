#ifdef MRB_USE_GVL_API

#ifndef MRB_USE_MUTEX_API
#error Mutex API is required.
#endif

#include "mruby/gvl.h"
#include "mruby/mutex.h"
#include "mruby/thread.h"
#include "mruby/atomic.h"

#ifdef MRB_GVL_DEBUG
#undef mrb_gvl_acquire
#undef mrb_gvl_release
#endif

struct mrb_gvl_t {
  mrb_mutex_t *mutex;
};

MRB_API void
mrb_gvl_init(mrb_state *mrb)
{
  if (!MRB_GET_VM(mrb)->gvl) {
    MRB_GET_VM(mrb)->gvl = (mrb_gvl_t *)mrb_malloc(mrb, sizeof(mrb_gvl_t));
    MRB_GET_VM(mrb)->gvl->mutex = mrb_mutex_create(mrb, 0);
  }
}

MRB_API void
mrb_gvl_cleanup(mrb_state *mrb)
{
  if (MRB_GET_VM(mrb)->gvl) {
    mrb_mutex_destroy(mrb, MRB_GET_VM(mrb)->gvl->mutex);
    MRB_GET_VM(mrb)->gvl->mutex = 0;
    mrb_free(mrb, MRB_GET_VM(mrb)->gvl);
    MRB_GET_VM(mrb)->gvl = 0;
  }
}

MRB_API void
mrb_gvl_acquire(mrb_state *mrb)
{
#ifdef MRB_USE_ATOMIC_API
  if (!MRB_GET_VM(mrb)->gvl || mrb_atomic_bool_load(&MRB_GET_THREAD_CONTEXT(mrb)->flag_gvl_acquired)) {
#else
  if (!MRB_GET_VM(mrb)->gvl || MRB_GET_THREAD_CONTEXT(mrb)->flag_gvl_acquired) {
#endif
    return;
  }
  mrb_mutex_lock(mrb, MRB_GET_VM(mrb)->gvl->mutex);
#ifdef MRB_USE_ATOMIC_API
  mrb_atomic_bool_store(&MRB_GET_THREAD_CONTEXT(mrb)->flag_gvl_acquired, TRUE);
#else
  MRB_GET_THREAD_CONTEXT(mrb)->flag_gvl_acquired = TRUE;
#endif
}

MRB_API void
mrb_gvl_release(mrb_state *mrb)
{
#ifdef MRB_USE_ATOMIC_API
  if (!MRB_GET_VM(mrb)->gvl || !mrb_atomic_bool_load(&MRB_GET_THREAD_CONTEXT(mrb)->flag_gvl_acquired)) {
#else
  if (!MRB_GET_VM(mrb)->gvl || !MRB_GET_THREAD_CONTEXT(mrb)->flag_gvl_acquired) {
#endif
    return;
  }
  mrb_mutex_unlock(mrb, MRB_GET_VM(mrb)->gvl->mutex);
#ifdef MRB_USE_ATOMIC_API
  mrb_atomic_bool_store(&MRB_GET_THREAD_CONTEXT(mrb)->flag_gvl_acquired, FALSE);
#else
  MRB_GET_THREAD_CONTEXT(mrb)->flag_gvl_acquired = FALSE;
#endif
}

MRB_API void
mrb_gvl_yield(mrb_state *mrb)
{
  mrb_gvl_release(mrb);
#ifdef MRB_USE_THREAD_API
#  ifdef MRB_USE_ATOMIC_API
  mrb_atomic_bool_store(&MRB_GET_THREAD_CONTEXT(mrb)->flag_gvl_releasing_requested, FALSE);
#  else
  MRB_GET_THREAD_CONTEXT(mrb)->flag_gvl_releasing_requested = FALSE;
#  endif
  mrb_thread_yield(mrb);
#endif
  mrb_gvl_acquire(mrb);
}

MRB_API mrb_bool
mrb_gvl_is_acquired(mrb_state *mrb)
{
#ifdef MRB_USE_ATOMIC_API
  return mrb_atomic_bool_load(&MRB_GET_THREAD_CONTEXT(mrb)->flag_gvl_acquired);
#else
  return MRB_GET_THREAD_CONTEXT(mrb)->flag_gvl_acquired;
#endif
}

MRB_API void
mrb_gvl_acquire_dbg(mrb_state *mrb, char const *file, int line, char const *func) {
#ifdef MRB_USE_THREAD_API
  fprintf(stderr, "GVL: acquire in %s %s:%d (in Thread-#%d)\n", func, file, line, MRB_GET_THREAD_CONTEXT(mrb)->id);
#else
  fprintf(stderr, "GVL: acquire in %s %s:%d\n", func, file, line);
#endif
  mrb_gvl_acquire(mrb);
}

MRB_API void
mrb_gvl_release_dbg(mrb_state *mrb, char const *file, int line, char const *func) {
#ifdef MRB_USE_THREAD_API
  fprintf(stderr, "GVL: release in %s %s:%d (in Thread-#%d)\n", func, file, line, MRB_GET_THREAD_CONTEXT(mrb)->id);
#else
  fprintf(stderr, "GVL: release in %s %s:%d\n", func, file, line);
#endif
  mrb_gvl_release(mrb);
}

MRB_API void
mrb_blocking_region_begin(mrb_state *mrb, mrb_blocking_region_t *region)
{
  const mrb_bool is_gvl_acquired = mrb_gvl_is_acquired(mrb);
  if (is_gvl_acquired) {
    mrb_gvl_release(mrb);
  }
  region->is_gvl_released = is_gvl_acquired;
}

MRB_API void
mrb_blocking_region_end(mrb_state *mrb, mrb_blocking_region_t *region)
{
  if (region->is_gvl_released) {
    mrb_gvl_acquire(mrb);
  }
}

#endif
