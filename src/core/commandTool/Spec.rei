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

let placeholder: string => string;

let decode: string => Decode.t(t);
