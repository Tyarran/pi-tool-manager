type command = {
  argv: list(string),
  env: list((string, string)),
  cwd: string,
  timeout: int,
  output: CommandTool.Spec.output,
};

let killGraceMs = 5000;

let display = (limits: CommandTool.Spec.output, ~stream, text) => {
  let output =
    CommandTool.Output.tail(
      ~maxLines=limits.maxLines,
      ~maxBytes=limits.maxBytes,
      text,
    );
  if (output.truncated) {
    let file =
      Node.joinPath(
        Node.tmpdir(),
        Printf.sprintf(
          "pi-tool-manager-%s-%s.log",
          stream,
          Node.randomUUID(),
        ),
      );
    Node.writeFile(file, text);
    CommandTool.Output.withNotice(output, ~file);
  } else {
    output.text;
  };
};

let resultOf = (command, ~stdout, ~stderr, outcome) => {
  PiTool.content: [|
    PiTool.text(
      CommandTool.Outcome.format(
        ~stdout=display(command.output, ~stream="stdout", stdout),
        ~stderr=display(command.output, ~stream="stderr", stderr),
        outcome,
      ),
    ),
  |],
  isError: CommandTool.Outcome.isError(outcome),
};

let environment = env => {
  let merged = Node.environment();
  List.iter(((name, value)) => Js.Dict.set(merged, name, value), env);
  merged;
};

let collect = stream => {
  let chunks = ref([]);
  Node.setEncoding(stream, "utf8");
  Node.onData(stream, chunk => chunks := [chunk, ...chunks^]);
  () => String.concat("", List.rev(chunks^));
};

let killGroup = (child, signalName) =>
  switch (Node.pid(child)) {
  | Some(pid) =>
    try(Node.kill(- pid, signalName)) {
    | _ => ()
    }
  | None => ()
  };

let exitOutcome = (~stopReason, code, signalName): CommandTool.Outcome.t =>
  switch (stopReason, Js.Null.toOption(code), Js.Null.toOption(signalName)) {
  | (Some(reason), _, _) => reason
  | (None, Some(code), _) => Exited(code)
  | (None, None, Some(signalName)) => Signaled(signalName)
  | (None, None, None) => Exited(-1)
  };

let supervise = (command, ~program, ~args, ~signal, ~resolve) => {
  let child =
    Node.spawn(
      program,
      Array.of_list(args),
      {
        cwd: command.cwd,
        detached: true,
        stdio: [|"ignore", "pipe", "pipe"|],
        env: environment(command.env),
      },
    );
  let stdout = collect(Node.stdout(child));
  let stderr = collect(Node.stderr(child));
  let finished = ref(false);
  let stopReason = ref(None);
  let timers = ref([]);
  let schedule = (delay, action) =>
    timers := [Js.Global.setTimeout(~f=action, delay), ...timers^];
  let finish = outcome =>
    if (! finished^) {
      finished := true;
      List.iter(Js.Global.clearTimeout, timers^);
      resolve(
        resultOf(command, ~stdout=stdout(), ~stderr=stderr(), outcome),
      );
    };
  let stop = reason =>
    if (! finished^ && Option.is_none(stopReason^)) {
      stopReason := Some(reason);
      killGroup(child, "SIGTERM");
      schedule(killGraceMs, () => killGroup(child, "SIGKILL"));
    };
  Node.onError(child, error =>
    finish(
      Failed(Option.value(Js.Exn.message(error), ~default="unknown error")),
    )
  );
  Node.onClose(child, (code, signalName) =>
    finish(exitOutcome(~stopReason=stopReason^, code, signalName))
  );
  schedule(command.timeout * 1000, () => stop(TimedOut(command.timeout)));
  switch (Js.Nullable.toOption(signal)) {
  | Some(signal) when Node.aborted(signal) => stop(Aborted)
  | Some(signal) => Node.onAbort(signal, () => stop(Aborted))
  | None => ()
  };
};

let run = (command, ~signal) =>
  switch (command.argv) {
  | [] =>
    Js.Promise.resolve(
      resultOf(command, ~stdout="", ~stderr="", Failed("empty command")),
    )
  | [program, ...args] =>
    Js.Promise.make((~resolve, ~reject as _) =>
      supervise(command, ~program, ~args, ~signal, ~resolve=result =>
        resolve(. result)
      )
    )
  };
