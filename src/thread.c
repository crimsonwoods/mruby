#ifdef MRB_USE_THREAD_API
#include "mruby/thread.h"
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
  return mrb_duplicate_core(mrb);
}

MRB_API void
mrb_thread_detach_vm(mrb_state *mrb)
{
  mrb_close_duplicated(mrb);
}

MRB_API int
mrb_thread_sleep(mrb_state *mrb, uint32_t millis)
{
  if (MRB_GET_VM(mrb)->thread_api == NULL) {
    return ENOENT;
  }
  return MRB_GET_VM(mrb)->thread_api->thread_sleep(mrb, millis);
}

#endif
