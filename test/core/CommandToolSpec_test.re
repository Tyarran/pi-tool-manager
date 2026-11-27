open Jest;
open Expect;

describe("CommandTool.Spec.decode", () => {
  test("decodes the command and its parameters", () => {
    let spec = Specs.jj;
    expect((
      spec.command,
      Names.Map.bindings(spec.parameters) |> List.map(fst),
      Names.Map.find_opt("message", spec.parameters),
      spec.timeout,
      spec.output,
    ))
    |> toEqual((
         CommandTool.Spec.Argv([
           Literal("~/bin/jj"),
           Placeholder("subcommand"),
           Placeholder("message"),
           Placeholder("verbose"),
           Placeholder("limit"),
           Placeholder("args"),
         ]),
         ["args", "limit", "message", "subcommand", "verbose"],
         Some(
           {
             kind:
               String({
                 enum: None,
                 pattern: None,
                 deny: [],
               }),
             description: Some("Message"),
             requirement:
               RequiredWhen(
                 Names.Map.singleton("subcommand", ["describe"]),
               ),
             flag: Some("-m"),
           }: CommandTool.Spec.param,
         ),
         120,
         {
           CommandTool.Spec.maxLines: 2000,
           maxBytes: 51200,
         },
       ));
  });

  test("keeps other braces of a shell command", () =>
    expect(Specs.logs.command)
    |> toEqual(
         CommandTool.Spec.Shell(
           "journalctl -u {unit} {since} | tail -n {lines} | awk '{print $1}'",
         ),
       )
  );

  testAll(
    "reports invalid definitions",
    [
      ({|{"command": ["ls"]}|}, "tools.tool.description is required"),
      ({|{"description": "d"}|}, "tools.tool.command is required"),
      (
        {|{"description": "d", "command": []}|},
        "tools.tool.command must not be empty",
      ),
      (
        {|{"description": "d", "command": "ls | wc"}|},
        "tools.tool.command must be an array of strings; set shell to true to run a shell command",
      ),
      (
        {|{"description": "d", "shell": true, "command": ["ls"]}|},
        "tools.tool.command must be a string",
      ),
      (
        {|{"description": "d", "shell": true, "command": " "}|},
        "tools.tool.command must not be empty",
      ),
      (
        {|{"description": "d", "command": ["{dir}"], "parameters": {"dir": {"type": "string"}}}|},
        "tools.tool.command[0] must be the literal program to run",
      ),
      (
        {|{"description": "d", "command": ["ls", "--dir={dir}"], "parameters": {"dir": {"type": "string"}}}|},
        "tools.tool.command[1] placeholders must be standalone tokens such as \"{name}\"",
      ),
      (
        {|{"description": "d", "command": ["ls", "{dir}"]}|},
        "tools.tool uses an undeclared parameter \"dir\"",
      ),
      (
        {|{"description": "d", "command": ["ls"], "parameters": {"dir": {"type": "string"}}}|},
        "tools.tool declares an unused parameter \"dir\"",
      ),
      (
        {|{"description": "d", "shell": true, "command": "ls {di}", "parameters": {"dir": {"type": "string"}}}|},
        "tools.tool declares an unused parameter \"dir\"",
      ),
      (
        {|{"description": "d", "command": ["ls", "{dir}"], "parameters": {"dir": {"type": "string", "requiredWhen": {"mode": ["x"]}}}}|},
        "tools.tool requiredWhen refers to an unknown parameter \"mode\"",
      ),
      (
        {|{"description": "d", "command": ["ls", "{dir}"], "parameters": {"dir": {"type": "string", "required": true, "requiredWhen": {}}}}|},
        "tools.tool.parameters.dir cannot combine required and requiredWhen",
      ),
      (
        {|{"description": "d", "command": ["ls", "{dir}"], "parameters": {"dir": {"type": "path"}}}|},
        "tools.tool.parameters.dir.type must be one of \"string\", \"integer\", \"number\", \"boolean\", \"array\"",
      ),
      (
        {|{"description": "d", "command": ["ls", "{n}"], "parameters": {"n": {"type": "integer", "deny": ["1"]}}}|},
        "tools.tool.parameters.n.deny only applies to string and array parameters",
      ),
      (
        {|{"description": "d", "command": ["ls", "{dir}"], "parameters": {"dir": {"type": "string", "enum": []}}}|},
        "tools.tool.parameters.dir.enum must not be empty",
      ),
      (
        {|{"description": "d", "command": ["ls"], "timeout": 0}|},
        "tools.tool.timeout must be a positive integer",
      ),
      (
        {|{"description": "d", "command": ["ls"], "env": {"A": 1}}|},
        "tools.tool.env.A must be a string",
      ),
    ],
    ((text, expected)) =>
    expect(Specs.decode(text) |> Helpers.message) |> toEqual(expected)
  );
});
