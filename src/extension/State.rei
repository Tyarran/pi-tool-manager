type t =
  | NotConfigured
  | Invalid(string)
  | Active(Config.t, Session.t);

let get: unit => t;

let set: t => unit;

let fromSettings: Js.Dict.t(Js.Json.t) => t;

let selection: unit => Names.Set.t;

let unavailableReason: t => option(string);
