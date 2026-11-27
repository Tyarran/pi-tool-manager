# Command-line tools

Each entry of the `tools` key registers a tool that runs a command. The agent only fills typed parameters: it never sees nor chooses the command line, and by default the command runs **without a shell**, so values are passed as separate arguments and are never interpreted.

```json
"jj": {
  "description": "Run a jj command in the current repository",
  "command": ["jj", "{subcommand}", "{message}", "{args}", "--color=never", "--no-pager"],
  "parameters": {
    "subcommand": { "type": "string", "enum": ["status", "log", "diff", "describe"], "required": true },
    "message": { "type": "string", "flag": "-m", "requiredWhen": { "subcommand": ["describe"] } },
    "args": { "type": "array", "deny": ["--config", "--config-file", "-R", "--repository"] }
  },
  "env": { "JJ_EDITOR": "false" },
  "timeout": 120
}
```

Remember to add the tool to `baseTools` or to a preset: like any other tool, it is only reachable when selected.

## Tool definition

| Key | Type | Default | Description |
|---|---|---|---|
| `description` | string | — | Required. Shown to the agent. |
| `command` | string[] | — | Required. The first token is the program; `{name}` tokens are replaced by parameters. A placeholder must be a whole token. `~` at the start of a literal token is the home directory. A string in [shell mode](#shell-mode). |
| `shell` | boolean | `false` | Run `command` through `/bin/sh`. |
| `parameters` | object | `{}` | Parameter name → definition, described below. Every parameter must be used, every placeholder declared. |
| `timeout` | integer | `120` | Seconds before the process group is killed. |
| `cwd` | string | project directory | Working directory, relative to the project directory. |
| `env` | object | `{}` | Extra environment variables: literal strings or a `{name}` placeholder. |
| `maxLines` / `maxBytes` | integer | `2000` / `51200` | Output kept for each stream; the full output is saved to a temporary file when truncated. |

## Parameter definition

| Key | Description |
|---|---|
| `type` | Required: `string`, `integer`, `number`, `boolean` or `array` (of strings). |
| `description` | Shown to the agent. |
| `required` | The parameter must be provided. |
| `requiredWhen` | `{ "other": ["value", …] }`: required when another string parameter has one of the values. Cannot be combined with `required`. |
| `enum` | Allowed values. String and array parameters only. |
| `pattern` | Regular expression every value must match. String and array parameters only. |
| `deny` | Forbidden values. `--opt` also forbids `--opt=…`; a short option such as `-R` also forbids grouped forms such as `-iR`. String and array parameters only. |
| `flag` | Option inserted before the value, for example `-m`. Repeated before each item of an array. A boolean with a flag only emits the flag when `true`. |

An absent parameter removes its token from the command. The result returned to the agent shows stdout and stderr separately, followed by the exit code; a non-zero exit code is an error.

## Shell mode

Set `"shell": true` to run a command that needs pipes, redirections or chaining. `command` is then a string run with `/bin/sh -c`, and every injected value is single-quoted, so it is never interpreted by the shell. Only `{name}` where `name` is a declared parameter is replaced: other braces, such as `awk '{print $1}'`, stay as written. An absent parameter is replaced by nothing.

```json
"recent_logs": {
  "description": "Show the last lines of a systemd unit's logs",
  "shell": true,
  "command": "journalctl -u {unit} {since} --no-pager | tail -n {lines}",
  "parameters": {
    "unit": { "type": "string", "required": true, "pattern": "^[a-zA-Z0-9@._-]+$" },
    "since": { "type": "string", "flag": "--since" },
    "lines": { "type": "integer", "required": true }
  }
}
```

The agent never chooses the mode: it is set by the configuration.
