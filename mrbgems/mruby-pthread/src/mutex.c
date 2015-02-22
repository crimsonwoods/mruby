#ifdef MRB_USE_MUTEX_API
#include "mruby.h"
#include "mruby/mutex.h"
#include <pthread.h>
#include <errno.h>

struct mrb_mutexattr_t {
  pthread_mutexattr_t attr;
};

struct mrb_mutex_t {
  pthread_mutex_t mutex;
};

static mrb_mutexattr_t*
mrb_pthread_mutexattr_create(mrb_state *mrb)
{
  mrb_mutexattr_t *attr = (mrb_mutexattr_t *)mrb_malloc(mrb, sizeof(mrb_mutexattr_t));
  (void)pthread_mutexattr_init(&attr->attr);
  return attr;
}

static void
mrb_pthread_mutexattr_destroy(mrb_state *mrb, mrb_mutexattr_t *attr)
{
  if (attr == NULL) {
    return;
  }
  pthread_mutexattr_destroy(&attr->attr);
  mrb_free(mrb, attr);
}

static mrb_mutex_t*
mrb_pthread_mutex_create(mrb_state *mrb, mrb_mutexattr_t *attr)
{
  mrb_mutex_t *mutex = (mrb_mutex_t*)mrb_malloc(mrb, sizeof(mrb_mutex_t));
  if (attr == NULL) {
    (void)pthread_mutex_init(&mutex->mutex, NULL);
  } else {
    (void)pthread_mutex_init(&mutex->mutex, &attr->attr);
  }
  return mutex;
}

static void
mrb_pthread_mutex_destroy(mrb_state *mrb, mrb_mutex_t *mutex)
{
  if (mutex == NULL) {
    return;
  }
  (void)pthread_mutex_destroy(&mutex->mutex);
  mrb_free(mrb, mutex);
}

static int
mrb_pthread_mutex_lock(mrb_state *mrb, mrb_mutex_t *mutex)
{
  if (mutex == NULL) {
    return EINVAL;
  }
  return pthread_mutex_lock(&mutex->mutex);
}

static int
mrb_pthread_mutex_unlock(mrb_state *mrb, mrb_mutex_t *mutex)
{
  if (mutex == NULL) {
    return EINVAL;
  }
  return pthread_mutex_unlock(&mutex->mutex);
}

static int
mrb_pthread_mutex_trylock(mrb_state *mrb, mrb_mutex_t *mutex)
{
  if (mutex == NULL) {
    return EINVAL;
  }
  return pthread_mutex_trylock(&mutex->mutex);
}

static const mrb_mutex_api_t api = {
  .mutexattr_create  = mrb_pthread_mutexattr_create,
  .mutexattr_destroy = mrb_pthread_mutexattr_destroy,
  .mutex_create      = mrb_pthread_mutex_create,
  .mutex_destroy     = mrb_pthread_mutex_destroy,
  .mutex_lock        = mrb_pthread_mutex_lock,
  .mutex_unlock      = mrb_pthread_mutex_unlock,
  .mutex_trylock     = mrb_pthread_mutex_trylock,
};

void
mrb_pthread_mutex_init_api(mrb_state *mrb) {
  mrb_mutex_init_api(mrb, &api);
}

#endif

