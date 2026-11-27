type t = {
  baseTools: Names.Set.t,
  defaultPreset: option(string),
  presets: Names.Map.t(Selection.preset),
  tools: list(CommandTool.Spec.t),
};

let key: string;

let decode: option(Js.Json.t) => result(option(t), string);

let presetNames: t => list(string);
