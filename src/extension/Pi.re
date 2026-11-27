type api;
type ui;

type context = {
  cwd: string,
  ui,
};

[@mel.send] external getSettings: api => Js.Dict.t(Js.Json.t) = "getSettings";

type toolInfo = {name: string};

[@mel.send] external getAllTools: api => array(toolInfo) = "getAllTools";

[@mel.send]
external setActiveTools: (api, array(string)) => unit = "setActiveTools";

type command = {
  description: string,
  handler: (string, context) => unit,
};

[@mel.send]
external registerCommand: (api, string, command) => unit = "registerCommand";

[@mel.send]
external onSessionStart:
  (api, [@mel.as "session_start"] _, (Js.Json.t, context) => unit) => unit =
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
