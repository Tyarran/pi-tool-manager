let json = Js.Json.parseExn;

let fields = text =>
  switch (Js.Json.decodeObject(json(text))) {
  | Some(fields) => fields
  | None => failwith("expected a JSON object")
  };

let names = Names.Set.of_list;

let message = result =>
  switch (result) {
  | Ok(_) => "ok"
  | Error(message) => message
  };
