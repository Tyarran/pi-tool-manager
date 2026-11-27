type text = {
  enum: option(list(string)),
  pattern: option(string),
  deny: list(string),
};

type kind =
  | String(text)
  | Array(text)
  | Integer
  | Number
  | Boolean;

type requirement =
  | Optional
  | Required
  | RequiredWhen(Names.Map.t(list(string)));

type param = {
  kind,
  description: option(string),
  requirement,
  flag: option(string),
};

type token =
  | Literal(string)
  | Placeholder(string);

type command =
  | Argv(list(token))
  | Shell(string);

type output = {
  maxLines: int,
  maxBytes: int,
};

type t = {
  name: string,
  description: string,
  command,
  parameters: Names.Map.t(param),
  timeout: int,
  cwd: option(string),
  env: Names.Map.t(token),
  output,
};

let defaultTimeout = 120;

let defaultOutput = {
  maxLines: 2000,
  maxBytes: 51200,
};

let ( let* ) = Result.bind;

let placeholder = name => "{" ++ name ++ "}";

let strings = Decode.list(Decode.string);

let decodeText = (path, fields) => {
  let* enum = Decode.optionalField(strings, "enum", path, fields);
  let* pattern = Decode.optionalField(Decode.string, "pattern", path, fields);
  let* deny = Decode.fieldOr(strings, "deny", ~default=[], path, fields);
  switch (enum) {
  | Some([]) => Decode.fail(path ++ ".enum", "must not be empty")
  | Some([_, ..._])
  | None =>
    Ok({
      enum,
      pattern,
      deny,
    })
  };
};

let textConstraints = ["enum", "pattern", "deny"];

let withoutTextConstraints = (kind, path, fields) =>
  switch (
    List.find_opt(
      key => Option.is_some(Js.Dict.get(fields, key)),
      textConstraints,
    )
  ) {
  | Some(key) =>
    Decode.fail(
      path ++ "." ++ key,
      "only applies to string and array parameters",
    )
  | None => Ok(kind)
  };

let decodeKind = (path, fields) => {
  let* name = Decode.field(Decode.string, "type", path, fields);
  switch (name) {
  | "string" => decodeText(path, fields) |> Result.map(text => String(text))
  | "array" => decodeText(path, fields) |> Result.map(text => Array(text))
  | "integer" => withoutTextConstraints(Integer, path, fields)
  | "number" => withoutTextConstraints(Number, path, fields)
  | "boolean" => withoutTextConstraints(Boolean, path, fields)
  | _ =>
    Decode.fail(
      path ++ ".type",
      "must be one of \"string\", \"integer\", \"number\", \"boolean\", \"array\"",
    )
  };
};

let decodeRequirement = (path, fields) => {
  let* required =
    Decode.fieldOr(Decode.bool, "required", ~default=false, path, fields);
  let* conditions =
    Decode.optionalField(
      Decode.dict(_ => strings),
      "requiredWhen",
      path,
      fields,
    );
  switch (required, conditions) {
  | (true, Some(_)) =>
    Decode.fail(path, "cannot combine required and requiredWhen")
  | (true, None) => Ok(Required)
  | (false, Some(conditions)) => Ok(RequiredWhen(conditions))
  | (false, None) => Ok(Optional)
  };
};

let decodeParam = (_name, path, json) => {
  let* fields = Decode.object_(path, json);
  let* kind = decodeKind(path, fields);
  let* description =
    Decode.optionalField(Decode.string, "description", path, fields);
  let* requirement = decodeRequirement(path, fields);
  let* flag = Decode.optionalField(Decode.string, "flag", path, fields);
  Ok({
    kind,
    description,
    requirement,
    flag,
  });
};

let hasBrace = text =>
  String.contains(text, '{') || String.contains(text, '}');

let placeholderName = text => {
  let length = String.length(text);
  length > 2 && text.[0] == '{' && text.[length - 1] == '}'
    ? Some(String.sub(text, 1, length - 2)) : None;
};

let decodeToken = (path, json) => {
  let* text = Decode.string(path, json);
  switch (placeholderName(text)) {
  | Some(name) when !hasBrace(name) => Ok(Placeholder(name))
  | Some(_)
  | None =>
    hasBrace(text)
      ? Decode.fail(
          path,
          "placeholders must be standalone tokens such as \"{name}\"",
        )
      : Ok(Literal(text))
  };
};

let decodeArgv = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONString(_) =>
    Decode.fail(
      path,
      "must be an array of strings; set shell to true to run a shell command",
    )
  | _ =>
    let* tokens = Decode.list(decodeToken, path, json);
    switch (tokens) {
    | [] => Decode.fail(path, "must not be empty")
    | [Placeholder(_), ..._] =>
      Decode.fail(path ++ "[0]", "must be the literal program to run")
    | [Literal(_), ..._] => Ok(Argv(tokens))
    };
  };

let decodeShell = (path, json) => {
  let* text = Decode.string(path, json);
  String.trim(text) == ""
    ? Decode.fail(path, "must not be empty") : Ok(Shell(text));
};

let tokenPlaceholder = token =>
  switch (token) {
  | Placeholder(name) => Some(name)
  | Literal(_) => None
  };

let commandPlaceholders = (command, ~declared) =>
  switch (command) {
  | Argv(tokens) =>
    Names.Set.of_list(List.filter_map(tokenPlaceholder, tokens))
  | Shell(text) =>
    Names.Set.filter(
      name => Js.String.includes(~search=placeholder(name), text),
      declared,
    )
  };

let conditionNames = parameters =>
  Names.Map.fold(
    (_, param, names) =>
      switch (param.requirement) {
      | RequiredWhen(conditions) =>
        Names.Set.union(Names.keys(conditions), names)
      | Required
      | Optional => names
      },
    parameters,
    Names.Set.empty,
  );

let checkReferences = (path, command, env, parameters) => {
  let declared = Names.keys(parameters);
  let used =
    Names.Set.union(
      commandPlaceholders(command, ~declared),
      Names.Map.fold(
        (_, token, used) =>
          switch (tokenPlaceholder(token)) {
          | Some(name) => Names.Set.add(name, used)
          | None => used
          },
        env,
        Names.Set.empty,
      ),
    );
  switch (
    Names.Set.min_elt_opt(Names.Set.diff(used, declared)),
    Names.Set.min_elt_opt(Names.Set.diff(declared, used)),
    Names.Set.min_elt_opt(
      Names.Set.diff(conditionNames(parameters), declared),
    ),
  ) {
  | (Some(name), _, _) =>
    Decode.fail(
      path,
      Printf.sprintf("uses an undeclared parameter \"%s\"", name),
    )
  | (None, Some(name), _) =>
    Decode.fail(
      path,
      Printf.sprintf("declares an unused parameter \"%s\"", name),
    )
  | (None, None, Some(name)) =>
    Decode.fail(
      path,
      Printf.sprintf(
        "requiredWhen refers to an unknown parameter \"%s\"",
        name,
      ),
    )
  | (None, None, None) => Ok()
  };
};

let decodeOutput = (path, fields) => {
  let* maxLines =
    Decode.fieldOr(
      Decode.positiveInt,
      "maxLines",
      ~default=defaultOutput.maxLines,
      path,
      fields,
    );
  let* maxBytes =
    Decode.fieldOr(
      Decode.positiveInt,
      "maxBytes",
      ~default=defaultOutput.maxBytes,
      path,
      fields,
    );
  Ok({
    maxLines,
    maxBytes,
  });
};

let decode = (name, path, json) => {
  let* fields = Decode.object_(path, json);
  let* description = Decode.field(Decode.string, "description", path, fields);
  let* shell =
    Decode.fieldOr(Decode.bool, "shell", ~default=false, path, fields);
  let* command =
    Decode.field(shell ? decodeShell : decodeArgv, "command", path, fields);
  let* parameters =
    Decode.fieldOr(
      Decode.dict(decodeParam),
      "parameters",
      ~default=Names.Map.empty,
      path,
      fields,
    );
  let* timeout =
    Decode.fieldOr(
      Decode.positiveInt,
      "timeout",
      ~default=defaultTimeout,
      path,
      fields,
    );
  let* cwd = Decode.optionalField(Decode.string, "cwd", path, fields);
  let* env =
    Decode.fieldOr(
      Decode.dict(_ => decodeToken),
      "env",
      ~default=Names.Map.empty,
      path,
      fields,
    );
  let* output = decodeOutput(path, fields);
  let* () = checkReferences(path, command, env, parameters);
  Ok({
    name,
    description,
    command,
    parameters,
    timeout,
    cwd,
    env,
    output,
  });
};
