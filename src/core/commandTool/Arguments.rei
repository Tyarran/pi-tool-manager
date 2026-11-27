type value =
  | Text(string)
  | Texts(list(string))
  | Int(int)
  | Float(float)
  | Bool(bool);

type t = Names.Map.t(value);

let decode: (Spec.t, Js.Dict.t(Js.Json.t)) => result(t, string);
