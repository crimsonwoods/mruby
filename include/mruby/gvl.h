/*
** mruby/gvl.h - Global VM Lock
**
** See Copyright Notice in mruby.h
*/

#ifdef MRB_USE_GVL_API
#ifndef MRB_GVL_H
#define MRB_GVL_H

#include "mruby.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mrb_gvl_t;
typedef struct mrb_gvl_t mrb_gvl_t;

typedef struct mrb_blocking_region_t {
  mrb_bool is_gvl_released;
} mrb_blocking_region_t;

MRB_API void mrb_gvl_init(mrb_state *mrb);
MRB_API void mrb_gvl_cleanup(mrb_state *mrb);
MRB_API void mrb_gvl_acquire(mrb_state *mrb);
MRB_API void mrb_gvl_release(mrb_state *mrb);
MRB_API void mrb_gvl_yield(mrb_state *mrb);
MRB_API mrb_bool mrb_gvl_is_acquired(mrb_state *mrb);
MRB_API void mrb_gvl_acquire_dbg(mrb_state *mrb, char const *file, int line, char const *func);
MRB_API void mrb_gvl_release_dbg(mrb_state *mrb, char const *file, int line, char const *func);
MRB_API void mrb_blocking_region_begin(mrb_state *mrb, mrb_blocking_region_t *region);
MRB_API void mrb_blocking_region_end(mrb_state *mrb, mrb_blocking_region_t *region);

#define MRB_BLOCKING_REGION(mrb, exec) \
{ \
  mrb_blocking_region_t region; \
  mrb_blocking_region_begin(mrb, &region); \
  exec; \
  mrb_blocking_region_end(mrb, &region); \
}

#ifdef MRB_GVL_DEBUG
#define mrb_gvl_acquire(mrb) mrb_gvl_acquire_dbg(mrb, __FILE__, __LINE__, __func__)
#define mrb_gvl_release(mrb) mrb_gvl_release_dbg(mrb, __FILE__, __LINE__, __func__)
#endif

#ifdef __cplusplus
}
#endif

#else

typedef struct mrb_blocking_region_t {
} mrb_blocking_region_t;

#define mrb_gvl_init(mrb)
#define mrb_gvl_cleanup(mrb)
#define mrb_gvl_acquire(mrb)
#define mrb_gvl_release(mrb)
#define mrb_gvl_is_acquired(mrb) (FALSE)
#define mrb_gvl_acquire_dbg(mrb, file, line, func)
#define mrb_gvl_release_dbg(mrb, file, line, func)
#define mrb_blocking_region_begin(mrb, region) ((void)region)
#define mrb_blocking_region_end(mrb, region)   ((void)region)

#define MRB_BLOCKING_REGION(mrb, exec) { exec; }

#endif
#endif
