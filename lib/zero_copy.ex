defmodule ZeroCopy do
  alias ZeroCopy.Nif

  def new(binary) do
    Nif.zc_reverse_new(binary)
  end

  def process(zc_state) do
    Nif.zc_reverse_process(zc_state)
  end

  def reverse(zc_state, size) do
    rev = process(zc_state)

    processed_size = IO.iodata_length(rev)
    IO.puts("processed size: #{processed_size}")

    if processed_size == size do
      rev
    else
      reverse(zc_state, size)
    end
  end

  def run do
    rev =
      Task.async(fn ->
        # 100 Mb
        binary = :crypto.strong_rand_bytes(100_000 * 1024)
        zc_state = new(binary)

        # pause for a while to capture memory usage
        Process.sleep(2000)

        reverse(zc_state, IO.iodata_length(binary))
      end)
      |> Task.await(:infinity)

    true = :erlang.garbage_collect()

    rev
  end
end
