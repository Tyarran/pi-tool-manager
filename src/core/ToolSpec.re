type paramType =
  | String
  | Integer
  | Number
  | Boolean
  | Array;

type param = {
  name: string,
  type_: paramType,
  description: option(string),
  required: bool,
  requiredWhen: list((string, list(string))),
  enum: option(list(string)),
  pattern: option(string),
  deny: list(string),
  flag: option(string),
};

type token =
  | Literal(string)
  | Placeholder(string);

type command =
  | Argv(list(token))
  | Shell(string);

type t = {
  name: string,
  description: string,
  command,
  parameters: list(param),
  timeout: int,
  cwd: option(string),
  env: list((string, token)),
  maxLines: option(int),
  maxBytes: option(int),
};

let defaultTimeout = 120;

let all = Results.all;

let field = Js.Dict.get;

let fail = (path, message) => Error(path ++ " " ++ message);

let decodeString = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONString(value) => Ok(value)
  | _ => fail(path, "must be a string")
  };

let decodeOptional = (decode, path, dict, key) =>
  switch (field(dict, key)) {
  | None => Ok(None)
  | Some(json) =>
    decode(path ++ "." ++ key, json) |> Result.map(Option.some)
  };

let decodePositiveInt = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONNumber(value) when Float.is_integer(value) && value >= 1.0 =>
    Ok(int_of_float(value))
  | _ => fail(path, "must be a positive integer")
  };

let decodeBool = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONTrue => Ok(true)
  | JSONFalse => Ok(false)
  | _ => fail(path, "must be a boolean")
  };

let decodeStrings = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONArray(items) =>
    items
    |> Array.to_list
    |> List.mapi((index, item) =>
         decodeString(Printf.sprintf("%s[%d]", path, index), item)
       )
    |> all
  | _ => fail(path, "must be an array of strings")
  };

let decodeType = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONString("string") => Ok(String)
  | JSONString("integer") => Ok(Integer)
  | JSONString("number") => Ok(Number)
  | JSONString("boolean") => Ok(Boolean)
  | JSONString("array") => Ok(Array)
  | _ =>
    fail(
      path,
      "must be one of \"string\", \"integer\", \"number\", \"boolean\", \"array\"",
    )
  };

let decodeRequiredWhen = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONObject(conditions) =>
    conditions
    |> Js.Dict.entries
    |> Array.to_list
    |> List.map(((name, values)) =>
         decodeStrings(path ++ "." ++ name, values)
         |> Result.map(values => (name, values))
       )
    |> all
  | _ => fail(path, "must be an object of string arrays")
  };

let ( let* ) = Result.bind;

let decodeParam = (path, name, json) =>
  switch (Js.Json.classify(json)) {
  | JSONObject(dict) =>
    let* type_ =
      switch (field(dict, "type")) {
      | None => fail(path ++ ".type", "is required")
      | Some(json) => decodeType(path ++ ".type", json)
      };
    let* description =
      decodeOptional(decodeString, path, dict, "description");
    let* required = decodeOptional(decodeBool, path, dict, "required");
    let* requiredWhen =
      decodeOptional(decodeRequiredWhen, path, dict, "requiredWhen");
    let* enum = decodeOptional(decodeStrings, path, dict, "enum");
    let* pattern = decodeOptional(decodeString, path, dict, "pattern");
    let* deny = decodeOptional(decodeStrings, path, dict, "deny");
    let* flag = decodeOptional(decodeString, path, dict, "flag");
    let textual = type_ == String || type_ == Array;
    switch (enum, pattern, deny) {
    | (Some([]), _, _) => fail(path ++ ".enum", "must not be empty")
    | (Some(_), _, _) when !textual =>
      fail(path ++ ".enum", "only applies to string and array parameters")
    | (_, Some(_), _) when !textual =>
      fail(path ++ ".pattern", "only applies to string and array parameters")
    | (_, _, Some(_)) when !textual =>
      fail(path ++ ".deny", "only applies to string and array parameters")
    | _ =>
      Ok({
        name,
        type_,
        description,
        required: Option.value(required, ~default=false),
        requiredWhen: Option.value(requiredWhen, ~default=[]),
        enum,
        pattern,
        deny: Option.value(deny, ~default=[]),
        flag,
      })
    };
  | _ => fail(path, "must be an object")
  };

let decodeParams = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONObject(dict) =>
    dict
    |> Js.Dict.entries
    |> Array.to_list
    |> List.map(((name, json)) =>
         decodeParam(path ++ "." ++ name, name, json)
       )
    |> all
  | _ => fail(path, "must be an object")
  };

let placeholderName = text => {
  let length = String.length(text);
  length > 2 && text.[0] == '{' && text.[length - 1] == '}'
    ? Some(String.sub(text, 1, length - 2)) : None;
};

let containsBrace = text =>
  String.contains(text, '{') || String.contains(text, '}');

let decodeToken = (path, text) =>
  switch (placeholderName(text)) {
  | Some(name) when !containsBrace(name) => Ok(Placeholder(name))
  | _ when containsBrace(text) =>
    fail(path, "placeholders must be standalone tokens such as \"{name}\"")
  | _ => Ok(Literal(text))
  };

let decodeArgv = (path, json) =>
  switch (decodeStrings(path, json)) {
  | Error(_) =>
    fail(
      path,
      "must be an array of strings (set shell to true to run a shell command)",
    )
  | Ok([]) => fail(path, "must not be empty")
  | Ok(tokens) =>
    let* tokens =
      tokens
      |> List.mapi((index, token) =>
           decodeToken(Printf.sprintf("%s[%d]", path, index), token)
         )
      |> all;
    switch (tokens) {
    | [Placeholder(_), ..._] =>
      fail(path ++ "[0]", "must be the literal program to run")
    | tokens => Ok(Argv(tokens))
    };
  };

let decodeShell = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONString(text) when String.trim(text) == "" =>
    fail(path, "must not be empty")
  | JSONString(text) => Ok(Shell(text))
  | _ => fail(path, "must be a string when shell is true")
  };

let decodeCommand = (~shell, path, json) =>
  shell ? decodeShell(path, json) : decodeArgv(path, json);

let decodeEnv = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONObject(dict) =>
    dict
    |> Js.Dict.entries
    |> Array.to_list
    |> List.map(((name, value)) => {
         let valuePath = path ++ "." ++ name;
         let* text = decodeString(valuePath, value);
         decodeToken(valuePath, text) |> Result.map(token => (name, token));
       })
    |> all
  | _ => fail(path, "must be an object of strings")
  };

let placeholders = tokens =>
  List.filter_map(
    token =>
      switch (token) {
      | Placeholder(name) => Some(name)
      | Literal(_) => None
      },
    tokens,
  );

let shellPlaceholder = name => "{" ++ name ++ "}";

let commandPlaceholders = (command, ~declared) =>
  switch (command) {
  | Argv(tokens) => placeholders(tokens)
  | Shell(text) =>
    List.filter(
      name => Js.String.includes(~search=shellPlaceholder(name), text),
      declared,
    )
  };

let checkReferences = (path, command, env, parameters) => {
  let declared = List.map((param: param) => param.name, parameters);
  let used =
    commandPlaceholders(command, ~declared)
    @ placeholders(List.map(snd, env));
  let undeclared = List.filter(name => !List.mem(name, declared), used);
  let unused = List.filter(name => !List.mem(name, used), declared);
  let unknownConditions =
    parameters
    |> List.concat_map((param: param) => List.map(fst, param.requiredWhen))
    |> List.filter(name => !List.mem(name, declared));
  switch (undeclared, unused, unknownConditions) {
  | ([name, ..._], _, _) =>
    fail(path, Printf.sprintf("uses an undeclared parameter \"%s\"", name))
  | (_, [name, ..._], _) =>
    fail(path, Printf.sprintf("declares an unused parameter \"%s\"", name))
  | (_, _, [name, ..._]) =>
    fail(
      path,
      Printf.sprintf(
        "requiredWhen refers to an unknown parameter \"%s\"",
        name,
      ),
    )
  | ([], [], []) => Ok()
  };
};

let decodeTool = (path, name, json) =>
  switch (Js.Json.classify(json)) {
  | JSONObject(dict) =>
    let* description =
      switch (field(dict, "description")) {
      | None => fail(path ++ ".description", "is required")
      | Some(json) => decodeString(path ++ ".description", json)
      };
    let* shell = decodeOptional(decodeBool, path, dict, "shell");
    let* command =
      switch (field(dict, "command")) {
      | None => fail(path ++ ".command", "is required")
      | Some(json) =>
        decodeCommand(
          ~shell=Option.value(shell, ~default=false),
          path ++ ".command",
          json,
        )
      };
    let* parameters = decodeOptional(decodeParams, path, dict, "parameters");
    let parameters = Option.value(parameters, ~default=[]);
    let* timeout = decodeOptional(decodePositiveInt, path, dict, "timeout");
    let* cwd = decodeOptional(decodeString, path, dict, "cwd");
    let* env = decodeOptional(decodeEnv, path, dict, "env");
    let env = Option.value(env, ~default=[]);
    let* maxLines = decodeOptional(decodePositiveInt, path, dict, "maxLines");
    let* maxBytes = decodeOptional(decodePositiveInt, path, dict, "maxBytes");
    let* () = checkReferences(path, command, env, parameters);
    Ok({
      name,
      description,
      command,
      parameters,
      timeout: Option.value(timeout, ~default=defaultTimeout),
      cwd,
      env,
      maxLines,
      maxBytes,
    });
  | _ => fail(path, "must be an object")
  };

let decode = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONObject(dict) =>
    dict
    |> Js.Dict.entries
    |> Array.to_list
    |> List.map(((name, json)) =>
         decodeTool(path ++ "." ++ name, name, json)
       )
    |> all
  | _ => fail(path, "must be an object")
  };
