type t('a) = (string, Js.Json.t) => result('a, string);

let fail: (string, string) => result('a, string);

let string: t(string);

let bool: t(bool);

let int: t(int);

let number: t(float);

let positiveInt: t(int);

let list: t('a) => t(list('a));

let object_: t(Js.Dict.t(Js.Json.t));

let dict: (string => t('a)) => t(Names.Map.t('a));

let field:
  (t('a), string, string, Js.Dict.t(Js.Json.t)) => result('a, string);

let optionalField:
  (t('a), string, string, Js.Dict.t(Js.Json.t)) =>
  result(option('a), string);

let fieldOr:
  (t('a), string, ~default: 'a, string, Js.Dict.t(Js.Json.t)) =>
  result('a, string);
