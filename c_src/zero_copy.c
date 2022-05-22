#include <stdio.h>

#include "erl_nif.h"

const ssize_t ZCOPY_REVERSE_PROCESS_LIMIT = 10000;

typedef struct _ZCopyState {
  // pointer to input data
  unsigned char *in_buf;
  ssize_t in_size;
  // input data specific opaque obj, this will be passed during unref
  void *in_ref;
  // function to be called to unref input data
  void (*in_unref)(void *, void *);

  ssize_t pos;

  // output
  unsigned char *out_buf;
} ZCopyState;

static void zcopy_state_type_dtor(ErlNifEnv *env, void *obj) {
  ZCopyState *zcopy = (ZCopyState *)obj;
  enif_fprintf(stderr, "free zcopy resource\n");
  enif_free(zcopy->out_buf);
  return;
}

ErlNifResourceType *ZCOPY_STATE_TYPE;

static void erl_binary_unref(void *buf, ErlNifEnv *env) {
  enif_fprintf(stderr, "freed nif_env\n");
  // freeing env frees unref all terms it contains
  enif_free_env(env);
  return;
}

static int erl_binary_ref(ERL_NIF_TERM bin_term, ZCopyState *zcopy) {
  ErlNifBinary bin;
  ERL_NIF_TERM term;
  ErlNifEnv *new_env;

  zcopy->in_buf = NULL;
  zcopy->in_size = 0;
  zcopy->in_ref = NULL;

  // keep reference to binary by creating new nif-env and copying
  // binary-term reference to it
  new_env = enif_alloc_env();
  term = enif_make_copy(new_env, bin_term);

  if (!enif_inspect_binary(new_env, term, &bin)) {
    enif_free_env(new_env);
    return 1;
  }

  // Note that we are *NOT* copying the binary data
  zcopy->in_buf = bin.data;
  zcopy->in_size = bin.size;

  // input buffer specific opaque data which will be passed as second
  // argument to finalizer during unref
  zcopy->in_ref = (void *)new_env;
  // function to be called to unref the input data
  zcopy->in_unref = (void (*)(void *, void *))erl_binary_unref;

  return 0;
}

// NIF

static ERL_NIF_TERM zc_reverse_new(ErlNifEnv *env, int argc,
                                   const ERL_NIF_TERM argv[]) {
  ZCopyState *zcopy = NULL;
  ERL_NIF_TERM term;

  if (argc != 1) {
    enif_fprintf(stderr, "incorrect argument count. passed: %d\n", argc);
    return enif_make_badarg(env);
  }

  if (!enif_is_binary(env, argv[0])) {
    enif_fprintf(stderr, "failed to get binary\n");
    return enif_make_badarg(env);
  }

  zcopy = enif_alloc_resource(ZCOPY_STATE_TYPE, sizeof(ZCopyState));

  if (erl_binary_ref(argv[0], zcopy)) {
    enif_release_resource(zcopy);
    return enif_make_badarg(env);
  }

  // init output buffer
  zcopy->pos = 0;
  zcopy->out_buf = enif_alloc(zcopy->in_size);

  // transfer ownership to ERTS
  term = enif_make_resource(env, zcopy);
  enif_release_resource(zcopy);

  return term;
}

static ERL_NIF_TERM zc_reverse_process(ErlNifEnv *env, int argc,
                                       const ERL_NIF_TERM argv[]) {
  ZCopyState *zcopy = NULL;
  ERL_NIF_TERM out_bin_term;
  ssize_t size;

  if (argc != 1) {
    enif_fprintf(stderr, "incorrect argument count. passed: %d\n", argc);
    return enif_make_badarg(env);
  }

  if (!enif_get_resource(env, argv[0], ZCOPY_STATE_TYPE, (void **)&zcopy)) {
    enif_fprintf(stderr, "failed to get zcopy resource\n");
    return enif_make_badarg(env);
  }

  size = zcopy->in_size;

  for (ssize_t i = 0; i < ZCOPY_REVERSE_PROCESS_LIMIT && zcopy->pos < size;
       i++) {
    zcopy->out_buf[zcopy->pos] = zcopy->in_buf[size - zcopy->pos - 1];
    zcopy->pos += 1;
  }

  // unref input if we no longer need it
  if (zcopy->pos == size && zcopy->in_buf != NULL) {
    (zcopy->in_unref)(zcopy->in_buf, zcopy->in_ref);
    zcopy->in_ref = NULL;
    zcopy->in_buf = NULL;
  }

  // we are creating binary-term which is tied to zcopy resource
  // without allocating new memory. ERTS ensures that zcopy resource
  // destructor is called when all binary terms and resource terms are
  // no longer reachable
  out_bin_term =
      enif_make_resource_binary(env, zcopy, zcopy->out_buf, zcopy->pos);
  return out_bin_term;
}

static int on_load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {
  ZCOPY_STATE_TYPE =
      enif_open_resource_type(env, NULL, "zero-copy resource",
                              (ErlNifResourceDtor *)zcopy_state_type_dtor,
                              ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER, NULL);

  if (!ZCOPY_STATE_TYPE) {
    return 1;
  }

  return 0;
}

static ErlNifFunc nif_funcs[] = {
    {"zc_reverse_new", 1, zc_reverse_new, 0},
    {"zc_reverse_process", 1, zc_reverse_process, 0}};

ERL_NIF_INIT(Elixir.ZeroCopy.Nif, nif_funcs, &on_load, NULL, NULL, NULL)
