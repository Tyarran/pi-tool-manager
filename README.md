<p align="center">
  <img src="docs/images/icon.svg" alt="pi-tool-manager" width="140">
</p>

<h1 align="center">pi-tool-manager</h1>

<p align="center"><strong>Restrict, organize and explain the tools your pi agent can use</strong></p>

<p align="center">
  <a href="https://github.com/Tyarran/pi-tool-manager/actions/workflows/ci.yml">
    <img src="https://github.com/Tyarran/pi-tool-manager/actions/workflows/ci.yml/badge.svg" alt="CI">
  </a>
  <a href="https://pi.dev">
    <img src="https://img.shields.io/badge/pi-extension-8b5cf6" alt="pi extension">
  </a>
  <a href="https://tyarran.github.io/pi-tool-manager/">
    <img src="https://img.shields.io/badge/docs-tyarran.github.io-blue" alt="Documentation">
  </a>
  <img src="https://img.shields.io/badge/made%20with-Reason%20%2B%20Melange-dd4b39" alt="Reason + Melange">
  <img src="https://img.shields.io/github/license/Tyarran/pi-tool-manager" alt="License">
</p>

---

A coding agent with a shell can do anything, and every MCP server adds more tools to its reach.
pi-tool-manager is a [pi](https://pi.dev) extension that puts you back in control:
you choose which tools the agent may use with presets that you switch per session,
you replace the shell with command-line tools declared in JSON whose arguments are validated,
and the agent reaches every tool through a single `tool_call` that requires the intent behind the call,
so the session history tells you why each tool was used.

```mermaid
flowchart LR
    LLM[Agent] -->|tool_call + intent| PTM
    LLM -->|tool_find| PTM
    Settings[settings.json<br/>baseTools, presets, tools] --> PTM
    You["/tool-preset, /tool"] --> PTM
    PTM[pi-tool-manager] -->|selected| Tools[read, edit, MCP tools...]
    PTM -->|selected| JSON[JSON command tools]
    PTM -.->|blocked| Other[bash and any other tool]
```

## Installation

Requirements: [mise](https://mise.jdx.dev), or opam 2 and Node.js ≥ 22.19 installed manually.

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

See the [installation guide](https://tyarran.github.io/pi-tool-manager/installation/) for the requirements and how to update.

## Quickstart

Add a minimal configuration next to it, in `~/.pi/agent/settings.json` or in the `.pi/settings.json` of a project:

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

Run `/reload` in pi. The agent now sees only `tool_call` and `tool_find`, and reaches `read`, `edit` and `write` through them; `bash` is blocked.
Change the selection for the current session at any time:

```text
/tool-preset read-only     # only read
/tool +bash                # allow bash until the next preset switch
```

An invalid configuration disables the extension and reports the error: pi keeps its default tools until the configuration is fixed and reloaded.
The [Quickstart](https://tyarran.github.io/pi-tool-manager/quickstart/) goes on with replacing the shell by command-line tools.

## Documentation

The full documentation is at <https://tyarran.github.io/pi-tool-manager/>.

| To... | Read |
|---|---|
| Install and try it | [Installation](https://tyarran.github.io/pi-tool-manager/installation/), [Quickstart](https://tyarran.github.io/pi-tool-manager/quickstart/) |
| Understand what the extension does | [How it works](https://tyarran.github.io/pi-tool-manager/how-it-works/) |
| Choose the tools of a session | [Selection and presets](https://tyarran.github.io/pi-tool-manager/configuration/) |
| Replace the shell with safe tools | [Command-line tools](https://tyarran.github.io/pi-tool-manager/command-tools/) |
| Understand or change the code | [CONTRIBUTING.md](CONTRIBUTING.md) |

The sources of the site live in [`docs/`](docs/). Preview it locally with `mise run docs`, or build it with `mise run docs:build`.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

MIT — see [LICENSE](LICENSE).
