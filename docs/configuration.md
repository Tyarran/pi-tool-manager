# Selection and presets

The extension reads the `pi-tool-manager` key from pi's settings: `~/.pi/agent/settings.json` for global settings, `<project>/.pi/settings.json` for project overrides. Project settings are deep-merged over global settings: objects merge key by key, arrays are replaced. A project can therefore replace a preset, or override a single field of a [command-line tool](command-tools.md).

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
    "tools": {}
  }
}
```

| Key | Type | Description |
|---|---|---|
| `baseTools` | string[] | Required. Plain tool names. |
| `presets` | object | Preset name → list of entries, described below. |
| `defaultPreset` | string | Preset applied at session start. Without it, the base list is used. |
| `tools` | object | [Command-line tools](command-tools.md) to register. |

## Preset entries

A preset follows the same rules as pi's `defaultTools`:

- `+name` adds a tool to the base list;
- `-name` removes a tool from it;
- plain names replace the base list, then the `+name` and `-name` entries apply in order.

| Preset | Selection with the example above |
|---|---|
| `["-bash"]` | `read`, `edit`, `write`, `jj` |
| `["-edit", "-write", "-bash"]` | `read`, `jj` |
| `["read"]` | `read` |

The `/tool +name -name` adjustments of a session apply on top of the preset with the same rules.
