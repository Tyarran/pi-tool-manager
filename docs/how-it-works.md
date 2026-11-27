# How it works

## The selection

The **selection** is the set of tools the agent may use. It is the `baseTools` list, modified by the active preset, then by the `/tool` adjustments of the session. See [Selection and presets](configuration.md).

## Two tools for the agent

The agent only sees two tools:

- `tool_call` runs a tool of the selection. It takes the name of the tool, its arguments, and a mandatory `intent`: a short sentence explaining why the agent calls it. The intent stays in the session history next to the call.
- `tool_find` searches the selection by keywords and returns the description and the parameter schema of the matches, so the agent knows how to call them.

The selected tools stay active so that `tool_call` can reach them, but their declarations are hidden from the model. The description of `tool_call` lists them with their guidelines. When a call is rejected before running, for example because of invalid arguments, the result includes the expected parameter schema so the agent can correct itself.

## A strict whitelist

Any tool outside the selection is blocked, including calls made indirectly from `codemode` scripts or loaded by `tool_search`. Before every turn, the extension checks the active tools again: tools that registered late, such as the tools of an MCP server that connected after the session started, are enabled when they are selected and disabled otherwise.

## Commands

| Command | Effect |
|---|---|
| `/tool-preset <name>` | Switch to a preset for the current session and reset the `/tool` adjustments. |
| `/tool +name -name …` | Add or remove tools for the current session, on top of the preset. |

Both commands fail without changing anything when a tool is unknown. At session start, the `defaultPreset` is applied with the tools known so far; the others are enabled as soon as they register. The selection is not saved: every new session starts from the `defaultPreset`.

## Invalid configuration

If the configuration is invalid, the extension does nothing but report the error: pi keeps its default behavior until the configuration is fixed and reloaded.
