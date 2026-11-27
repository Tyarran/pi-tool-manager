type command = {
  argv: list(string),
  env: list((string, string)),
  cwd: string,
  timeout: int,
  output: CommandTool.Spec.output,
};

let run:
  (command, ~signal: Js.Nullable.t(Node.abortSignal)) =>
  Js.Promise.t(PiTool.result);
