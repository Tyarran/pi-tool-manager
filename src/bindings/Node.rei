type childProcess;

type stream;

type abortSignal;

type spawnOptions = {
  cwd: string,
  detached: bool,
  stdio: array(string),
  env: Js.Dict.t(string),
};

let spawn: (string, array(string), spawnOptions) => childProcess;

let pid: childProcess => option(int);

let stdout: childProcess => stream;

let stderr: childProcess => stream;

let setEncoding: (stream, string) => unit;

let onData: (stream, string => unit) => unit;

let onClose:
  (childProcess, (Js.Null.t(int), Js.Null.t(string)) => unit) => unit;

let onError: (childProcess, Js.Exn.t => unit) => unit;

let kill: (int, string) => unit;

let environment: unit => Js.Dict.t(string);

let aborted: abortSignal => bool;

let onAbort: (abortSignal, unit => unit) => unit;

let writeFile: (string, string) => unit;

let resolvePath: (string, string) => string;

let joinPath: (string, string) => string;

let tmpdir: unit => string;

let homedir: unit => string;

let randomUUID: unit => string;
