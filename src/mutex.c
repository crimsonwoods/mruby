#ifdef MRB_USE_MUTEX_API
#include "mruby/mutex.h"
#include <errno.h>

MRB_API void
mrb_mutex_init_api(mrb_state *mrb, const mrb_mutex_api_t *api)
{
  if (MRB_GET_VM(mrb)->mutex_api == NULL) {
    MRB_GET_VM(mrb)->mutex_api = (mrb_mutex_api_t *)mrb_malloc(mrb, sizeof(mrb_mutex_api_t));
  }
  *MRB_GET_VM(mrb)->mutex_api = *api;
}

mrb_mutexattr_t *
mrb_mutexattr_create(mrb_state *mrb)
{
  if (MRB_GET_VM(mrb)->mutex_api == NULL) {
    return NULL;
  }
  return MRB_GET_VM(mrb)->mutex_api->mutexattr_create(mrb);
}

void
mrb_mutexattr_destroy(mrb_state *mrb, mrb_mutexattr_t *attr)
{
  if (MRB_GET_VM(mrb)->mutex_api == NULL) {
    return;
  }
  MRB_GET_VM(mrb)->mutex_api->mutexattr_destroy(mrb, attr);
}

mrb_mutex_t *
mrb_mutex_create(mrb_state *mrb, mrb_mutexattr_t *attr)
{
  if (MRB_GET_VM(mrb)->mutex_api == NULL) {
    return NULL;
  }
  return MRB_GET_VM(mrb)->mutex_api->mutex_create(mrb, attr);
}

void
mrb_mutex_destroy(mrb_state *mrb, mrb_mutex_t *mutex)
{
  if (MRB_GET_VM(mrb)->mutex_api == NULL) {
    return;
  }
  return MRB_GET_VM(mrb)->mutex_api->mutex_destroy(mrb, mutex);
}

int
mrb_mutex_lock(mrb_state *mrb, mrb_mutex_t *mutex)
{
  if (MRB_GET_VM(mrb)->mutex_api == NULL) {
    return ENOTSUP;
  }
  return MRB_GET_VM(mrb)->mutex_api->mutex_lock(mrb, mutex);
}

int
mrb_mutex_unlock(mrb_state *mrb, mrb_mutex_t *mutex)
{
  if (MRB_GET_VM(mrb)->mutex_api == NULL) {
    return ENOTSUP;
  }
  return MRB_GET_VM(mrb)->mutex_api->mutex_unlock(mrb, mutex);
}

int
mrb_mutex_trylock(mrb_state *mrb, mrb_mutex_t *mutex)
{
  if (MRB_GET_VM(mrb)->mutex_api == NULL) {
    return ENOTSUP;
  }
  return MRB_GET_VM(mrb)->mutex_api->mutex_trylock(mrb, mutex);
}

#endif
