let workingDirectory = (spec, ~home, ~projectDirectory) =>
  switch (CommandTool.Render.workingDirectory(spec, ~home)) {
  | Some(directory) => Node.resolvePath(projectDirectory, directory)
  | None => projectDirectory
  };

let execute =
    (spec: CommandTool.Spec.t, args, signal, context: PiTool.context) =>
  switch (CommandTool.Arguments.decode(spec, args)) {
  | Error(message) => Js.Promise.resolve(PiTool.failure(message))
  | Ok(values) =>
    let home = Node.homedir();
    Runner.run(
      {
        argv: CommandTool.Render.argv(spec, values, ~home),
        env: CommandTool.Render.env(spec, values, ~home),
        cwd: workingDirectory(spec, ~home, ~projectDirectory=context.cwd),
        timeout: spec.timeout,
        output: spec.output,
      },
      ~signal,
    );
  };

let register = (pi, spec: CommandTool.Spec.t) =>
  Pi.registerTool(
    pi,
    PiTool.define(
      ~name=spec.name,
      ~label=spec.name,
      ~description=spec.description,
      ~parameters=CommandTool.Schema.make(spec),
      ~defaultActive=false,
      (_id, args, signal, _onUpdate, context) =>
      execute(spec, args, signal, context)
    ),
  );
