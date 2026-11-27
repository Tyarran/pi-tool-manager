type content;

type result = {
  content: array(content),
  isError: bool,
};

type outcome = {
  result,
  isError: bool,
  durationMs: option(float),
};

type updateCallback;

type executeOptions = {
  signal: Js.Nullable.t(Node.abortSignal),
  onUpdate: Js.Nullable.t(updateCallback),
};

type context = {cwd: string};

type exposure = string;

type reach;

type loadout;

type declared = {
  name: string,
  description: string,
};

type loadoutChanges = {
  descriptions: Js.Dict.t(string),
  hiddenDeclarations: array(string),
};

type execute('params) =
  (
    string,
    'params,
    Js.Nullable.t(Node.abortSignal),
    Js.Nullable.t(updateCallback),
    context
  ) =>
  Js.Promise.t(result);

type definition('params) = {
  name: string,
  label: string,
  description: string,
  promptSnippet: option(string),
  promptGuidelines: option(array(string)),
  parameters: Js.Json.t,
  exposure: option(exposure),
  defaultActive: option(bool),
  reachesTools: option(Js.Dict.t(Js.Json.t) => reach),
  prepareLoadout: option(loadout => loadoutChanges),
  renderCall: option((Js.Json.t, PiTui.theme, Js.Json.t) => PiTui.component),
  execute: execute('params),
};

let define =
    (
      ~name,
      ~label,
      ~description,
      ~promptSnippet=?,
      ~promptGuidelines=?,
      ~parameters,
      ~exposure=?,
      ~defaultActive=?,
      ~reachesTools=?,
      ~prepareLoadout=?,
      ~renderCall=?,
      execute,
    ) => {
  name,
  label,
  description,
  promptSnippet,
  promptGuidelines,
  parameters,
  exposure,
  defaultActive,
  reachesTools,
  prepareLoadout,
  renderCall,
  execute,
};

type textContent = {
  [@mel.as "type"]
  kind: string,
  text: string,
};

external fromText: textContent => content = "%identity";

external toText: content => textContent = "%identity";

let text = text =>
  fromText({
    kind: "text",
    text,
  });

let textOf = content => {
  let content = toText(content);
  content.kind == "text" ? Some(content.text) : None;
};

let success = message => {
  content: [|text(message)|],
  isError: false,
};

let failure = message => {
  content: [|text(message)|],
  isError: true,
};

[@mel.scope "Object"]
external assign: ([@mel.as {json|{}|json}] _, result, result) => result =
  "assign";

let withContent = (result, ~content, ~isError) =>
  assign(
    result,
    {
      content,
      isError,
    },
  );

[@mel.send]
external executeTool:
  (context, string, Js.Json.t, executeOptions) => Js.Promise.t(outcome) =
  "executeTool";

let modelOnly = "model-only";

external reachOf: 'a => reach = "%identity";

let allTools = reachOf("all");

let someTools = names => reachOf(names);

[@mel.get] external declared: loadout => array(declared) = "declared";

[@mel.send]
external promptGuidelines: (loadout, string) => array(string) =
  "getPromptGuidelines";
