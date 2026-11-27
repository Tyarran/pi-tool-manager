type modifier =
  | Add(string)
  | Remove(string);

type preset = {
  replacement: option(Names.Set.t),
  modifiers: list(modifier),
};

let parseNames: list(string) => result(Names.Set.t, string);

let parsePreset: list(string) => result(preset, string);

let parseModifiers: list(string) => result(list(modifier), string);

let modifierName: modifier => string;

let apply: (Names.Set.t, list(modifier)) => Names.Set.t;

let resolve: (~base: Names.Set.t, preset) => Names.Set.t;

let referencedNames: preset => Names.Set.t;
