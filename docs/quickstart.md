# Quickstart

This page assumes the extension is [installed](installation.md).

## A first configuration

Add a `pi-tool-manager` key to `~/.pi/agent/settings.json`, or to the `.pi/settings.json` of a project:

```json
{
  "pi-tool-manager": {
    "baseTools": ["read", "edit", "write", "bash"],
    "defaultPreset": "no-bash",
    "presets": {
      "no-bash": ["-bash"],
      "read-only": ["read"]
    }
  }
}
```

Run `/reload` in pi. The agent now sees only two tools, `tool_call` and `tool_find`, and reaches `read`, `edit` and `write` through them. `bash` is blocked, as is any tool that is not selected.

## Change the tools of a session

```text
/tool-preset read-only     # only read
/tool +bash                # allow bash until the next preset switch
/tool-preset no-bash       # back to the default preset, without the adjustment
```

The selection is not saved: every new session starts from the `defaultPreset`.

## Replace the shell

Declare the commands the agent needs as [command-line tools](command-tools.md), and add them to `baseTools`:

```json
{
  "pi-tool-manager": {
    "baseTools": ["read", "edit", "write", "git_log"],
    "tools": {
      "git_log": {
        "description": "Show the recent commits of the repository",
        "command": ["git", "log", "--oneline", "-n", "{count}"],
        "parameters": {
          "count": { "type": "integer", "required": true }
        }
      }
    }
  }
}
```

The agent calls `git_log` through `tool_call` with a `count`: it never writes the command line itself.

!!! warning "Invalid configuration"

    If the configuration is invalid, the extension reports the error and does nothing else:
    pi keeps its default tools until the configuration is fixed and reloaded.

## Next steps

- [How it works](how-it-works.md) explains `tool_call`, `tool_find` and the whitelist.
- [Selection and presets](configuration.md) details the preset rules.
- [Command-line tools](command-tools.md) lists every option of a JSON tool.
