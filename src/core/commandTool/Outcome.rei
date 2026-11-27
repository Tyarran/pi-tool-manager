type t =
  | Exited(int)
  | Signaled(string)
  | TimedOut(int)
  | Aborted
  | Failed(string);

let isError: t => bool;

let format: (~stdout: string, ~stderr: string, t) => string;
