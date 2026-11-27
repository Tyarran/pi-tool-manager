let ( let* ) = Result.bind;

let update = (pi, context: Pi.context, change) =>
  switch (State.get()) {
  | Active(config, session) =>
    switch (change(config, session)) {
    | Ok(session) =>
      State.set(Active(config, session));
      Guard.apply(pi, config, session);
      Pi.notify(
        context.ui,
        "Active tools: " ++ Names.join(Session.selection(config, session)),
        `info,
      );
    | Error(message) => Pi.notify(context.ui, message, `error)
    }
  | (NotConfigured | Invalid(_)) as state =>
    Option.iter(
      message => Pi.notify(context.ui, message, `error),
      State.unavailableReason(state),
    )
  };

let switchPreset = (pi, text, context) =>
  update(
    pi,
    context,
    (config, _session) => {
      let* name =
        CommandParser.presetName(
          ~available=Config.presetNames(config),
          text,
        );
      Session.switchPreset(config, ~known=Guard.knownTools(pi), name);
    },
  );

let adjust = (pi, text, context) =>
  update(
    pi,
    context,
    (_config, session) => {
      let* modifiers = CommandParser.adjustments(text);
      Session.adjust(~known=Guard.knownTools(pi), session, modifiers);
    },
  );

let register = pi => {
  Pi.registerCommand(
    pi,
    "tool-preset",
    {
      description: "Switch the tool preset for the current session: /tool-preset <name>",
      handler: switchPreset(pi),
    },
  );
  Pi.registerCommand(
    pi,
    "tool",
    {
      description: "Add or remove tools for the current session: /tool +name -name",
      handler: adjust(pi),
    },
  );
};
