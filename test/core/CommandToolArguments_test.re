open Jest;
open Expect;

let decode = text =>
  Specs.arguments(Specs.jj, text) |> Result.map(Names.Map.bindings);

describe("CommandTool.Arguments.decode", () => {
  test("decodes the provided arguments", () =>
    expect(
      decode(
        {|{"subcommand": "describe", "message": "fix", "limit": 2, "verbose": null}|},
      ),
    )
    |> toEqual(
         Ok([
           ("limit", CommandTool.Arguments.Int(2)),
           ("message", CommandTool.Arguments.Text("fix")),
           ("subcommand", CommandTool.Arguments.Text("describe")),
         ]),
       )
  );

  testAll(
    "rejects invalid arguments",
    [
      ({|{}|}, "subcommand is required"),
      (
        {|{"subcommand": "log"}|},
        "subcommand must be one of status, describe, got \"log\"",
      ),
      (
        {|{"subcommand": "describe"}|},
        "message is required when subcommand is \"describe\"",
      ),
      (
        {|{"subcommand": "status", "args": ["--config"]}|},
        "args does not allow \"--config\"",
      ),
      (
        {|{"subcommand": "status", "args": ["--config=x"]}|},
        "args does not allow \"--config=x\"",
      ),
      (
        {|{"subcommand": "status", "args": ["-iR"]}|},
        "args does not allow \"-iR\"",
      ),
      (
        {|{"subcommand": "status", "args": ["a;b"]}|},
        "args must match ^[^;]*$, got \"a;b\"",
      ),
      ({|{"subcommand": "status", "args": [1]}|}, "args[0] must be a string"),
      (
        {|{"subcommand": "status", "limit": 1.5}|},
        "limit must be an integer",
      ),
      (
        {|{"subcommand": "status", "verbose": "yes"}|},
        "verbose must be a boolean",
      ),
    ],
    ((text, expected)) =>
    expect(decode(text) |> Helpers.message) |> toEqual(expected)
  );
});
