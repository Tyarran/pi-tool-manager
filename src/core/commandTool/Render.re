let shellProgram = "/bin/sh";

let expandHome = (~home, text) =>
  if (text == "~") {
    home;
  } else if (String.length(text) >= 2 && String.sub(text, 0, 2) == "~/") {
    home ++ String.sub(text, 1, String.length(text) - 1);
  } else {
    text;
  };

let texts = (value: Arguments.value) =>
  switch (value) {
  | Text(text) => [text]
  | Texts(texts) => texts
  | Int(number) => [string_of_int(number)]
  | Float(number) => [Js.Float.toString(number)]
  | Bool(flag) => [string_of_bool(flag)]
  };

let withFlag = (flag, value: Arguments.value) =>
  switch (value) {
  | Bool(enabled) => enabled ? [flag] : []
  | Text(_)
  | Texts(_)
  | Int(_)
  | Float(_) => List.concat_map(text => [flag, text], texts(value))
  };

let argumentsFor = (spec: Spec.t, values, name) =>
  switch (
    Names.Map.find_opt(name, spec.parameters),
    Names.Map.find_opt(name, values),
  ) {
  | (Some({ flag: Some(flag), _ }), Some(value)) => withFlag(flag, value)
  | (Some({ flag: None, _ }), Some(value)) => texts(value)
  | (Some(_), None)
  | (None, Some(_))
  | (None, None) => []
  };

let quote = text =>
  "'" ++ String.concat("'\\''", String.split_on_char('\'', text)) ++ "'";

let replaceAll = (~search, ~by, text) =>
  text |> Js.String.split(~sep=search) |> Js.Array.join(~sep=by);

let shellCommand = (spec: Spec.t, values, text) =>
  Names.Map.fold(
    (name, _, text) =>
      replaceAll(
        ~search=Spec.placeholder(name),
        ~by=
          String.concat(
            " ",
            List.map(quote, argumentsFor(spec, values, name)),
          ),
        text,
      ),
    spec.parameters,
    text,
  );

let renderToken = (spec, values, ~home, token: Spec.token) =>
  switch (token) {
  | Literal(text) => [expandHome(~home, text)]
  | Placeholder(name) => argumentsFor(spec, values, name)
  };

let argv = (spec: Spec.t, values, ~home) =>
  switch (spec.command) {
  | Shell(text) => [shellProgram, "-c", shellCommand(spec, values, text)]
  | Argv(tokens) =>
    List.concat_map(renderToken(spec, values, ~home), tokens)
  };

let envValue = (values, ~home, token: Spec.token) =>
  switch (token) {
  | Literal(text) => Some(expandHome(~home, text))
  | Placeholder(name) =>
    Names.Map.find_opt(name, values)
    |> Option.map(value => String.concat(" ", texts(value)))
  };

let env = (spec: Spec.t, values, ~home) =>
  Names.Map.bindings(spec.env)
  |> List.filter_map(((name, token)) =>
       envValue(values, ~home, token) |> Option.map(value => (name, value))
     );

let workingDirectory = (spec: Spec.t, ~home) =>
  Option.map(expandHome(~home), spec.cwd);
