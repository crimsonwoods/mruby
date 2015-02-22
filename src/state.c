/*
** state.c - mrb_state open/close functions
**
** See Copyright Notice in mruby.h
*/

#include <stdlib.h>
#include <string.h>
#include "mruby.h"
#include "mruby/irep.h"
#include "mruby/variable.h"
#include "mruby/debug.h"
#include "mruby/string.h"
#include "mruby/thread.h"
#include "mruby/gvl.h"

void mrb_init_heap(mrb_state*);
void mrb_init_core(mrb_state*);
void mrb_init_mrbgems(mrb_state*);

static mrb_value
inspect_main(mrb_state *mrb, mrb_value mod)
{
  return mrb_str_new_lit(mrb, "main");
}

static mrb_thread_context*
mrb_alloc_thread_context(mrb_state *mrb)
{
  static const mrb_thread_context mrb_thread_context_zero = { 0 };
  static const struct mrb_context mrb_context_zero = { 0 };
  mrb_thread_context *context = (mrb_thread_context*)mrb_malloc(mrb, sizeof(mrb_thread_context));
  *context = mrb_thread_context_zero;
#ifndef MRB_GC_FIXED_ARENA
  context->arena = (struct RBasic**)mrb_malloc(mrb, sizeof(struct RBasic*)*MRB_GC_ARENA_SIZE);
  context->arena_capa = MRB_GC_ARENA_SIZE;
#endif
  context->c = (struct mrb_context*)mrb_malloc(mrb, sizeof(struct mrb_context));
  *context->c = mrb_context_zero;
  context->root_c = context->c;
  return context;
}

MRB_API mrb_state*
mrb_open_core(mrb_allocf f, void *ud)
{
  static const mrb_vm_context mrb_vm_context_zero = { 0 };
  mrb_state *mrb;

  mrb = (mrb_state *)(f)(NULL, NULL, sizeof(mrb_state), ud);
  if (mrb == NULL) return NULL;

  mrb->allocf_ud = ud;
  mrb->allocf = f;

  MRB_GET_VM(mrb) = (mrb_vm_context *)(f)(NULL, NULL, sizeof(mrb_vm_context), ud);
  if (MRB_GET_VM(mrb) == NULL) {
    mrb_free(mrb, mrb);
    return NULL;
  }
  *MRB_GET_VM(mrb) = mrb_vm_context_zero;
  MRB_GET_VM(mrb)->current_white_part = MRB_GC_WHITE_A;
  MRB_GET_VM(mrb)->atexit_stack_len = 0;

  MRB_GET_THREAD_CONTEXT(mrb) = mrb_alloc_thread_context(mrb);
#ifdef MRB_USE_THREAD_API
  MRB_GET_VM(mrb)->threads[0] = MRB_GET_THREAD_CONTEXT(mrb);
  MRB_GET_VM(mrb)->thread_count = 1;
#endif

  mrb_init_heap(mrb);

  mrb_init_core(mrb);

  return mrb;
}

#ifdef MRB_USE_THREAD_API
MRB_API mrb_state*
mrb_duplicate_core(mrb_state *mrb)
{
  mrb_state *dup;
  dup = (mrb_state *)mrb_malloc(mrb, sizeof(mrb_state));
  *dup = *mrb;
  MRB_GET_THREAD_CONTEXT(dup) = mrb_alloc_thread_context(mrb);
  MRB_GET_THREAD_CONTEXT(dup)->id = ++MRB_GET_VM(mrb)->thread_index;
  return dup;
}
#endif

void*
mrb_default_allocf(mrb_state *mrb, void *p, size_t size, void *ud)
{
  if (size == 0) {
    free(p);
    return NULL;
  }
  else {
    return realloc(p, size);
  }
}

struct alloca_header {
  struct alloca_header *next;
  char buf[];
};

MRB_API void*
mrb_alloca(mrb_state *mrb, size_t size)
{
  struct alloca_header *p;

  p = (struct alloca_header*) mrb_malloc(mrb, sizeof(struct alloca_header)+size);
  p->next = MRB_GET_VM(mrb)->mems;
  MRB_GET_VM(mrb)->mems = p;
  return (void*)p->buf;
}

static void
mrb_alloca_free(mrb_state *mrb)
{
  struct alloca_header *p;
  struct alloca_header *tmp;

  if (mrb == NULL) return;
  p = MRB_GET_VM(mrb)->mems;

  while (p) {
    tmp = p;
    p = p->next;
    mrb_free(mrb, tmp);
  }
}

MRB_API mrb_state*
mrb_open(void)
{
  mrb_state *mrb = mrb_open_allocf(mrb_default_allocf, NULL);

  return mrb;
}

MRB_API mrb_state*
mrb_open_allocf(mrb_allocf f, void *ud)
{
  mrb_state *mrb = mrb_open_core(f, ud);

  if (mrb == NULL) {
    return NULL;
  }

#ifndef DISABLE_GEMS
  mrb_init_mrbgems(mrb);
  mrb_gc_arena_restore(mrb, 0);
#endif
#ifdef MRB_USE_GVL_API
  mrb_gvl_init(mrb);
#endif
#if defined(MRB_USE_THREAD_API) && defined(MRB_USE_GVL_API)
  mrb_timer_thread_create(mrb);
#endif
  return mrb;
}

void mrb_free_symtbl(mrb_state *mrb);
void mrb_free_heap(mrb_state *mrb);

void
mrb_irep_incref(mrb_state *mrb, mrb_irep *irep)
{
  irep->refcnt++;
}

void
mrb_irep_decref(mrb_state *mrb, mrb_irep *irep)
{
  irep->refcnt--;
  if (irep->refcnt == 0) {
    mrb_irep_free(mrb, irep);
  }
}

void
mrb_irep_free(mrb_state *mrb, mrb_irep *irep)
{
  size_t i;

  if (!(irep->flags & MRB_ISEQ_NO_FREE))
    mrb_free(mrb, irep->iseq);
  for (i=0; i<irep->plen; i++) {
    if (mrb_type(irep->pool[i]) == MRB_TT_STRING) {
      mrb_gc_free_str(mrb, RSTRING(irep->pool[i]));
      mrb_free(mrb, mrb_obj_ptr(irep->pool[i]));
    }
#ifdef MRB_WORD_BOXING
    else if (mrb_type(irep->pool[i]) == MRB_TT_FLOAT) {
      mrb_free(mrb, mrb_obj_ptr(irep->pool[i]));
    }
#endif
  }
  mrb_free(mrb, irep->pool);
  mrb_free(mrb, irep->syms);
  for (i=0; i<irep->rlen; i++) {
    mrb_irep_decref(mrb, irep->reps[i]);
  }
  mrb_free(mrb, irep->reps);
  mrb_free(mrb, irep->lv);
  mrb_free(mrb, (void *)irep->filename);
  mrb_free(mrb, irep->lines);
  mrb_debug_info_free(mrb, irep->debug_info);
  mrb_free(mrb, irep);
}

mrb_value
mrb_str_pool(mrb_state *mrb, mrb_value str)
{
  struct RString *s = mrb_str_ptr(str);
  struct RString *ns;
  char *ptr;
  mrb_int len;

  ns = (struct RString *)mrb_malloc(mrb, sizeof(struct RString));
  ns->tt = MRB_TT_STRING;
  ns->c = MRB_GET_VM(mrb)->string_class;

  if (RSTR_NOFREE_P(s)) {
    ns->flags = MRB_STR_NOFREE;
    ns->as.heap.ptr = s->as.heap.ptr;
    ns->as.heap.len = s->as.heap.len;
    ns->as.heap.aux.capa = 0;
  }
  else {
    ns->flags = 0;
    if (RSTR_EMBED_P(s)) {
      ptr = s->as.ary;
      len = RSTR_EMBED_LEN(s);
    }
    else {
      ptr = s->as.heap.ptr;
      len = s->as.heap.len;
    }

    if (len < RSTRING_EMBED_LEN_MAX) {
      RSTR_SET_EMBED_FLAG(ns);
      RSTR_SET_EMBED_LEN(ns, len);
      if (ptr) {
        memcpy(ns->as.ary, ptr, len);
      }
      ns->as.ary[len] = '\0';
    }
    else {
      ns->as.heap.ptr = (char *)mrb_malloc(mrb, (size_t)len+1);
      ns->as.heap.len = len;
      ns->as.heap.aux.capa = len;
      if (ptr) {
        memcpy(ns->as.heap.ptr, ptr, len);
      }
      ns->as.heap.ptr[len] = '\0';
    }
  }
  return mrb_obj_value(ns);
}

MRB_API void
mrb_free_context(mrb_state *mrb, struct mrb_context *c)
{
  if (!c) return;
  mrb_free(mrb, c->stbase);
  mrb_free(mrb, c->cibase);
  mrb_free(mrb, c->rescue);
  mrb_free(mrb, c->ensure);
  mrb_free(mrb, c);
}

MRB_API void
mrb_close(mrb_state *mrb)
{
#if defined MRB_USE_THREAD_API && defined MRB_USE_GVL_API
  mrb_timer_thread_destroy(mrb);
#endif

  if (MRB_GET_VM(mrb)->atexit_stack_len > 0) {
    mrb_int i;
    for (i = MRB_GET_VM(mrb)->atexit_stack_len; i > 0; --i) {
      MRB_GET_VM(mrb)->atexit_stack[i - 1](mrb);
    }
#ifndef MRB_FIXED_STATE_ATEXIT_STACK
    mrb_free(mrb, MRB_GET_VM(mrb)->atexit_stack);
#endif
  }

  /* free */
  mrb_gc_free_gv(mrb);
  mrb_free_context(mrb, MRB_GET_ROOT_CONTEXT(mrb));
  mrb_free_symtbl(mrb);
  mrb_free_heap(mrb);
#ifdef MRB_USE_GVL_API
  mrb_gvl_cleanup(mrb);
#endif
  mrb_alloca_free(mrb);
#ifndef MRB_GC_FIXED_ARENA
  mrb_free(mrb, MRB_GET_THREAD_CONTEXT(mrb)->arena);
#endif
  mrb_free(mrb, MRB_GET_THREAD_CONTEXT(mrb));
#ifdef MRB_USE_MUTEX_API
  mrb_free(mrb, MRB_GET_VM(mrb)->mutex_api);
#endif
#ifdef MRB_USE_THREAD_API
  mrb_free(mrb, MRB_GET_VM(mrb)->thread_api);
#endif
  mrb_free(mrb, MRB_GET_VM(mrb));
  mrb_free(mrb, mrb);
}

#ifdef MRB_USE_THREAD_API
MRB_API void
mrb_close_duplicated(mrb_state *mrb)
{
  mrb_free_context(mrb, MRB_GET_ROOT_CONTEXT(mrb));
#ifndef MRB_GC_FIXED_ARENA
  mrb_free(mrb, MRB_GET_THREAD_CONTEXT(mrb)->arena);
#endif
  mrb_free(mrb, MRB_GET_THREAD_CONTEXT(mrb));
  mrb_free(mrb, mrb);
}
#endif

MRB_API mrb_irep*
mrb_add_irep(mrb_state *mrb)
{
  static const mrb_irep mrb_irep_zero = { 0 };
  mrb_irep *irep;

  irep = (mrb_irep *)mrb_malloc(mrb, sizeof(mrb_irep));
  *irep = mrb_irep_zero;
  irep->refcnt = 1;

  return irep;
}

MRB_API mrb_value
mrb_top_self(mrb_state *mrb)
{
  if (!MRB_GET_VM(mrb)->top_self) {
    MRB_GET_VM(mrb)->top_self = (struct RObject*)mrb_obj_alloc(mrb, MRB_TT_OBJECT, MRB_GET_VM(mrb)->object_class);
    mrb_define_singleton_method(mrb, MRB_GET_VM(mrb)->top_self, "inspect", inspect_main, MRB_ARGS_NONE());
    mrb_define_singleton_method(mrb, MRB_GET_VM(mrb)->top_self, "to_s", inspect_main, MRB_ARGS_NONE());
  }
  return mrb_obj_value(MRB_GET_VM(mrb)->top_self);
}

MRB_API void
mrb_state_atexit(mrb_state *mrb, mrb_atexit_func f)
{
#ifdef MRB_FIXED_STATE_ATEXIT_STACK
  if (MRB_GET_VM(mrb)->atexit_stack_len + 1 > MRB_FIXED_STATE_ATEXIT_STACK_SIZE) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "exceeded fixed state atexit stack limit");
  }
#else
  size_t stack_size;

  stack_size = sizeof(mrb_atexit_func) * (MRB_GET_VM(mrb)->atexit_stack_len + 1);
  if (MRB_GET_VM(mrb)->atexit_stack_len == 0) {
    MRB_GET_VM(mrb)->atexit_stack = (mrb_atexit_func*)mrb_malloc(mrb, stack_size);
  } else {
    MRB_GET_VM(mrb)->atexit_stack = (mrb_atexit_func*)mrb_realloc(mrb, MRB_GET_VM(mrb)->atexit_stack, stack_size);
  }
#endif

  MRB_GET_VM(mrb)->atexit_stack[MRB_GET_VM(mrb)->atexit_stack_len++] = f;
}
