# pi-tool-manager

A [pi](https://pi.dev) extension to manage and restrict the tools available to the LLM.

> 🚧 Work in progress. See the [roadmap epic](https://github.com/Tyarran/pi-tool-manager/issues/12).

## Goals

- **JSON tools**: declare command-line tools in configuration, with typed and validated parameters injected into a command template, run without a shell by default.
- **Presets**: user-defined tool sets with a `defaultPreset`, switched with `/tool-preset <name>` and adjusted with `/tool +x -y` for the current session, enforced as a strict whitelist.
- **Proxy**: `tool_call` (with a mandatory `intent`) and `tool_find` are the only tools visible to the LLM.

## Installation

Requirements: [mise](https://mise.jdx.dev) (or opam 2 and Node.js ≥ 22.19 installed manually).

```sh
git clone https://github.com/Tyarran/pi-tool-manager.git ~/Projects/pi-tool-manager
cd ~/Projects/pi-tool-manager
mise install
opam init -a --bare            # first opam use only
opam switch create . 5.5.0 --deps-only --with-test -y
eval $(opam env)
npm ci
dune build
```

Register the extension in `~/.pi/agent/settings.json`:

```json
{
  "extensions": ["~/Projects/pi-tool-manager"]
}
```

Then run `/reload` in pi. Rebuild with `dune build` and `/reload` after each change.

## How it works

- The **selection** of tools is the `baseTools` list modified by the active preset, then by the `/tool` adjustments of the session.
- The LLM only sees two tools: `tool_call`, which runs a tool of the selection with a mandatory `intent`, and `tool_find`, which searches the selection and returns the parameter schema of the matches. The selected tools stay active so `tool_call` can reach them, but their declarations are hidden from the model.
- The selection is a strict whitelist: calls to any other tool are blocked, including nested calls from `codemode` scripts, and tools that register late (MCP servers) are re-checked before every turn.
- If the configuration is invalid, the extension does nothing but report the error: pi keeps its default behavior until the configuration is fixed and reloaded.

## Commands

| Command | Effect |
|---|---|
| `/tool-preset <name>` | Switch to a preset for the current session and reset the `/tool` adjustments. |
| `/tool +name -name …` | Add or remove tools for the current session, on top of the preset. |

Both commands fail without changing anything when a tool is unknown. At session start, the `defaultPreset` is applied with the tools known so far; the others are enabled as soon as they register.

## Configuration

The extension reads the `pi-tool-manager` key from pi's settings (`pi.getSettings()`): `~/.pi/agent/settings.json` for global settings, `<project>/.pi/settings.json` for project overrides. Project settings are deep-merged over global settings: objects merge key by key, arrays are replaced. A project can therefore replace a preset, or override a single field of a JSON tool.

```json
{
  "pi-tool-manager": {
    "baseTools": ["read", "edit", "write", "bash", "jj"],
    "defaultPreset": "no-bash",
    "presets": {
      "no-bash": ["-bash"],
      "read-only": ["-edit", "-write", "-bash"],
      "only-read": ["read"]
    },
    "tools": {
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
    }
  }
}
```

### Selection

| Key | Type | Description |
|---|---|---|
| `baseTools` | string[] | Required. Plain tool names. |
| `presets` | object | Preset name → list of entries. `+name` adds a tool to the base, `-name` removes one; plain names replace the base, then the modifiers apply in order (same rules as pi's `defaultTools`). |
| `defaultPreset` | string | Preset applied at session start. Without it, the base list is used. |

### JSON tools

Each entry of `tools` registers a tool that runs a command **without a shell**: values are passed as separate arguments and are never interpreted.

| Key | Type | Default | Description |
|---|---|---|---|
| `description` | string | — | Required. Shown to the LLM. |
| `command` | string[] | — | Required. The first token is the program; `{name}` tokens are replaced by parameters. A placeholder must be a whole token. `~` at the start of a literal token is the home directory. A string in [shell mode](#shell-mode). |
| `shell` | boolean | `false` | Run `command` through `/bin/sh`. |
| `parameters` | object | `{}` | Parameter name → definition (below). Every parameter must be used, every placeholder declared. |
| `timeout` | integer | `120` | Seconds before the process group is killed. |
| `cwd` | string | project directory | Working directory, relative to the project directory. |
| `env` | object | `{}` | Extra environment variables: literal strings or a `{name}` placeholder. |
| `maxLines` / `maxBytes` | integer | `2000` / `51200` | Output kept for each stream; the full output is saved to a temporary file when truncated. |

Parameter definition:

| Key | Description |
|---|---|
| `type` | Required: `string`, `integer`, `number`, `boolean` or `array` (of strings). |
| `description` | Shown to the LLM. |
| `required` | The parameter must be provided. |
| `requiredWhen` | `{ "other": ["value", …] }`: required when another string parameter has one of the values. |
| `enum` | Allowed values (string and array items). |
| `pattern` | Regular expression every value must match (string and array items). |
| `deny` | Forbidden values (string and array items). `--opt` also forbids `--opt=…`; a short option such as `-R` also forbids grouped forms such as `-iR`. |
| `flag` | Option inserted before the value, for example `-m`. Repeated before each item of an array. A boolean with a flag only emits the flag when `true`. |

An absent parameter removes its token from the command. The result returned to the LLM shows stdout and stderr separately, followed by the exit code; a non-zero exit code is an error.

### Shell mode

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

The LLM never chooses the mode: it is set by the configuration.

## Development

```sh
dune build        # build
npm test          # build and run the Jest tests
npm run fmt       # format the Reason sources
```

## License

MIT
