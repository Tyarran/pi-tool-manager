let decode = (~name="tool", text) =>
  CommandTool.Spec.decode(name, "tools." ++ name, Helpers.json(text));

let make = text =>
  switch (decode(text)) {
  | Ok(spec) => spec
  | Error(message) => failwith(message)
  };

let jj =
  make(
    {|{
  "description": "Run jj",
  "command": ["~/bin/jj", "{subcommand}", "{message}", "{verbose}", "{limit}", "{args}"],
  "parameters": {
    "subcommand": {"type": "string", "enum": ["status", "describe"], "required": true},
    "message": {"type": "string", "flag": "-m", "requiredWhen": {"subcommand": ["describe"]}, "description": "Message"},
    "verbose": {"type": "boolean", "flag": "--verbose"},
    "limit": {"type": "integer", "flag": "-n"},
    "args": {"type": "array", "deny": ["--config", "-R"], "pattern": "^[^;]*$"}
  },
  "env": {"JJ_EDITOR": "false", "JJ_MESSAGE": "{message}", "HOME_DIR": "~"},
  "cwd": "~/repo"
}|},
  );

let logs =
  make(
    {|{
  "description": "Logs",
  "shell": true,
  "command": "journalctl -u {unit} {since} | tail -n {lines} | awk '{print $1}'",
  "parameters": {
    "unit": {"type": "string", "required": true},
    "since": {"type": "string", "flag": "--since"},
    "lines": {"type": "integer"}
  }
}|},
  );

let arguments = (spec, text) =>
  CommandTool.Arguments.decode(spec, Helpers.fields(text));
