let words = text =>
  String.split_on_char(' ', text)
  |> List.concat_map(String.split_on_char('\t'))
  |> List.filter(word => word != "");

let presetName = (~available, text) =>
  switch (words(text)) {
  | [name] => Ok(name)
  | [] =>
    Error(
      Printf.sprintf(
        "Usage: /tool-preset <name> (available: %s)",
        String.concat(", ", available),
      ),
    )
  | [_, _, ..._] => Error("Usage: /tool-preset <name>")
  };

let adjustments = text =>
  switch (words(text)) {
  | [] => Error("Usage: /tool +name -name ...")
  | words => Selection.parseModifiers(words)
  };
