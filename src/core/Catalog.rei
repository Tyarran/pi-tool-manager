type tool = {
  name: string,
  description: string,
};

type found = {
  name: string,
  description: string,
  schema: Js.Json.t,
};

let callDescription: (~guidelines: list(string), list(tool)) => string;

let foundTools: list(found) => string;

let expectedParameters: (~name: string, Js.Json.t) => string;

let blocked: string => string;
