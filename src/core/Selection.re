let isModifier = Config.isModifier;

let toolName = Config.toolName;

let applyModifiers = (tools, entries) =>
  List.fold_left(
    (tools, entry) =>
      if (!isModifier(entry)) {
        tools;
      } else {
        let name = toolName(entry);
        let without = List.filter(tool => tool != name, tools);
        entry.[0] == '+' ? without @ [name] : without;
      },
    tools,
    entries,
  );

let resolve = (~base, entries) => {
  let plain = List.filter(entry => !isModifier(entry), entries);
  applyModifiers(plain == [] ? base : plain, entries);
};

let unknownTools = (~known, entries) =>
  entries
  |> List.map(toolName)
  |> List.filter(name => !List.mem(name, known))
  |> List.sort_uniq(compare);

let sameTools = (a, b) =>
  List.sort_uniq(compare, a) == List.sort_uniq(compare, b);

let tokenize = text =>
  String.split_on_char(' ', text)
  |> List.concat_map(String.split_on_char('\t'))
  |> List.filter(token => token != "");

let parseAdjustments = text =>
  switch (tokenize(text)) {
  | [] => Error("Usage: /tool +name -name ...")
  | tokens =>
    switch (
      List.find_opt(
        token => !isModifier(token) || toolName(token) == "",
        tokens,
      )
    ) {
    | Some(token) =>
      Error(
        Printf.sprintf(
          "Invalid entry \"%s\": use +name to add a tool and -name to remove one",
          token,
        ),
      )
    | None => Ok(tokens)
    }
  };
