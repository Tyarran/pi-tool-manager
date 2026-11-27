# Contributing to pi-tool-manager

This guide describes how the code is organized and the rules every change follows. The goal is simple, readable code that uses the idioms of Reason and OCaml, with the domain expressed in types.

## Setup

```sh
mise install
opam switch create . 5.5.0 --deps-only --with-test -y
eval $(opam env)
npm ci
```

| Command | Effect |
|---|---|
| `dune build` | Build the extension and the tests |
| `npm test` | Build and run every Jest suite |
| `npm run fmt` | Format the Reason sources (`dune build @fmt --auto-promote`) |

The CI checks formatting, builds and runs the tests on every push.

## Architecture

```
src/
├── core/         Pure domain logic. Knows nothing about pi or Node.
├── bindings/     Typed externals for pi, the pi TUI, Node and TypeBox. No logic.
└── extension/    Wires the core to pi: state, events, commands and tools.
test/
├── core/         Unit tests of the core modules.
└── integration/  Tests of the extension against a fake pi.
```

Dependencies only point downwards: `extension` uses `core` and `bindings`; `core` and `bindings` never use `extension`; `core` never uses `bindings`.

Subdirectories are qualified (`(include_subdirs qualified)`): `src/core/commandTool/Spec.re` is the module `CommandTool.Spec`. Group modules in a subdirectory when they form one concept with several parts.

### Core

| Module | Responsibility |
|---|---|
| `Results` | Combine results over lists |
| `Names` | `Set` and `Map` of names (tools, presets, parameters) |
| `Decode` | JSON decoders that report the path of the faulty value |
| `Selection` | Tool selection modifiers (`Add`, `Remove`), presets and their resolution |
| `Session` | Active preset and adjustments of the current session |
| `CommandParser` | Parsing of the `/tool-preset` and `/tool` arguments |
| `Config` | The `pi-tool-manager` settings, decoded once into precise types |
| `Proxy` | The `tool_call` and `tool_find` tools: names, parameters and call checks |
| `Catalog` | Texts shown to the model by `tool_call` and `tool_find` |
| `Search` | BM25 ranking of tools for `tool_find` |
| `CommandTool.Spec` | Definition of a command-line tool, decoded from the configuration |
| `CommandTool.Arguments` | Arguments sent by the model, decoded and checked against the definition |
| `CommandTool.Render` | Program arguments, shell command, environment and working directory of a call |
| `CommandTool.Schema` | JSON Schema of a command-line tool |
| `CommandTool.Output` | Truncation of the command output |
| `CommandTool.Outcome` | How a command ended and the text returned to the model |

### Bindings

`Pi`, `PiTool`, `PiTui` and `Node` declare externals and the records they exchange with JavaScript. They contain no logic beyond building the values JavaScript expects.

### Extension

| Module | Responsibility |
|---|---|
| `State` | The only mutable state: the configuration and the session |
| `Guard` | Keeps the active tools in sync and blocks calls outside the selection |
| `Commands` | `/tool-preset` and `/tool` |
| `ToolCall`, `ToolFind`, `CommandTools` | Registration of the tools |
| `Runner` | Runs a process and collects its output |
| `Extension` | Entry point: wires everything together, nothing else |

## Code guidelines

### Keep the core pure

The core is a set of functions from values to values. Effects (pi calls, processes, files, time) live in `extension/`, through `bindings/`. A function that needs an effect receives its result as an argument: `Session.adjust(~known, session, entries)` takes the known tool names instead of asking pi.

### Parse, don't validate

Untrusted input (settings, command arguments, model arguments) is decoded once, at the boundary, into precise types. After decoding, the code never inspects strings again: a modifier is `Add("bash")`, not `"+bash"`.

### Make illegal states unrepresentable

Prefer variants to booleans, strings and options that depend on each other.

```reason
type command =
  | Argv(list(token))
  | Shell(string);
```

rather than a `shell: bool` next to a `command` whose type depends on it.

### Use the right data structure

- `Names.Set` for sets of names (selection, known tools).
- `Names.Map` for lookups by name (presets, parameters).
- `list` for ordered sequences (command tokens, modifiers applied in order).
- `array` only at the JavaScript boundary.

### Errors are values

- The core returns `result('a, string)` and never raises.
- Chain results with `let*` (`Result.bind`) instead of nested `switch`.
- Error messages are English sentences that name the faulty value: `pi-tool-manager.presets.code contains an empty tool name`.
- `failwith` and exceptions are only acceptable in tests.

### Match exhaustively

Never use a wildcard `_` on a variant defined in this project: adding a constructor must break the build where it is not handled.

### Small modules, minimal interfaces

- Every module has a `.rei` interface exposing only what other modules use. Helpers stay private.
- A module has one responsibility, stated by its name. If a module needs a sentence with "and" to describe it, split it.
- Functions stay short; extract a named function rather than nesting.

### Naming

- Names come from the domain: `selection`, `preset`, `adjustment`, `spec`, `argument`.
- Functions are verbs (`resolve`, `decode`, `render`), values are nouns.
- Labeled arguments (`~known`, `~home`) when two arguments of the same type could be swapped.
- No abbreviations except well-known ones (`argv`, `env`, `cwd`, `json`).

### No comments

Code explains itself through names and types. Do not add comments; rename or extract instead.

### Bindings

- Every external is typed with records or abstract types; `Js.t({..})` is not allowed.
- Objects passed to JavaScript are built with records or `[@mel.obj]`.
- Optional JavaScript values are `option` (or `Js.Nullable.t` at the boundary, converted immediately).

### State

`State` holds the only `ref` of the project, behind functions (`State.get`, `State.set`). Other modules receive values, not the state.

### Formatting

Run `npm run fmt` before committing. The CI rejects unformatted code.

## Tests

- Every core module has a test file named after it: `test/core/Selection_test.re`, `test/core/CommandToolSpec_test.re` for `CommandTool.Spec`.
- Tests use the public interface (`.rei`) only.
- Prefer `testAll` with a table of cases to repeated tests.
- Integration tests in `test/integration/` load the extension with a fake pi written in Reason and check its behavior: registered tools, active tools, blocked calls, commands, and real command execution. JavaScript packages that Jest cannot load (ES modules such as `@earendil-works/pi-tui`) are mapped to stubs in `test/stubs/`.
- A bug fix comes with a test that fails without the fix.

## Workflow

- Version control uses [jj](https://jj-vcs.github.io/jj/).
- Commits follow [Conventional Commits](https://www.conventionalcommits.org/): `feat:`, `fix:`, `refactor:`, `test:`, `docs:`, `chore:`.
- A commit that completes an issue ends with `Closes #<number>`.
- Each commit builds, is formatted and passes the tests.
