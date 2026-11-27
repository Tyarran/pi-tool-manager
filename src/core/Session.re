type t = {
  preset: option(string),
  adjustments: list(string),
};

let start = (config: Config.t) => {
  preset: config.defaultPreset,
  adjustments: [],
};

let presetEntries = (config: Config.t, preset) =>
  switch (preset) {
  | None => []
  | Some(name) =>
    Option.value(List.assoc_opt(name, config.presets), ~default=[])
  };

let tools = (config: Config.t, session) =>
  Selection.applyModifiers(
    Selection.resolve(
      ~base=config.baseTools,
      presetEntries(config, session.preset),
    ),
    session.adjustments,
  );

let unknownToolsError = names =>
  Error(Printf.sprintf("Unknown tools: %s", String.concat(", ", names)));

let presetNames = (config: Config.t) =>
  String.concat(", ", List.map(fst, config.presets));

let switchPreset = (config: Config.t, ~known, text) =>
  switch (Selection.tokenize(text)) {
  | [] =>
    Error(
      Printf.sprintf(
        "Usage: /tool-preset <name> (available: %s)",
        presetNames(config),
      ),
    )
  | [name] =>
    switch (List.assoc_opt(name, config.presets)) {
    | None =>
      Error(
        Printf.sprintf(
          "Unknown preset \"%s\" (available: %s)",
          name,
          presetNames(config),
        ),
      )
    | Some(entries) =>
      let session = {
        preset: Some(name),
        adjustments: [],
      };
      switch (
        Selection.unknownTools(~known, entries @ tools(config, session))
      ) {
      | [] => Ok(session)
      | names => unknownToolsError(names)
      };
    }
  | _ => Error("Usage: /tool-preset <name>")
  };

let adjust = (~known, session, text) =>
  switch (Selection.parseAdjustments(text)) {
  | Error(_) as error => error
  | Ok(entries) =>
    switch (Selection.unknownTools(~known, entries)) {
    | [] =>
      Ok({
        ...session,
        adjustments: session.adjustments @ entries,
      })
    | names => unknownToolsError(names)
    }
  };
