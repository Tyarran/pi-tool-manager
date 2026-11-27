type state =
  | NotConfigured
  | Disabled(string)
  | Active(Config.t, Session.t);

let state = ref(NotConfigured);

let load = pi =>
  switch (Config.decode(Js.Dict.get(Pi.getSettings(pi), Settings.key))) {
  | Ok(None) => NotConfigured
  | Ok(Some(config)) => Active(config, Session.start(config))
  | Error(message) => Disabled(message)
  };

let knownTools = pi =>
  Pi.getAllTools(pi)
  |> Array.to_list
  |> List.map((tool: Pi.toolInfo) => tool.name);

let apply = (pi, config, session) =>
  Pi.setActiveTools(pi, Array.of_list(Session.active(config, session)));

let available = () =>
  switch (state^) {
  | Active(config, session) => Session.tools(config, session)
  | NotConfigured
  | Disabled(_) => []
  };

let sync = pi =>
  switch (state^) {
  | Active(config, session)
      when
        Session.needsSync(
          config,
          session,
          ~known=knownTools(pi),
          ~active=Array.to_list(Pi.getActiveTools(pi)),
        ) =>
    apply(pi, config, session)
  | Active(_)
  | NotConfigured
  | Disabled(_) => ()
  };

let guard = (event: Pi.toolCallEvent) =>
  switch (state^) {
  | Active(config, session)
      when !Session.allows(config, session, event.toolName) =>
    Js.Undefined.return({
      Pi.block: true,
      reason:
        Printf.sprintf(
          "%s is not allowed by the active tool selection (%s)",
          event.toolName,
          Settings.key,
        ),
    })
  | Active(_)
  | NotConfigured
  | Disabled(_) => Js.Undefined.empty
  };

let unavailable = () =>
  switch (state^) {
  | Disabled(message) =>
    Some(
      Printf.sprintf(
        "%s is disabled, fix its configuration: %s",
        Settings.key,
        message,
      ),
    )
  | NotConfigured =>
    Some(Printf.sprintf("%s is not configured", Settings.key))
  | Active(_) => None
  };

let update = (pi, ctx: Pi.context, change) =>
  switch (state^) {
  | Active(config, session) =>
    switch (change(config, session)) {
    | Error(message) => Pi.notify(ctx.ui, message, `error)
    | Ok(session) =>
      state := Active(config, session);
      apply(pi, config, session);
      Pi.notify(
        ctx.ui,
        Printf.sprintf(
          "Active tools: %s",
          String.concat(", ", Session.tools(config, session)),
        ),
        `info,
      );
    }
  | NotConfigured
  | Disabled(_) =>
    Option.iter(message => Pi.notify(ctx.ui, message, `error), unavailable())
  };

let activate = pi => {
  Pi.onSessionStart(
    pi,
    (_event, ctx) => {
      state := load(pi);
      switch (state^) {
      | Active(config, session) =>
        List.iter(JsonTool.register(pi), config.tools);
        apply(pi, config, session);
      | Disabled(_) =>
        Option.iter(
          message => Pi.notify(ctx.ui, message, `error),
          unavailable(),
        )
      | NotConfigured => ()
      };
    },
  );
  Pi.onBeforeAgentStart(pi, (_event, _ctx) => sync(pi));
  Pi.onTurnStart(pi, (_event, _ctx) => sync(pi));
  Pi.onToolCall(pi, (event, _ctx) => guard(event));
  ToolCall.register(pi, ~available);
  ToolFind.register(pi, ~available);
  Pi.registerCommand(
    pi,
    "tool-preset",
    {
      description: "Switch the tool preset for the current session: /tool-preset <name>",
      handler: (args, ctx) =>
        update(pi, ctx, (config, _session) =>
          Session.switchPreset(config, ~known=knownTools(pi), args)
        ),
    },
  );
  Pi.registerCommand(
    pi,
    "tool",
    {
      description: "Add or remove tools for the current session: /tool +name -name",
      handler: (args, ctx) =>
        update(pi, ctx, (_config, session) =>
          Session.adjust(~known=knownTools(pi), session, args)
        ),
    },
  );
};
