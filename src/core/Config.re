type t = {
  baseTools: list(string),
  defaultPreset: option(string),
  presets: list((string, list(string))),
};

let path = key => Settings.key ++ "." ++ key;

let all = results =>
  List.fold_right(
    (result, acc) =>
      switch (result, acc) {
      | (Ok(value), Ok(values)) => Ok([value, ...values])
      | (Error(message), _)
      | (_, Error(message)) => Error(message)
      },
    results,
    Ok([]),
  );

let isModifier = entry =>
  String.length(entry) > 0 && (entry.[0] == '+' || entry.[0] == '-');

let toolName = entry =>
  isModifier(entry)
    ? String.sub(entry, 1, String.length(entry) - 1) : entry;

let decodeEntry = (~name, ~allowModifiers, json) =>
  switch (Js.Json.classify(json)) {
  | JSONString(entry) when toolName(entry) == "" =>
    Error(Printf.sprintf("%s contains an empty tool name", name))
  | JSONString(entry) when isModifier(entry) && !allowModifiers =>
    Error(
      Printf.sprintf(
        "%s must list plain tool names, got \"%s\"",
        name,
        entry,
      ),
    )
  | JSONString(entry) => Ok(entry)
  | _ => Error(Printf.sprintf("%s must only contain strings", name))
  };

let decodeToolList = (~name, ~allowModifiers, json) =>
  switch (Js.Json.classify(json)) {
  | JSONArray(entries) =>
    entries
    |> Array.to_list
    |> List.map(decodeEntry(~name, ~allowModifiers))
    |> all
  | _ => Error(Printf.sprintf("%s must be an array of strings", name))
  };

let decodeBaseTools = dict =>
  switch (Js.Dict.get(dict, "baseTools")) {
  | None => Error(Printf.sprintf("%s is required", path("baseTools")))
  | Some(json) =>
    decodeToolList(~name=path("baseTools"), ~allowModifiers=false, json)
  };

let decodePresets = dict =>
  switch (Option.map(Js.Json.classify, Js.Dict.get(dict, "presets"))) {
  | None => Ok([])
  | Some(JSONObject(presets)) =>
    presets
    |> Js.Dict.entries
    |> Array.to_list
    |> List.map(((name, json)) =>
         decodeToolList(
           ~name=path("presets." ++ name),
           ~allowModifiers=true,
           json,
         )
         |> Result.map(entries => (name, entries))
       )
    |> all
  | Some(_) =>
    Error(Printf.sprintf("%s must be an object", path("presets")))
  };

let decodeDefaultPreset = (dict, presets) =>
  switch (Option.map(Js.Json.classify, Js.Dict.get(dict, "defaultPreset"))) {
  | None => Ok(None)
  | Some(JSONString(name)) when List.mem_assoc(name, presets) =>
    Ok(Some(name))
  | Some(JSONString(name)) =>
    Error(
      Printf.sprintf(
        "%s names an unknown preset \"%s\"",
        path("defaultPreset"),
        name,
      ),
    )
  | Some(_) =>
    Error(Printf.sprintf("%s must be a string", path("defaultPreset")))
  };

let decode = (json: option(Js.Json.t)) =>
  switch (Option.map(Js.Json.classify, json)) {
  | None => Ok(None)
  | Some(JSONObject(dict)) =>
    switch (decodeBaseTools(dict), decodePresets(dict)) {
    | (Error(message), _)
    | (_, Error(message)) => Error(message)
    | (Ok(baseTools), Ok(presets)) =>
      decodeDefaultPreset(dict, presets)
      |> Result.map(defaultPreset =>
           Some({
             baseTools,
             defaultPreset,
             presets,
           })
         )
    }
  | Some(_) => Error(Printf.sprintf("%s must be an object", Settings.key))
  };
