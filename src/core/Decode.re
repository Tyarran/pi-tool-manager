type t('a) = (string, Js.Json.t) => result('a, string);

let ( let* ) = Result.bind;

let fail = (path, message) => Error(path ++ " " ++ message);

let string = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONString(value) => Ok(value)
  | _ => fail(path, "must be a string")
  };

let bool = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONTrue => Ok(true)
  | JSONFalse => Ok(false)
  | _ => fail(path, "must be a boolean")
  };

let number = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONNumber(value) => Ok(value)
  | _ => fail(path, "must be a number")
  };

let int = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONNumber(value) when Float.is_integer(value) =>
    Ok(int_of_float(value))
  | _ => fail(path, "must be an integer")
  };

let positiveInt = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONNumber(value) when Float.is_integer(value) && value >= 1.0 =>
    Ok(int_of_float(value))
  | _ => fail(path, "must be a positive integer")
  };

let list = (decode, path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONArray(items) =>
    items
    |> Array.to_list
    |> List.mapi((index, item) => (index, item))
    |> Results.map(((index, item)) =>
         decode(Printf.sprintf("%s[%d]", path, index), item)
       )
  | _ => fail(path, "must be an array")
  };

let object_ = (path, json) =>
  switch (Js.Json.classify(json)) {
  | JSONObject(fields) => Ok(fields)
  | _ => fail(path, "must be an object")
  };

let dict = (decode, path, json) => {
  let* fields = object_(path, json);
  let* bindings =
    fields
    |> Js.Dict.entries
    |> Array.to_list
    |> Results.map(((key, value)) =>
         decode(key, path ++ "." ++ key, value)
         |> Result.map(decoded => (key, decoded))
       );
  Ok(Names.Map.of_seq(List.to_seq(bindings)));
};

let optionalField = (decode, key, path, fields) =>
  switch (Js.Dict.get(fields, key)) {
  | None => Ok(None)
  | Some(json) =>
    decode(path ++ "." ++ key, json) |> Result.map(Option.some)
  };

let field = (decode, key, path, fields) => {
  let* value = optionalField(decode, key, path, fields);
  switch (value) {
  | Some(value) => Ok(value)
  | None => fail(path ++ "." ++ key, "is required")
  };
};

let fieldOr = (decode, key, ~default, path, fields) =>
  optionalField(decode, key, path, fields)
  |> Result.map(Option.value(~default));
