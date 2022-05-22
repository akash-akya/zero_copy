defmodule ZeroCopy.Nif do
  @moduledoc false
  @on_load :load_nifs

  def load_nifs do
    nif_path = :filename.join(:code.priv_dir(:vix), "zero_copy")
    :erlang.load_nif(nif_path, 0)
  end

  # def nif_target_new,
  #   do: :erlang.nif_error(:nif_library_not_loaded)
end
