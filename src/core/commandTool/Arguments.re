type value =
  | Text(string)
  | Texts(list(string))
  | Int(int)
  | Float(float)
  | Bool(bool);

type t = Names.Map.t(value);

let ( let* ) = Result.bind;

let startsWith = (~prefix, text) =>
  String.length(text) >= String.length(prefix)
  && String.sub(text, 0, String.length(prefix)) == prefix;

let isShortOption = text =>
  String.length(text) == 2 && text.[0] == '-' && text.[1] != '-';

let isShortCluster = text =>
  String.length(text) >= 2 && text.[0] == '-' && text.[1] != '-';

let denies = (text, denied) =>
  text == denied
  || startsWith(~prefix=denied ++ "=", text)
  || isShortOption(denied)
  && isShortCluster(text)
  && String.contains_from(text, 1, denied.[1]);

let checkEnum = (name, enum, text) =>
  switch (enum) {
  | Some(allowed) when !List.mem(text, allowed) =>
    Decode.fail(
      name,
      Printf.sprintf(
        "must be one of %s, got \"%s\"",
        String.concat(", ", allowed),
        text,
      ),
    )
  | Some(_)
  | None => Ok()
  };

let matches = (pattern, text) =>
  Js.Re.test(~str=text, Js.Re.fromStringWithFlags(pattern, ~flags="u"));

let checkPattern = (name, pattern, text) =>
  switch (pattern) {
  | Some(pattern) when !matches(pattern, text) =>
    Decode.fail(
      name,
      Printf.sprintf("must match %s, got \"%s\"", pattern, text),
    )
  | Some(_)
  | None => Ok()
  };

let checkDeny = (name, deny, text) =>
  List.exists(denies(text), deny)
    ? Decode.fail(name, Printf.sprintf("does not allow \"%s\"", text)) : Ok();

let checkText = (name, constraints: Spec.text, text) => {
  let* () = checkEnum(name, constraints.enum, text);
  let* () = checkPattern(name, constraints.pattern, text);
  checkDeny(name, constraints.deny, text);
};

let decodeValue = (name, kind: Spec.kind, json) =>
  switch (kind) {
  | String(constraints) =>
    let* text = Decode.string(name, json);
    let* () = checkText(name, constraints, text);
    Ok(Text(text));
  | Array(constraints) =>
    let* texts = Decode.list(Decode.string, name, json);
    let* () = Results.iter(checkText(name, constraints), texts);
    Ok(Texts(texts));
  | Integer => Decode.int(name, json) |> Result.map(number => Int(number))
  | Number =>
    Decode.number(name, json) |> Result.map(number => Float(number))
  | Boolean => Decode.bool(name, json) |> Result.map(flag => Bool(flag))
  };

let provided = (args, name) =>
  switch (Js.Dict.get(args, name)) {
  | Some(json) when !Js.Json.test(json, Null) => Some(json)
  | Some(_)
  | None => None
  };

let metCondition = (values, (condition, expected)) =>
  switch (Names.Map.find_opt(condition, values)) {
  | Some(Text(text)) when List.mem(text, expected) =>
    Some((condition, text))
  | Some(Text(_) | Texts(_) | Int(_) | Float(_) | Bool(_))
  | None => None
  };

let checkPresence = (values, (name, param: Spec.param)) =>
  if (Names.Map.mem(name, values)) {
    Ok();
  } else {
    switch (param.requirement) {
    | Optional => Ok()
    | Required => Decode.fail(name, "is required")
    | RequiredWhen(conditions) =>
      switch (
        List.find_map(metCondition(values), Names.Map.bindings(conditions))
      ) {
      | Some((condition, text)) =>
        Decode.fail(
          name,
          Printf.sprintf("is required when %s is \"%s\"", condition, text),
        )
      | None => Ok()
      }
    };
  };

let decode = (spec: Spec.t, args) => {
  let parameters = Names.Map.bindings(spec.parameters);
  let* decoded =
    parameters
    |> List.filter_map(((name, param: Spec.param)) =>
         provided(args, name) |> Option.map(json => (name, param.kind, json))
       )
    |> Results.map(((name, kind, json)) =>
         decodeValue(name, kind, json) |> Result.map(value => (name, value))
       );
  let values = Names.Map.of_seq(List.to_seq(decoded));
  let* () = Results.iter(checkPresence(values), parameters);
  Ok(values);
};
