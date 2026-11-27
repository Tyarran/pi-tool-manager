let object_ = fields => Js.Json.object_(Js.Dict.fromList(fields));

let typed = name => ("type", Js.Json.string(name));

let textSchema = (text: Spec.text) =>
  List.concat([
    [typed("string")],
    switch (text.enum) {
    | Some(values) => [
        ("enum", Js.Json.stringArray(Array.of_list(values))),
      ]
    | None => []
    },
    switch (text.pattern) {
    | Some(pattern) => [("pattern", Js.Json.string(pattern))]
    | None => []
    },
  ]);

let kindSchema = (kind: Spec.kind) =>
  switch (kind) {
  | String(text) => textSchema(text)
  | Array(text) => [typed("array"), ("items", object_(textSchema(text)))]
  | Integer => [typed("integer")]
  | Number => [typed("number")]
  | Boolean => [typed("boolean")]
  };

let denied = (kind: Spec.kind) =>
  switch (kind) {
  | String(text)
  | Array(text) => text.deny
  | Integer
  | Number
  | Boolean => []
  };

let condition = ((name, values)) =>
  Printf.sprintf("%s is %s", name, String.concat("|", values));

let requirementHint = (requirement: Spec.requirement) =>
  switch (requirement) {
  | RequiredWhen(conditions) =>
    Some(
      "Required when "
      ++ String.concat(
           " or ",
           List.map(condition, Names.Map.bindings(conditions)),
         )
      ++ ".",
    )
  | Required
  | Optional => None
  };

let denyHint = deny =>
  switch (deny) {
  | [] => None
  | [_, ..._] => Some("Not allowed: " ++ String.concat(", ", deny) ++ ".")
  };

let description = (param: Spec.param) =>
  [
    param.description,
    requirementHint(param.requirement),
    denyHint(denied(param.kind)),
  ]
  |> List.filter_map(Fun.id)
  |> String.concat(" ");

let paramSchema = (param: Spec.param) =>
  object_(
    switch (description(param)) {
    | "" => kindSchema(param.kind)
    | text =>
      kindSchema(param.kind) @ [("description", Js.Json.string(text))]
    },
  );

let isRequired = (param: Spec.param) =>
  switch (param.requirement) {
  | Required => true
  | RequiredWhen(_)
  | Optional => false
  };

let make = (spec: Spec.t) => {
  let parameters = Names.Map.bindings(spec.parameters);
  object_([
    typed("object"),
    (
      "properties",
      object_(
        List.map(
          ((name, param)) => (name, paramSchema(param)),
          parameters,
        ),
      ),
    ),
    (
      "required",
      parameters
      |> List.filter(((_, param)) => isRequired(param))
      |> List.map(fst)
      |> Array.of_list
      |> Js.Json.stringArray,
    ),
    ("additionalProperties", Js.Json.boolean(false)),
  ]);
};
