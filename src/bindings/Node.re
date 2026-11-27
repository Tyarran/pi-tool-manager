type childProcess;

type stream;

type abortSignal;

type spawnOptions = {
  cwd: string,
  detached: bool,
  stdio: array(string),
  env: Js.Dict.t(string),
};

[@mel.module "node:child_process"]
external spawn: (string, array(string), spawnOptions) => childProcess =
  "spawn";

[@mel.get] [@mel.return undefined_to_opt]
external pid: childProcess => option(int) = "pid";

[@mel.get] external stdout: childProcess => stream = "stdout";

[@mel.get] external stderr: childProcess => stream = "stderr";

[@mel.send] external setEncoding: (stream, string) => unit = "setEncoding";

[@mel.send]
external onData: (stream, [@mel.as "data"] _, string => unit) => unit = "on";

[@mel.send]
external onClose:
  (
    childProcess,
    [@mel.as "close"] _,
    (Js.Null.t(int), Js.Null.t(string)) => unit
  ) =>
  unit =
  "on";

[@mel.send]
external onError: (childProcess, [@mel.as "error"] _, Js.Exn.t => unit) => unit =
  "on";

[@mel.scope "process"] external kill: (int, string) => unit = "kill";

[@mel.scope "process"] external env: Js.Dict.t(string) = "env";

let environment = () => Js.Dict.fromArray(Js.Dict.entries(env));

[@mel.get] external aborted: abortSignal => bool = "aborted";

[@mel.send]
external onAbort: (abortSignal, [@mel.as "abort"] _, unit => unit) => unit =
  "addEventListener";

[@mel.module "node:fs"]
external writeFile: (string, string) => unit = "writeFileSync";

[@mel.module "node:path"]
external resolvePath: (string, string) => string = "resolve";

[@mel.module "node:path"]
external joinPath: (string, string) => string = "join";

[@mel.module "node:os"] external tmpdir: unit => string = "tmpdir";

[@mel.module "node:os"] external homedir: unit => string = "homedir";

[@mel.module "node:crypto"] external randomUUID: unit => string = "randomUUID";
