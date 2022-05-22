defmodule ZeroCopyTest do
  use ExUnit.Case
  doctest ZeroCopy

  test "greets the world" do
    assert ZeroCopy.hello() == :world
  end
end
