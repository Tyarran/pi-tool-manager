let killGraceMs = 5000;

type limits = {
  maxLines: int,
  maxBytes: int,
};

let truncate = (~limits, ~name, output) => {
  let truncation =
    Pi.truncateTail(
      output,
      {
        maxLines: limits.maxLines,
        maxBytes: limits.maxBytes,
      },
    );
  if (truncation.truncated) {
    let file =
      Node.joinPath(
        Node.tmpdir(),
        Printf.sprintf("pi-tool-manager-%s-%s.log", name, Node.randomUUID()),
      );
    Node.writeFileSync(file, output);
    Printf.sprintf(
      "[Output truncated: last %d of %d lines. Full output saved to: %s]\n%s",
      truncation.outputLines,
      truncation.totalLines,
      file,
      truncation.content,
    );
  } else {
    truncation.content;
  };
};

let mergeEnv = env => {
  let merged = Js.Dict.fromArray(Js.Dict.entries(Node.processEnv));
  List.iter(((name, value)) => Js.Dict.set(merged, name, value), env);
  merged;
};

let run = (~cwd, ~argv, ~env, ~timeout, ~limits, ~signal) =>
  Js.Promise.make((~resolve, ~reject as _) => {
    let stdout = ref([]);
    let stderr = ref([]);
    let finished = ref(false);
    let reason = ref(None);
    let timers = ref([]);
    let collect = (chunks, chunk) => chunks := [chunk, ...chunks^];
    let joined = chunks => String.concat("", List.rev(chunks^));
    let finish = outcome =>
      if (! finished^) {
        finished := true;
        List.iter(Js.Global.clearTimeout, timers^);
        let text =
          Outcome.format(
            ~stdout=truncate(~limits, ~name="stdout", joined(stdout)),
            ~stderr=truncate(~limits, ~name="stderr", joined(stderr)),
            outcome,
          );
        resolve(.
          ToolResult.make(
            ~content=[|ToolResult.text(text)|],
            ~isError=Outcome.isError(outcome),
            (),
          ),
        );
      };
    switch (argv) {
    | [] => finish(Failed("empty command"))
    | [program, ...args] =>
      let child =
        Node.spawn(
          program,
          Array.of_list(args),
          {
            cwd,
            detached: true,
            stdio: [|"ignore", "pipe", "pipe"|],
            env: mergeEnv(env),
          },
        );
      let kill = signalName =>
        switch (Node.pid(child)) {
        | Some(pid) =>
          try(Node.killProcess(- pid, signalName)) {
          | _ => ()
          }
        | None => ()
        };
      let stop = why =>
        if (! finished^ && Option.is_none(reason^)) {
          reason := Some(why);
          kill("SIGTERM");
          timers :=
            [
              Js.Global.setTimeout(~f=() => kill("SIGKILL"), killGraceMs),
              ...timers^,
            ];
        };
      Node.setEncoding(Node.stdout(child), "utf8");
      Node.setEncoding(Node.stderr(child), "utf8");
      Node.onData(Node.stdout(child), collect(stdout));
      Node.onData(Node.stderr(child), collect(stderr));
      Node.onError(child, error =>
        finish(
          Failed(
            Option.value(Js.Exn.message(error), ~default="unknown error"),
          ),
        )
      );
      Node.onClose(child, (code, signalName) =>
        finish(
          switch (
            reason^,
            Js.Null.toOption(code),
            Js.Null.toOption(signalName),
          ) {
          | (Some(outcome), _, _) => outcome
          | (None, Some(code), _) => Exited(code)
          | (None, None, Some(signalName)) => Signaled(signalName)
          | (None, None, None) => Exited(-1)
          },
        )
      );
      timers :=
        [
          Js.Global.setTimeout(
            ~f=() => stop(Outcome.TimedOut(timeout)),
            timeout * 1000,
          ),
          ...timers^,
        ];
      switch (Js.Nullable.toOption(signal)) {
      | Some(signal) when Node.aborted(signal) => stop(Outcome.Aborted)
      | Some(signal) => Node.onAbort(signal, () => stop(Outcome.Aborted))
      | None => ()
      };
    };
  });
