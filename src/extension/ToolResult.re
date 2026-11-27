[@mel.obj]
external make:
  (~content: array(Js.Json.t), ~isError: bool, unit) => Pi.toolResult;

let text = value =>
  Js.Json.object_(
    Js.Dict.fromList([
      ("type", Js.Json.string("text")),
      ("text", Js.Json.string(value)),
    ]),
  );

let success = message =>
  make(~content=[|text(message)|], ~isError=false, ());

let failure = message =>
  make(~content=[|text(message)|], ~isError=true, ());
