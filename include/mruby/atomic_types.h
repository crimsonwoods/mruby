#ifndef MRUBY_ATOMIC_TYPES_H
#define MRUBY_ATOMIC_TYPES_H

#ifndef MRB_USE_ATOMIC_API_STUB
typedef mrb_int   mrb_atomic_t;
typedef mrb_bool  mrb_atomic_bool_t;
#else
typedef volatile mrb_int   mrb_atomic_t;
typedef volatile mrb_bool  mrb_atomic_bool_t;
#endif

#endif

