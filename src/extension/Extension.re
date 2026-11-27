let start = (pi, context: Pi.context) => {
  let state = State.fromSettings(Pi.settings(pi));
  State.set(state);
  switch (state) {
  | Active(config, session) =>
    List.iter(CommandTools.register(pi), config.tools);
    Guard.apply(pi, config, session);
  | Invalid(_) =>
    Option.iter(
      message => Pi.notify(context.ui, message, `error),
      State.unavailableReason(state),
    )
  | NotConfigured => ()
  };
};

let activate = pi => {
  Pi.onSessionStart(pi, (_event, context) => start(pi, context));
  Guard.register(pi);
  Commands.register(pi);
  ToolCall.register(pi, ~selection=State.selection);
  ToolFind.register(pi, ~selection=State.selection);
};
