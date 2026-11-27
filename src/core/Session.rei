type t = {
  preset: option(string),
  adjustments: list(Selection.modifier),
};

let start: Config.t => t;

let selection: (Config.t, t) => Names.Set.t;

let active: (Config.t, t) => Names.Set.t;

let allows: (Config.t, t, string) => bool;

let needsSync:
  (Config.t, t, ~known: Names.Set.t, ~active: Names.Set.t) => bool;

let switchPreset:
  (Config.t, ~known: Names.Set.t, string) => result(t, string);

let adjust:
  (~known: Names.Set.t, t, list(Selection.modifier)) => result(t, string);
