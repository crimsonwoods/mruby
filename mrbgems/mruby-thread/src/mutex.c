#ifdef MRB_USE_MUTEX_API

#include "mruby.h"
#include "mruby/class.h"
#include "mruby/data.h"
#include "mruby/value.h"
#include "mruby/mutex.h"
#include "mruby/proc.h"

static const char MUTEX_CLASSNAME[] = "Mutex";

typedef struct mrb_mutex_data_t {
  mrb_mutex_t *mutex;
} mrb_mutex_data_t;

static void
mrb_free_mutex_data(mrb_state *mrb, void *ptr)
{
  mrb_mutex_data_t *data = (mrb_mutex_data_t*)ptr;
  if (data) {
    if (data->mutex) {
      mrb_mutex_destroy(mrb, data->mutex);
    }
    mrb_free(mrb, data);
  }
}

static const struct mrb_data_type mrb_mutex_data_type = {
  MUTEX_CLASSNAME, mrb_free_mutex_data
};

static mrb_value
mrb_mutex_obj_init(mrb_state *mrb, mrb_value self)
{
  mrb_mutex_data_t *data;

  data = (mrb_mutex_data_t*)DATA_PTR(self);
  if (data) {
    mrb_free_mutex_data(mrb, data);
  }
  mrb_data_init(self, 0, &mrb_mutex_data_type);

  data = mrb_malloc(mrb, sizeof(mrb_mutex_data_t));
  data->mutex = mrb_mutex_create(mrb, 0);

  mrb_data_init(self, data, &mrb_mutex_data_type);

  return self;
}

static mrb_value
mrb_mutex_obj_lock(mrb_state *mrb, mrb_value self)
{
  mrb_mutex_data_t *data = DATA_GET_PTR(mrb, self, &mrb_mutex_data_type, mrb_mutex_data_t);
  if (mrb_mutex_lock(mrb, data->mutex) != 0) {
    // TODO raise exception
  }
  return self;
}

static mrb_value
mrb_mutex_obj_unlock(mrb_state *mrb, mrb_value self)
{
  mrb_mutex_data_t *data = DATA_GET_PTR(mrb, self, &mrb_mutex_data_type, mrb_mutex_data_t);
  if (mrb_mutex_unlock(mrb, data->mutex) != 0) {
    // TODO raise exception
  }
  return self;
}

static mrb_value
mrb_mutex_obj_try_lock(mrb_state *mrb, mrb_value self)
{
  mrb_mutex_data_t *data = DATA_GET_PTR(mrb, self, &mrb_mutex_data_type, mrb_mutex_data_t);
  if (mrb_mutex_trylock(mrb, data->mutex) != 0) {
    return mrb_false_value();
  }
  return mrb_true_value();
}

static mrb_value
mrb_mutex_obj_synchronize(mrb_state *mrb, mrb_value self)
{
  mrb_value block;
  mrb_value ret;
  mrb_mutex_data_t *data = DATA_GET_PTR(mrb, self, &mrb_mutex_data_type, mrb_mutex_data_t);

  mrb_get_args(mrb, "&", &block);

  if (mrb_nil_p(block)) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "supplied block object is nil.");
  }

  if (MRB_PROC_CFUNC_P(mrb_proc_ptr(block))) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "C-function is not supported.");
  }

  if (mrb_mutex_lock(mrb, data->mutex) != 0) {
    // TODO raise exception
  }

  ret = mrb_yield_argv(mrb, block, 0, NULL);

  (void)mrb_mutex_unlock(mrb, data->mutex);

  return ret;
}

void
mrb_define_mutex_class(mrb_state *mrb)
{
  struct RClass *c = mrb_define_class(mrb, MUTEX_CLASSNAME, MRB_GET_VM(mrb)->object_class);

  MRB_SET_INSTANCE_TT(c, MRB_TT_DATA);

  mrb_define_method(mrb, c, "initialize",  mrb_mutex_obj_init,        MRB_ARGS_OPT(1));
  mrb_define_method(mrb, c, "lock",        mrb_mutex_obj_lock,        MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "unlock",      mrb_mutex_obj_unlock,      MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "try_lock",    mrb_mutex_obj_try_lock,    MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "synchronize", mrb_mutex_obj_synchronize, MRB_ARGS_REQ(1));
}

#endif
