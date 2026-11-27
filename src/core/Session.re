type t = {
  preset: option(string),
  adjustments: list(Selection.modifier),
};

let ( let* ) = Result.bind;

let start = (config: Config.t) => {
  preset: config.defaultPreset,
  adjustments: [],
};

let presetOf = (config: Config.t, name) =>
  Names.Map.find_opt(name, config.presets);

let selection = (config: Config.t, session) => {
  let base =
    switch (Option.bind(session.preset, presetOf(config))) {
    | Some(preset) => Selection.resolve(~base=config.baseTools, preset)
    | None => config.baseTools
    };
  Selection.apply(base, session.adjustments);
};

let active = (config, session) =>
  Names.Set.union(selection(config, session), Proxy.tools);

let allows = (config, session, name) =>
  Names.Set.mem(name, active(config, session));

let needsSync = (config, session, ~known, ~active as current) =>
  !
    Names.Set.equal(
      Names.Set.inter(active(config, session), known),
      current,
    );

let checkKnown = (~known, names) => {
  let unknown = Names.Set.diff(names, known);
  Names.Set.is_empty(unknown)
    ? Ok() : Error("Unknown tools: " ++ Names.join(unknown));
};

let switchPreset = (config: Config.t, ~known, name) =>
  switch (presetOf(config, name)) {
  | None =>
    Error(
      Printf.sprintf(
        "Unknown preset \"%s\" (available: %s)",
        name,
        String.concat(", ", Config.presetNames(config)),
      ),
    )
  | Some(preset) =>
    let session = {
      preset: Some(name),
      adjustments: [],
    };
    let* () =
      checkKnown(
        ~known,
        Names.Set.union(
          Selection.referencedNames(preset),
          selection(config, session),
        ),
      );
    Ok(session);
  };

let adjust = (~known, session, modifiers) => {
  let* () =
    checkKnown(
      ~known,
      Names.Set.of_list(List.map(Selection.modifierName, modifiers)),
    );
  Ok({
    ...session,
    adjustments: session.adjustments @ modifiers,
  });
};
