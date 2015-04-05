#ifdef MRB_USE_THREAD_API
#include "mruby/thread.h"
#include "mruby/atomic.h"
#include <errno.h>

MRB_API void
mrb_thread_init_api(mrb_state *mrb, const mrb_thread_api_t *api)
{
  if (MRB_GET_VM(mrb)->thread_api == NULL) {
    MRB_GET_VM(mrb)->thread_api = (mrb_thread_api_t*)mrb_malloc(mrb, sizeof(mrb_thread_api_t));
  }
  *MRB_GET_VM(mrb)->thread_api = *api;
}

MRB_API mrb_threadattr_t*
mrb_threadattr_create(mrb_state *mrb)
{
  if (MRB_GET_VM(mrb)->thread_api == NULL) {
    return NULL;
  }
  return MRB_GET_VM(mrb)->thread_api->thread_attr_create(mrb);
}

MRB_API void
mrb_threadattr_destroy(mrb_state *mrb, mrb_threadattr_t *attr)
{
  if (MRB_GET_VM(mrb)->thread_api == NULL) {
    return;
  }
  MRB_GET_VM(mrb)->thread_api->thread_attr_destroy(mrb, attr);
}

MRB_API mrb_thread_t*
mrb_thread_create(mrb_state *mrb, mrb_threadattr_t *attr, mrb_thread_proc_t proc, void *arg)
{
  if (MRB_GET_VM(mrb)->thread_api == NULL) {
    return NULL;
  }
  return MRB_GET_VM(mrb)->thread_api->thread_create(mrb, attr, proc, arg);
}

MRB_API void
mrb_thread_destroy(mrb_state *mrb, mrb_thread_t *thread)
{
  if (MRB_GET_VM(mrb)->thread_api == NULL) {
    return;
  }
  MRB_GET_VM(mrb)->thread_api->thread_destroy(mrb, thread);
}

MRB_API int
mrb_thread_join(mrb_state *mrb, mrb_thread_t *thread, void **result)
{
  if (MRB_GET_VM(mrb)->thread_api == NULL) {
    return ENOENT;
  }
  return MRB_GET_VM(mrb)->thread_api->thread_join(mrb, thread, result);
}

MRB_API mrb_state*
mrb_thread_attach_vm(mrb_state *mrb)
{
  mrb_int i;
  mrb_state *new_state = mrb_duplicate_core(mrb);
  for (i = 0; i < MRB_FIXED_THREAD_SIZE; ++i) {
    if (MRB_GET_VM(mrb)->threads[i] == NULL) {
      MRB_GET_VM(mrb)->threads[i] = MRB_GET_THREAD_CONTEXT(new_state);
      ++MRB_GET_VM(mrb)->thread_count;
      break;
    }
  }
  return new_state;
}

MRB_API void
mrb_thread_detach_vm(mrb_state *mrb)
{
  mrb_thread_context *context;
  mrb_int i;
  if (!mrb) {
    return;
  }
  context = MRB_GET_THREAD_CONTEXT(mrb);
  for (i = 0; i < MRB_FIXED_THREAD_SIZE; ++i) {
    if (MRB_GET_VM(mrb)->threads[i] == context) {
      MRB_GET_VM(mrb)->threads[i] = NULL;
      --MRB_GET_VM(mrb)->thread_count;
      mrb_close_duplicated(mrb);
      break;
    }
  }
}

MRB_API int
mrb_thread_sleep(mrb_state *mrb, uint32_t millis)
{
  if (MRB_GET_VM(mrb)->thread_api == NULL) {
    return ENOENT;
  }
  return MRB_GET_VM(mrb)->thread_api->thread_sleep(mrb, millis);
}

MRB_API void
mrb_thread_yield(mrb_state *mrb)
{
  if (MRB_GET_VM(mrb)->thread_api == NULL) {
    return;
  }
  MRB_GET_VM(mrb)->thread_api->thread_yield(mrb);
}

#if defined(MRB_USE_GVL_API) && !defined(MRB_NO_USE_TIMER_THREAD)

#ifndef MRB_TIMER_THREAD_INTERVAL
#define MRB_TIMER_THREAD_INTERVAL 1
#endif

static void*
timer_thread(mrb_state *mrb, void *arg)
{
  size_t i;
  while (!mrb_atomic_bool_load(&MRB_GET_VM(mrb)->stop_timer_thread)) {
    mrb_thread_sleep(mrb, MRB_TIMER_THREAD_INTERVAL);
    if (MRB_GET_VM(mrb)->thread_count <= 1) {
      continue;
    }
    for (i = 0; i < MRB_FIXED_THREAD_SIZE; ++i) {
      mrb_thread_context * const context = MRB_GET_VM(mrb)->threads[i];
      if (!context) {
        continue;
      }
#ifdef MRB_USE_ATOMIC_API
      if (!mrb_atomic_bool_load(&context->flag_gvl_acquired)) {
#else
      if (!context->flag_gvl_acquired) {
#endif
        continue;
      }
#ifdef MRB_USE_ATOMIC_API
      mrb_atomic_bool_store(&context->flag_gvl_releasing_requested, TRUE);
#else
      context->flag_gvl_releasing_requested = TRUE;
#endif
    }
  }
  return 0;
}

MRB_API void
mrb_timer_thread_create(mrb_state *mrb)
{
  if (MRB_GET_VM(mrb)->timer_thread) {
    return;
  }
  MRB_GET_VM(mrb)->timer_thread = mrb_thread_create(mrb, NULL, timer_thread, NULL);
}

MRB_API void
mrb_timer_thread_destroy(mrb_state *mrb)
{
  if (!MRB_GET_VM(mrb)->timer_thread) {
    return;
  }
  mrb_atomic_bool_store(&MRB_GET_VM(mrb)->stop_timer_thread, TRUE);
  mrb_thread_join(mrb, MRB_GET_VM(mrb)->timer_thread, NULL);
  mrb_thread_destroy(mrb, MRB_GET_VM(mrb)->timer_thread);
  MRB_GET_VM(mrb)->timer_thread = NULL;
}

#endif
#endif
