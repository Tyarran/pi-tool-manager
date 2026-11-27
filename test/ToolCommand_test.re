open Jest;
open Expect;

let spec =
  switch (
    ToolSpec.decode(
      "tools",
      Js.Json.parseExn(
        {|{"jj": {
  "description": "Run jj",
  "command": ["~/bin/jj", "{subcommand}", "{message}", "{verbose}", "{limit}", "{args}"],
  "parameters": {
    "subcommand": {"type": "string", "enum": ["status", "describe"], "required": true},
    "message": {"type": "string", "flag": "-m", "requiredWhen": {"subcommand": ["describe"]}, "description": "Message"},
    "verbose": {"type": "boolean", "flag": "--verbose"},
    "limit": {"type": "integer", "flag": "-n"},
    "args": {"type": "array", "deny": ["--config", "-R"], "pattern": "^[^;]*$"}
  },
  "env": {"JJ_EDITOR": "false", "JJ_MESSAGE": "{message}", "HOME_DIR": "~"}
}}|},
      ),
    )
  ) {
  | Ok([spec]) => spec
  | _ => failwith("invalid test spec")
  };

let args = text =>
  switch (Js.Json.classify(Js.Json.parseExn(text))) {
  | JSONObject(dict) => dict
  | _ => failwith("arguments must be an object")
  };

let validate = text => ToolCommand.validate(spec, args(text));

let render = text =>
  validate(text)
  |> Result.map(values => ToolCommand.render(spec, values, ~home="/home/me"));

describe("ToolCommand.validate", () => {
  test("accepts valid arguments", () =>
    expect(validate({|{"subcommand": "describe", "message": "fix"}|}))
    |> toEqual(
         Ok([
           ("subcommand", ToolCommand.Text("describe")),
           ("message", ToolCommand.Text("fix")),
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
      (
        {|{"subcommand": "status", "limit": 1.5}|},
        "limit must be an integer",
      ),
      (
        {|{"subcommand": "status", "args": [1]}|},
        "args must only contain strings",
      ),
    ],
    ((text, expected)) =>
    expect(validate(text)) |> toEqual(Error(expected))
  );

  test("ignores null values", () =>
    expect(validate({|{"subcommand": "status", "message": null}|}))
    |> toEqual(Ok([("subcommand", ToolCommand.Text("status"))]))
  );
});

describe("ToolCommand.render", () => {
  test("drops absent parameters and expands the home of literals", () =>
    expect(render({|{"subcommand": "status"}|}))
    |> toEqual(Ok(["/home/me/bin/jj", "status"]))
  );

  test("prefixes values with their flag and spreads arrays", () =>
    expect(
      render(
        {|{"subcommand": "describe", "message": "~ fix", "verbose": true, "limit": 3, "args": ["-r", "@"]}|},
      ),
    )
    |> toEqual(
         Ok([
           "/home/me/bin/jj",
           "describe",
           "-m",
           "~ fix",
           "--verbose",
           "-n",
           "3",
           "-r",
           "@",
         ]),
       )
  );

  test("omits a false boolean flag", () =>
    expect(render({|{"subcommand": "status", "verbose": false}|}))
    |> toEqual(Ok(["/home/me/bin/jj", "status"]))
  );
});

describe("ToolCommand.environment", () =>
  test("sets literals and present parameters", () =>
    expect(
      validate({|{"subcommand": "describe", "message": "fix"}|})
      |> Result.map(values =>
           ToolCommand.environment(spec, values, ~home="/home/me")
         ),
    )
    |> toEqual(
         Ok([
           ("JJ_EDITOR", "false"),
           ("JJ_MESSAGE", "fix"),
           ("HOME_DIR", "/home/me"),
         ]),
       )
  )
);

describe("ToolCommand.schema", () =>
  test("builds a JSON schema with constraints and hints", () =>
    expect(Js.Json.stringify(ToolCommand.schema(spec)))
    |> toEqual(
         {|{"type":"object","properties":{"subcommand":{"type":"string","enum":["status","describe"]},"message":{"type":"string","description":"Message Required when subcommand is describe."},"verbose":{"type":"boolean"},"limit":{"type":"integer"},"args":{"type":"array","items":{"type":"string","pattern":"^[^;]*$"},"description":"Not allowed: --config, -R."}},"required":["subcommand"],"additionalProperties":false}|},
       )
  )
);
