type modifier =
  | Add(string)
  | Remove(string);

type preset = {
  replacement: option(Names.Set.t),
  modifiers: list(modifier),
};

type entry =
  | Name(string)
  | Modifier(modifier);

let ( let* ) = Result.bind;

let parseEntry = text =>
  switch (text) {
  | ""
  | "+"
  | "-" => Error("contains an empty tool name")
  | _ =>
    let name = String.sub(text, 1, String.length(text) - 1);
    switch (text.[0]) {
    | '+' => Ok(Modifier(Add(name)))
    | '-' => Ok(Modifier(Remove(name)))
    | _ => Ok(Name(text))
    };
  };

let nameOf = entry =>
  switch (entry) {
  | Name(name) => Some(name)
  | Modifier(_) => None
  };

let modifierOf = entry =>
  switch (entry) {
  | Name(_) => None
  | Modifier(modifier) => Some(modifier)
  };

let parseNames = texts =>
  texts
  |> Results.map(text => {
       let* entry = parseEntry(text);
       switch (entry) {
       | Name(name) => Ok(name)
       | Modifier(_) =>
         Error(
           Printf.sprintf("must list plain tool names, got \"%s\"", text),
         )
       };
     })
  |> Result.map(Names.Set.of_list);

let parsePreset = texts => {
  let* entries = Results.map(parseEntry, texts);
  let replacement =
    switch (List.filter_map(nameOf, entries)) {
    | [] => None
    | names => Some(Names.Set.of_list(names))
    };
  Ok({
    replacement,
    modifiers: List.filter_map(modifierOf, entries),
  });
};

let invalidModifier = text =>
  Error(
    Printf.sprintf(
      "Invalid entry \"%s\": use +name to add a tool and -name to remove one",
      text,
    ),
  );

let parseModifiers = texts =>
  texts
  |> Results.map(text =>
       switch (parseEntry(text)) {
       | Ok(Modifier(modifier)) => Ok(modifier)
       | Ok(Name(_))
       | Error(_) => invalidModifier(text)
       }
     );

let modifierName = modifier =>
  switch (modifier) {
  | Add(name)
  | Remove(name) => name
  };

let applyOne = (tools, modifier) =>
  switch (modifier) {
  | Add(name) => Names.Set.add(name, tools)
  | Remove(name) => Names.Set.remove(name, tools)
  };

let apply = (tools, modifiers) => List.fold_left(applyOne, tools, modifiers);

let resolve = (~base, preset) =>
  apply(Option.value(preset.replacement, ~default=base), preset.modifiers);

let referencedNames = preset =>
  Names.Set.union(
    Option.value(preset.replacement, ~default=Names.Set.empty),
    Names.Set.of_list(List.map(modifierName, preset.modifiers)),
  );
