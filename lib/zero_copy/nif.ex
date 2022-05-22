defmodule ZeroCopy.Nif do
  @moduledoc false
  @on_load :load_nifs

  def load_nifs do
    nif_path = :filename.join(:code.priv_dir(:zero_copy), "zero_copy")
    :erlang.load_nif(nif_path, 0)
  end

  def zc_reverse_new(_binary), do: :erlang.nif_error(:nif_library_not_loaded)

  def zc_reverse_process(_zcopy_state), do: :erlang.nif_error(:nif_library_not_loaded)
end
