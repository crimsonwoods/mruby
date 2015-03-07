#ifdef MRB_USE_THREAD_API

#include "mruby.h"

extern void mrb_define_thread_class(mrb_state *mrb);
#ifdef MRB_USE_MUTEX_API
extern void mrb_define_mutex_class(mrb_state *mrb);
#endif

void
mrb_mruby_thread_gem_init(mrb_state *mrb)
{
  mrb_define_thread_class(mrb);
#ifdef MRB_USE_MUTEX_API
  mrb_define_mutex_class(mrb);
#endif
}

void
mrb_mruby_thread_gem_final(mrb_state *mrb)
{
}

#endif

