open ToolSpec;

type value =
  | Text(string)
  | Whole(int)
  | Decimal(float)
  | Switch(bool)
  | Texts(list(string));

let ( let* ) = Result.bind;

let fail = (param: param, message) =>
  Error(Printf.sprintf("%s %s", param.name, message));

let decodeValue = (param: param, json) =>
  switch (param.type_, Js.Json.classify(json)) {
  | (String, JSONString(text)) => Ok(Text(text))
  | (Integer, JSONNumber(number)) when Float.is_integer(number) =>
    Ok(Whole(int_of_float(number)))
  | (Number, JSONNumber(number)) => Ok(Decimal(number))
  | (Boolean, JSONTrue) => Ok(Switch(true))
  | (Boolean, JSONFalse) => Ok(Switch(false))
  | (Array, JSONArray(items)) =>
    items
    |> Array.to_list
    |> List.map(item =>
         switch (Js.Json.classify(item)) {
         | JSONString(text) => Ok(text)
         | _ => fail(param, "must only contain strings")
         }
       )
    |> Results.all
    |> Result.map(texts => Texts(texts))
  | (String, _) => fail(param, "must be a string")
  | (Integer, _) => fail(param, "must be an integer")
  | (Number, _) => fail(param, "must be a number")
  | (Boolean, _) => fail(param, "must be a boolean")
  | (Array, _) => fail(param, "must be an array of strings")
  };

let startsWith = (~prefix, text) =>
  String.length(text) >= String.length(prefix)
  && String.sub(text, 0, String.length(prefix)) == prefix;

let isShortOption = text =>
  String.length(text) == 2 && text.[0] == '-' && text.[1] != '-';

let isShortCluster = text =>
  String.length(text) >= 2 && text.[0] == '-' && text.[1] != '-';

let isDenied = (deny, text) =>
  List.exists(
    denied =>
      text == denied
      || startsWith(~prefix=denied ++ "=", text)
      || isShortOption(denied)
      && isShortCluster(text)
      && String.contains_from(text, 1, denied.[1]),
    deny,
  );

let matches = (pattern, text) =>
  Js.Re.test(~str=text, Js.Re.fromStringWithFlags(pattern, ~flags="u"));

let checkText = (param: param, text) =>
  switch (param.enum, param.pattern) {
  | (Some(allowed), _) when !List.mem(text, allowed) =>
    fail(
      param,
      Printf.sprintf(
        "must be one of %s, got \"%s\"",
        String.concat(", ", allowed),
        text,
      ),
    )
  | (_, Some(pattern)) when !matches(pattern, text) =>
    fail(param, Printf.sprintf("must match %s, got \"%s\"", pattern, text))
  | _ when isDenied(param.deny, text) =>
    fail(param, Printf.sprintf("does not allow \"%s\"", text))
  | _ => Ok()
  };

let checkValue = (param, value) =>
  switch (value) {
  | Text(text) => checkText(param, text)
  | Texts(texts) =>
    texts |> List.map(checkText(param)) |> Results.all |> Result.map(_ => ())
  | Whole(_)
  | Decimal(_)
  | Switch(_) => Ok()
  };

let conditionMet = (values, (name, expected)) =>
  switch (List.assoc_opt(name, values)) {
  | Some(Text(text)) => List.mem(text, expected)
  | _ => false
  };

let checkPresence = (values, param: param) =>
  switch (
    List.mem_assoc(param.name, values),
    List.find_opt(conditionMet(values), param.requiredWhen),
  ) {
  | (true, _) => Ok()
  | (false, _) when param.required => fail(param, "is required")
  | (false, Some((name, _))) =>
    switch (List.assoc_opt(name, values)) {
    | Some(Text(text)) =>
      fail(
        param,
        Printf.sprintf("is required when %s is \"%s\"", name, text),
      )
    | _ => fail(param, "is required")
    }
  | (false, None) => Ok()
  };

let validate = (spec: ToolSpec.t, args: Js.Dict.t(Js.Json.t)) => {
  let* values =
    spec.parameters
    |> List.filter_map((param: param) =>
         switch (Js.Dict.get(args, param.name)) {
         | None => None
         | Some(json) when Js.Json.test(json, Null) => None
         | Some(json) =>
           Some(
             decodeValue(param, json)
             |> Result.map(value => (param.name, value)),
           )
         }
       )
    |> Results.all;
  let* _ =
    spec.parameters
    |> List.map((param: param) =>
         switch (List.assoc_opt(param.name, values)) {
         | Some(value) => checkValue(param, value)
         | None => checkPresence(values, param)
         }
       )
    |> Results.all;
  Ok(values);
};

let expandHome = (~home, text) =>
  if (text == "~") {
    home;
  } else if (startsWith(~prefix="~/", text)) {
    home ++ String.sub(text, 1, String.length(text) - 1);
  } else {
    text;
  };

let texts = value =>
  switch (value) {
  | Text(text) => [text]
  | Whole(number) => [string_of_int(number)]
  | Decimal(number) => [Js.Float.toString(number)]
  | Switch(flag) => [string_of_bool(flag)]
  | Texts(texts) => texts
  };

let arguments = (param: param, value) =>
  switch (param.flag, value) {
  | (Some(flag), Switch(true)) => [flag]
  | (Some(_), Switch(false)) => []
  | (Some(flag), value) =>
    List.concat_map(text => [flag, text], texts(value))
  | (None, value) => texts(value)
  };

let paramOf = (spec: ToolSpec.t, name) =>
  List.find((param: param) => param.name == name, spec.parameters);

let shellProgram = "/bin/sh";

let renderShell = (spec: ToolSpec.t, values, text) =>
  List.fold_left(
    (text, param: param) => {
      let replacement =
        switch (List.assoc_opt(param.name, values)) {
        | None => ""
        | Some(value) => ShellQuote.join(arguments(param, value))
        };
      text
      |> Js.String.split(~sep=ToolSpec.shellPlaceholder(param.name))
      |> Js.Array.join(~sep=replacement);
    },
    text,
    spec.parameters,
  );

let render = (spec: ToolSpec.t, values, ~home) =>
  switch (spec.command) {
  | Shell(text) => [shellProgram, "-c", renderShell(spec, values, text)]
  | Argv(tokens) =>
    List.concat_map(
      token =>
        switch (token) {
        | Literal(text) => [expandHome(~home, text)]
        | Placeholder(name) =>
          switch (List.assoc_opt(name, values)) {
          | None => []
          | Some(value) => arguments(paramOf(spec, name), value)
          }
        },
      tokens,
    )
  };

let environment = (spec: ToolSpec.t, values, ~home) =>
  List.filter_map(
    ((name, token)) =>
      switch (token) {
      | Literal(text) => Some((name, expandHome(~home, text)))
      | Placeholder(param) =>
        List.assoc_opt(param, values)
        |> Option.map(value => (name, String.concat(" ", texts(value))))
      },
    spec.env,
  );

let typeName = type_ =>
  switch (type_) {
  | String => "string"
  | Integer => "integer"
  | Number => "number"
  | Boolean => "boolean"
  | Array => "array"
  };

let describeParam = (param: param) =>
  [
    param.description,
    switch (param.requiredWhen) {
    | [] => None
    | conditions =>
      Some(
        "Required when "
        ++ String.concat(
             " or ",
             List.map(
               ((name, values)) =>
                 Printf.sprintf(
                   "%s is %s",
                   name,
                   String.concat("|", values),
                 ),
               conditions,
             ),
           )
        ++ ".",
      )
    },
    switch (param.deny) {
    | [] => None
    | deny => Some("Not allowed: " ++ String.concat(", ", deny) ++ ".")
    },
  ]
  |> List.filter_map(part => part)
  |> String.concat(" ");

let textSchema = (param: param) =>
  [("type", Js.Json.string("string"))]
  @ Option.fold(
      ~none=[],
      ~some=
        values => [("enum", Js.Json.stringArray(Array.of_list(values)))],
      param.enum,
    )
  @ Option.fold(
      ~none=[],
      ~some=pattern => [("pattern", Js.Json.string(pattern))],
      param.pattern,
    );

let paramSchema = (param: param) => {
  let base =
    switch (param.type_) {
    | Array => [
        ("type", Js.Json.string("array")),
        ("items", Js.Json.object_(Js.Dict.fromList(textSchema(param)))),
      ]
    | String => textSchema(param)
    | type_ => [("type", Js.Json.string(typeName(type_)))]
    };
  let description =
    switch (describeParam(param)) {
    | "" => []
    | text => [("description", Js.Json.string(text))]
    };
  Js.Json.object_(Js.Dict.fromList(base @ description));
};

let schema = (spec: ToolSpec.t) =>
  Js.Json.object_(
    Js.Dict.fromList([
      ("type", Js.Json.string("object")),
      (
        "properties",
        Js.Json.object_(
          Js.Dict.fromList(
            List.map(
              (param: param) => (param.name, paramSchema(param)),
              spec.parameters,
            ),
          ),
        ),
      ),
      (
        "required",
        spec.parameters
        |> List.filter((param: param) => param.required)
        |> List.map((param: param) => param.name)
        |> Array.of_list
        |> Js.Json.stringArray,
      ),
      ("additionalProperties", Js.Json.boolean(false)),
    ]),
  );
