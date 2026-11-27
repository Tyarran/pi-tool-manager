let limitsOf = (spec: ToolSpec.t): Runner.limits => {
  maxLines: Option.value(spec.maxLines, ~default=Pi.defaultMaxLines),
  maxBytes: Option.value(spec.maxBytes, ~default=Pi.defaultMaxBytes),
};

let execute = (spec: ToolSpec.t, args, signal, ctx: Pi.context) =>
  switch (ToolCommand.validate(spec, args)) {
  | Error(message) => Js.Promise.resolve(ToolResult.failure(message))
  | Ok(values) =>
    let home = Node.homedir();
    Runner.run(
      ~cwd=
        switch (spec.cwd) {
        | None => ctx.cwd
        | Some(cwd) =>
          Node.resolvePath(ctx.cwd, ToolCommand.expandHome(~home, cwd))
        },
      ~argv=ToolCommand.render(spec, values, ~home),
      ~env=ToolCommand.environment(spec, values, ~home),
      ~timeout=spec.timeout,
      ~limits=limitsOf(spec),
      ~signal,
    );
  };

let register = (pi, spec: ToolSpec.t) =>
  Pi.registerTool(
    pi,
    {
      "name": spec.name,
      "label": spec.name,
      "description": spec.description,
      "parameters": ToolCommand.schema(spec),
      "defaultActive": false,
      "execute": (_toolCallId, args, signal, _onUpdate, ctx) =>
        execute(spec, args, signal, ctx),
    },
  );
