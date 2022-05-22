#include <stdio.h>

#include "erl_nif.h"

static int on_load(ErlNifEnv *env, void **priv, ERL_NIF_TERM load_info) {
  return 0;
}

static ErlNifFunc nif_funcs[] = {};

ERL_NIF_INIT(Elixir.ZeroCopy.Nif, nif_funcs, &on_load, NULL, NULL, NULL)
