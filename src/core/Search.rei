type document;

let document:
  (
    ~name: string,
    ~description: string,
    ~parameters: Js.Json.t,
    ~namespace: list(string)
  ) =>
  document;

let rank: (string, list(document), ~limit: int) => list(string);
