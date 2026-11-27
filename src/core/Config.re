type t = {
  baseTools: Names.Set.t,
  defaultPreset: option(string),
  presets: Names.Map.t(Selection.preset),
  tools: list(CommandTool.Spec.t),
};

let key = "pi-tool-manager";

let ( let* ) = Result.bind;

let parsed = (parse, path, json) => {
  let* texts = Decode.list(Decode.string, path, json);
  parse(texts) |> Result.map_error(message => path ++ " " ++ message);
};

let toolNames = parsed(Selection.parseNames);

let preset = _name => parsed(Selection.parsePreset);

let checkDefaultPreset = (path, presets, defaultPreset) =>
  switch (defaultPreset) {
  | Some(name) when !Names.Map.mem(name, presets) =>
    Decode.fail(
      path ++ ".defaultPreset",
      Printf.sprintf("names an unknown preset \"%s\"", name),
    )
  | Some(_)
  | None => Ok()
  };

let decodeConfig = (path, json) => {
  let* fields = Decode.object_(path, json);
  let* baseTools = Decode.field(toolNames, "baseTools", path, fields);
  let* presets =
    Decode.fieldOr(
      Decode.dict(preset),
      "presets",
      ~default=Names.Map.empty,
      path,
      fields,
    );
  let* defaultPreset =
    Decode.optionalField(Decode.string, "defaultPreset", path, fields);
  let* () = checkDefaultPreset(path, presets, defaultPreset);
  let* tools =
    Decode.fieldOr(
      Decode.dict(CommandTool.Spec.decode),
      "tools",
      ~default=Names.Map.empty,
      path,
      fields,
    );
  Ok({
    baseTools,
    defaultPreset,
    presets,
    tools: List.map(snd, Names.Map.bindings(tools)),
  });
};

let decode = settings =>
  switch (settings) {
  | None => Ok(None)
  | Some(json) => decodeConfig(key, json) |> Result.map(Option.some)
  };

let presetNames = config =>
  List.map(fst, Names.Map.bindings(config.presets));
