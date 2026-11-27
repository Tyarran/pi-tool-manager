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

## Configuration

The extension reads the `pi-tool-manager` key from pi's settings (`pi.getSettings()`): `~/.pi/agent/settings.json` for global settings, `<project>/.pi/settings.json` for project overrides. Project settings are deep-merged over global settings: objects merge key by key, arrays are replaced.

## Development

```sh
dune build        # build
npm test          # build and run the Jest tests
npm run fmt       # format the Reason sources
```

## License

MIT
