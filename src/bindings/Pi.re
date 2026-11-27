type api;

type ui;

type context = {
  cwd: string,
  ui,
};

type level = [
  | `info
  | `warning
  | `error
];

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

type command = {
  description: string,
  handler: (string, context) => unit,
};

type toolCall = {toolName: string};

type block = {
  block: bool,
  reason: string,
};

[@mel.send] external notify: (ui, string, level) => unit = "notify";

[@mel.send] external settings: api => Js.Dict.t(Js.Json.t) = "getSettings";

[@mel.send] external allTools: api => array(toolInfo) = "getAllTools";

[@mel.send] external activeTools: api => array(string) = "getActiveTools";

[@mel.send]
external setActiveTools: (api, array(string)) => unit = "setActiveTools";

[@mel.send]
external registerTool: (api, PiTool.definition('params)) => unit =
  "registerTool";

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

[@mel.send]
external onToolCall:
  (
    api,
    [@mel.as "tool_call"] _,
    (toolCall, context) => Js.Undefined.t(block)
  ) =>
  unit =
  "on";
