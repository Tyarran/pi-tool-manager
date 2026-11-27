type api;
type ui;

type context = {
  cwd: string,
  ui,
};

[@mel.send] external getSettings: api => Js.Dict.t(Js.Json.t) = "getSettings";

type namespace = {
  name: string,
  description: option(string),
  instructions: option(string),
};

type toolInfo = {
  name: string,
  description: string,
  parameters: Js.Json.t,
  namespace: option(namespace),
};

[@mel.send] external getAllTools: api => array(toolInfo) = "getAllTools";

[@mel.send] external getActiveTools: api => array(string) = "getActiveTools";

[@mel.send]
external setActiveTools: (api, array(string)) => unit = "setActiveTools";

type command = {
  description: string,
  handler: (string, context) => unit,
};

[@mel.send] external registerTool: (api, Js.t({..})) => unit = "registerTool";

type agentTool = {
  name: string,
  description: string,
};

type loadout = {declared: array(agentTool)};

[@mel.send]
external getPromptGuidelines: (loadout, string) => array(string) =
  "getPromptGuidelines";

type loadoutChanges = {
  descriptions: Js.Dict.t(string),
  hiddenDeclarations: array(string),
};

type signal;
type updateCallback;
type toolResult;

type truncateOptions = {
  maxLines: int,
  maxBytes: int,
};

type truncation = {
  content: string,
  truncated: bool,
  outputLines: int,
  totalLines: int,
};

[@mel.module "@earendil-works/pi-coding-agent"]
external truncateTail: (string, truncateOptions) => truncation =
  "truncateTail";

[@mel.module "@earendil-works/pi-coding-agent"]
external defaultMaxLines: int = "DEFAULT_MAX_LINES";

[@mel.module "@earendil-works/pi-coding-agent"]
external defaultMaxBytes: int = "DEFAULT_MAX_BYTES";

[@mel.get] external resultContent: toolResult => array(Js.Json.t) = "content";

[@mel.set]
external setResultContent: (toolResult, array(Js.Json.t)) => unit = "content";

[@mel.set] external setResultIsError: (toolResult, bool) => unit = "isError";

[@mel.scope "Object"]
external copyResult: (Js.t({.}), toolResult) => toolResult = "assign";

type toolOutcome = {
  result: toolResult,
  isError: bool,
  durationMs: option(float),
};

type executeOptions = {
  signal: Js.Nullable.t(signal),
  onUpdate: Js.Nullable.t(updateCallback),
};

type toolContext;

[@mel.send]
external executeTool:
  (toolContext, string, Js.Json.t, executeOptions) =>
  Js.Promise.t(toolOutcome) =
  "executeTool";

type theme;

[@mel.send] external fg: (theme, string, string) => string = "fg";
[@mel.send] external bold: (theme, string) => string = "bold";

type component;

[@mel.new] [@mel.module "@earendil-works/pi-tui"]
external text: (string, int, int) => component = "Text";

[@mel.send]
external registerCommand: (api, string, command) => unit = "registerCommand";

[@mel.send]
external onSessionStart:
  (api, [@mel.as "session_start"] _, (Js.Json.t, context) => unit) => unit =
  "on";

[@mel.send]
external onBeforeAgentStart:
  (api, [@mel.as "before_agent_start"] _, (Js.Json.t, context) => unit) => unit =
  "on";

[@mel.send]
external onTurnStart:
  (api, [@mel.as "turn_start"] _, (Js.Json.t, context) => unit) => unit =
  "on";

type toolCallEvent = {toolName: string};

type toolCallResult = {
  block: bool,
  reason: string,
};

[@mel.send]
external onToolCall:
  (
    api,
    [@mel.as "tool_call"] _,
    (toolCallEvent, context) => Js.Undefined.t(toolCallResult)
  ) =>
  unit =
  "on";

[@mel.send]
external notify:
  (
    ui,
    string,
    [
      | `info
      | `warning
      | `error
    ]
  ) =>
  unit =
  "notify";
