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

let notify: (ui, string, level) => unit;

let settings: api => Js.Dict.t(Js.Json.t);

let allTools: api => array(toolInfo);

let activeTools: api => array(string);

let setActiveTools: (api, array(string)) => unit;

let registerTool: (api, PiTool.definition('params)) => unit;

let registerCommand: (api, string, command) => unit;

let onSessionStart: (api, (Js.Json.t, context) => unit) => unit;

let onBeforeAgentStart: (api, (Js.Json.t, context) => unit) => unit;

let onTurnStart: (api, (Js.Json.t, context) => unit) => unit;

let onToolCall: (api, (toolCall, context) => Js.Undefined.t(block)) => unit;
