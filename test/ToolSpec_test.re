open Jest;
open Expect;

let decode = text =>
  ToolSpec.decode("tools", Js.Json.parseExn(text))
  |> Result.map(specs => List.hd(specs));

let message = result =>
  switch (result) {
  | Ok(_) => "ok"
  | Error(message) => message
  };

let jj = {|{"jj": {
  "description": "Run jj",
  "command": ["jj", "{subcommand}", "{message}", "{args}", "--no-pager"],
  "parameters": {
    "subcommand": {"type": "string", "enum": ["status", "describe"], "required": true, "description": "Subcommand"},
    "message": {"type": "string", "flag": "-m", "requiredWhen": {"subcommand": ["describe"]}},
    "args": {"type": "array", "deny": ["--config", "-R"]}
  },
  "timeout": 30,
  "cwd": "~/repo",
  "env": {"JJ_EDITOR": "false", "JJ_USER": "{message}"},
  "maxLines": 100
}}|};

describe("ToolSpec.decode", () => {
  test("decodes a full definition", () =>
    expect(decode(jj))
    |> toEqual(
         Ok(
           {
             name: "jj",
             description: "Run jj",
             command: [
               Literal("jj"),
               Placeholder("subcommand"),
               Placeholder("message"),
               Placeholder("args"),
               Literal("--no-pager"),
             ],
             parameters: [
               {
                 name: "subcommand",
                 type_: String,
                 description: Some("Subcommand"),
                 required: true,
                 requiredWhen: [],
                 enum: Some(["status", "describe"]),
                 pattern: None,
                 deny: [],
                 flag: None,
               },
               {
                 name: "message",
                 type_: String,
                 description: None,
                 required: false,
                 requiredWhen: [("subcommand", ["describe"])],
                 enum: None,
                 pattern: None,
                 deny: [],
                 flag: Some("-m"),
               },
               {
                 name: "args",
                 type_: Array,
                 description: None,
                 required: false,
                 requiredWhen: [],
                 enum: None,
                 pattern: None,
                 deny: ["--config", "-R"],
                 flag: None,
               },
             ],
             timeout: 30,
             cwd: Some("~/repo"),
             env: [
               ("JJ_EDITOR", Literal("false")),
               ("JJ_USER", Placeholder("message")),
             ],
             maxLines: Some(100),
             maxBytes: None,
           }: ToolSpec.t,
         ),
       )
  );

  test("applies defaults", () =>
    expect(
      decode({|{"ls": {"description": "List", "command": ["ls", "-la"]}}|})
      |> Result.map((spec: ToolSpec.t) =>
           (spec.timeout, spec.parameters, spec.env)
         ),
    )
    |> toEqual(Ok((120, [], [])))
  );

  testAll(
    "reports invalid definitions",
    [
      ({|{"ls": {"command": ["ls"]}}|}, "tools.ls.description is required"),
      ({|{"ls": {"description": "List"}}|}, "tools.ls.command is required"),
      (
        {|{"ls": {"description": "List", "command": []}}|},
        "tools.ls.command must not be empty",
      ),
      (
        {|{"ls": {"description": "List", "command": ["{dir}"], "parameters": {"dir": {"type": "string"}}}}|},
        "tools.ls.command[0] must be the literal program to run",
      ),
      (
        {|{"ls": {"description": "List", "command": ["ls", "--dir={dir}"], "parameters": {"dir": {"type": "string"}}}}|},
        "tools.ls.command[1] placeholders must be standalone tokens such as \"{name}\"",
      ),
      (
        {|{"ls": {"description": "List", "command": ["ls", "{dir}"]}}|},
        "tools.ls uses an undeclared parameter \"dir\"",
      ),
      (
        {|{"ls": {"description": "List", "command": ["ls"], "parameters": {"dir": {"type": "string"}}}}|},
        "tools.ls declares an unused parameter \"dir\"",
      ),
      (
        {|{"ls": {"description": "List", "command": ["ls", "{dir}"], "parameters": {"dir": {"type": "string", "requiredWhen": {"mode": ["x"]}}}}}|},
        "tools.ls requiredWhen refers to an unknown parameter \"mode\"",
      ),
      (
        {|{"ls": {"description": "List", "command": ["ls", "{dir}"], "parameters": {"dir": {"type": "path"}}}}|},
        "tools.ls.parameters.dir.type must be one of \"string\", \"integer\", \"number\", \"boolean\", \"array\"",
      ),
      (
        {|{"ls": {"description": "List", "command": ["ls", "{n}"], "parameters": {"n": {"type": "integer", "enum": ["1"]}}}}|},
        "tools.ls.parameters.n.enum only applies to string and array parameters",
      ),
      (
        {|{"ls": {"description": "List", "command": ["ls", "{dir}"], "parameters": {"dir": {"type": "string", "enum": []}}}}|},
        "tools.ls.parameters.dir.enum must not be empty",
      ),
      (
        {|{"ls": {"description": "List", "command": ["ls"], "timeout": 0}}|},
        "tools.ls.timeout must be a positive integer",
      ),
      (
        {|{"ls": {"description": "List", "command": ["ls"], "env": {"A": 1}}}|},
        "tools.ls.env.A must be a string",
      ),
    ],
    ((text, expected)) =>
    expect(message(decode(text))) |> toEqual(expected)
  );
});
