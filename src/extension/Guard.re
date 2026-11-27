let knownTools = pi =>
  Pi.allTools(pi)
  |> Array.to_list
  |> List.map((tool: Pi.toolInfo) => tool.name)
  |> Names.Set.of_list;

let apply = (pi, config, session) =>
  Pi.setActiveTools(
    pi,
    Array.of_list(Names.Set.elements(Session.active(config, session))),
  );

let activeTools = pi =>
  Names.Set.of_list(Array.to_list(Pi.activeTools(pi)));

let sync = pi =>
  switch (State.get()) {
  | Active(config, session)
      when
        Session.needsSync(
          config,
          session,
          ~known=knownTools(pi),
          ~active=activeTools(pi),
        ) =>
    apply(pi, config, session)
  | Active(_)
  | NotConfigured
  | Invalid(_) => ()
  };

let check = (call: Pi.toolCall) =>
  switch (State.get()) {
  | Active(config, session)
      when !Session.allows(config, session, call.toolName) =>
    Js.Undefined.return({
      Pi.block: true,
      reason: Catalog.blocked(call.toolName),
    })
  | Active(_)
  | NotConfigured
  | Invalid(_) => Js.Undefined.empty
  };

let register = pi => {
  Pi.onBeforeAgentStart(pi, (_event, _context) => sync(pi));
  Pi.onTurnStart(pi, (_event, _context) => sync(pi));
  Pi.onToolCall(pi, (call, _context) => check(call));
};
