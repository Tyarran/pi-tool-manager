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

type exposure;

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

let define:
  (
    ~name: string,
    ~label: string,
    ~description: string,
    ~promptSnippet: string=?,
    ~promptGuidelines: array(string)=?,
    ~parameters: Js.Json.t,
    ~exposure: exposure=?,
    ~defaultActive: bool=?,
    ~reachesTools: Js.Dict.t(Js.Json.t) => reach=?,
    ~prepareLoadout: loadout => loadoutChanges=?,
    ~renderCall: (Js.Json.t, PiTui.theme, Js.Json.t) => PiTui.component=?,
    execute('params)
  ) =>
  definition('params);

let text: string => content;

let textOf: content => option(string);

let success: string => result;

let failure: string => result;

let withContent: (result, ~content: array(content), ~isError: bool) => result;

let executeTool:
  (context, string, Js.Json.t, executeOptions) => Js.Promise.t(outcome);

let modelOnly: exposure;

let allTools: reach;

let someTools: array(string) => reach;

let declared: loadout => array(declared);

let promptGuidelines: (loadout, string) => array(string);
