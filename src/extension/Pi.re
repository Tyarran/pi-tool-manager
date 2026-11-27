type api;
type ui;

type context = {
  cwd: string,
  ui,
};

[@mel.send] external getSettings: api => Js.Dict.t(Js.Json.t) = "getSettings";

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
