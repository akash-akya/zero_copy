defmodule ZeroCopyTest do
  use ExUnit.Case

  test "reverse" do
    size = 100 * 1024
    binary = :crypto.strong_rand_bytes(size)
    reversed = ZeroCopy.reverse(ZeroCopy.new(binary), size)

    assert reversed == IO.iodata_to_binary(do_reverse(binary))
  end

  defp do_reverse(<<>>), do: []

  defp do_reverse(<<byte::8, rest::binary>>) do
    [do_reverse(rest), <<byte::8>>]
  end
end
