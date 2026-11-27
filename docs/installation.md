# Installation

## Requirements

- [mise](https://mise.jdx.dev), which installs Node.js and opam from the repository's `mise.toml`;
- or [opam](https://opam.ocaml.org) 2 and Node.js ≥ 22.19 installed manually.

## Build the extension

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

## Register it in pi

Add the repository to the extensions of `~/.pi/agent/settings.json`:

```json
{
  "extensions": ["~/Projects/pi-tool-manager"]
}
```

Then run `/reload` in pi.

!!! note "Nothing changes until it is configured"

    Without a `pi-tool-manager` key in the settings, the extension does nothing and pi keeps its default tools.
    The [Quickstart](quickstart.md) adds a first configuration.

## Update

```sh
cd ~/Projects/pi-tool-manager
git pull
npm ci
dune build
```

Then run `/reload` in pi.
