open Jest;
open Expect;

let selection = Helpers.names(["read", "edit"]);

describe("Proxy.checkCall", () =>
  testAll(
    "checks the tool and the intent",
    [
      ("read", "Inspect the file", Ok()),
      ("read", " ", Error("intent must explain why the tool is called")),
      (
        "bash",
        "Run it",
        Error("Tool \"bash\" is not available. Available tools: edit, read"),
      ),
    ],
    ((tool, intent, expected)) =>
    expect(Proxy.checkCall(~selection, ~tool, ~intent)) |> toEqual(expected)
  )
);
