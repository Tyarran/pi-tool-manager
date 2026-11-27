type tool = PiTool.definition(Js.Json.t);

type handler;

type t = {
  settings: Js.Dict.t(Js.Json.t),
  tools: Hashtbl.t(string, tool),
  mutable active: Names.Set.t,
  handlers: Hashtbl.t(string, handler),
  commands: Hashtbl.t(string, Pi.command),
  mutable notifications: list((string, string)),
};

type api = {
  getSettings: unit => Js.Dict.t(Js.Json.t),
  getAllTools: unit => array(Pi.toolInfo),
  getActiveTools: unit => array(string),
  setActiveTools: array(string) => unit,
  on: (string, handler) => unit,
  registerCommand: (string, Pi.command) => unit,
  registerTool: tool => unit,
};

type ui = {notify: (string, string) => unit};

type toolContext = {
  cwd: string,
  executeTool:
    (string, Js.Json.t, PiTool.executeOptions) => Js.Promise.t(PiTool.outcome),
};

type theme = {
  fg: (string, string) => string,
  bold: string => string,
};

type loadout = {
  declared: array(PiTool.declared),
  getPromptGuidelines: string => array(string),
};

type renderedText = {text: string};

external toPi: api => Pi.api = "%identity";

external toUi: ui => Pi.ui = "%identity";

external toToolContext: toolContext => PiTool.context = "%identity";

external toTheme: theme => PiTui.theme = "%identity";

external toLoadout: loadout => PiTool.loadout = "%identity";

external renderedText: PiTui.component => renderedText = "%identity";

external toolCallEvent: Pi.toolCall => Js.Json.t = "%identity";

type eventHandler = (Js.Json.t, Pi.context) => Js.Undefined.t(Pi.block);

external asHandler: handler => eventHandler = "%identity";

let projectDirectory = "/tmp";

let builtin = (name, description) =>
  PiTool.define(
    ~name,
    ~label=name,
    ~description,
    ~parameters=
      Js.Json.parseExn(
        {|{"type": "object", "properties": {"path": {"type": "string"}}}|},
      ),
    (_id, _args, _signal, _onUpdate, _context) =>
    Js.Promise.resolve(PiTool.success("ran " ++ name))
  );

let builtins = [
  builtin("read", "Read a file"),
  builtin("edit", "Edit a file"),
  builtin("bash", "Run a shell command"),
  builtin("grep", "Search file contents"),
];

let create = settings => {
  let fake = {
    settings:
      switch (settings) {
      | Some(settings) =>
        Js.Dict.fromList([("pi-tool-manager", Js.Json.parseExn(settings))])
      | None => Js.Dict.empty()
      },
    tools: Hashtbl.create(16),
    active: Names.Set.empty,
    handlers: Hashtbl.create(8),
    commands: Hashtbl.create(4),
    notifications: [],
  };
  List.iter(
    (tool: tool) => Hashtbl.replace(fake.tools, tool.name, tool),
    builtins,
  );
  fake.active =
    Names.Set.of_list(List.map((tool: tool) => tool.name, builtins));
  fake;
};

let register = (fake, tool: tool) => {
  Hashtbl.replace(fake.tools, tool.name, tool);
  if (tool.defaultActive != Some(false)) {
    fake.active = Names.Set.add(tool.name, fake.active);
  };
};

let infoOf = (tool: tool) => {
  Pi.name: tool.name,
  description: tool.description,
  parameters: tool.parameters,
  namespace: None,
};

let api = fake =>
  toPi({
    getSettings: () => fake.settings,
    getAllTools: () =>
      Hashtbl.to_seq_values(fake.tools)
      |> List.of_seq
      |> List.map(infoOf)
      |> Array.of_list,
    getActiveTools: () => Array.of_list(Names.Set.elements(fake.active)),
    setActiveTools: names =>
      fake.active =
        names
        |> Array.to_list
        |> List.filter(Hashtbl.mem(fake.tools))
        |> Names.Set.of_list,
    on: (event, handler) => Hashtbl.add(fake.handlers, event, handler),
    registerCommand: (name, command) =>
      Hashtbl.replace(fake.commands, name, command),
    registerTool: register(fake),
  });

let context = fake => {
  Pi.cwd: projectDirectory,
  ui:
    toUi({
      notify: (message, level) =>
        fake.notifications = fake.notifications @ [(level, message)],
    }),
};

let emit = (fake, event, payload) =>
  Hashtbl.find_all(fake.handlers, event)
  |> List.rev
  |> List.filter_map(handler =>
       Js.Undefined.toOption(asHandler(handler, payload, context(fake)))
     );

let startSession = fake => ignore(emit(fake, "session_start", Js.Json.null));

let startTurn = fake => ignore(emit(fake, "turn_start", Js.Json.null));

let blockReason = (fake, toolName) =>
  switch (emit(fake, "tool_call", toolCallEvent({ toolName: toolName }))) {
  | [block, ..._] => Some(block.reason)
  | [] => None
  };

let runCommand = (fake, name, args) =>
  switch (Hashtbl.find_opt(fake.commands, name)) {
  | Some(command) => command.handler(args, context(fake))
  | None => failwith("unknown command " ++ name)
  };

let rejected = message => {
  PiTool.result: PiTool.failure(message),
  isError: true,
  durationMs: None,
};

let isInvalid = args =>
  switch (
    Option.bind(Js.Json.decodeObject(args), fields =>
      Js.Dict.get(fields, "invalid")
    )
  ) {
  | Some(_) => true
  | None => false
  };

let rec toolContext = fake =>
  toToolContext({
    cwd: projectDirectory,
    executeTool: (name, args, _options) => executeNested(fake, name, args),
  })
and executeNested = (fake, name, args) =>
  switch (blockReason(fake, name), Hashtbl.find_opt(fake.tools, name)) {
  | (Some(reason), _) => Js.Promise.resolve(rejected(reason))
  | (None, None) =>
    Js.Promise.resolve(rejected("Tool " ++ name ++ " not found"))
  | (None, Some(_)) when isInvalid(args) =>
    Js.Promise.resolve(rejected("Validation failed"))
  | (None, Some(tool)) =>
    tool.execute(
      "nested",
      args,
      Js.Nullable.null,
      Js.Nullable.null,
      toolContext(fake),
    )
    |> Js.Promise.then_((result: PiTool.result) =>
         Js.Promise.resolve({
           PiTool.result,
           isError: result.isError,
           durationMs: Some(1.0),
         })
       )
  };

let find = (fake, name) =>
  switch (Hashtbl.find_opt(fake.tools, name)) {
  | Some(tool) => tool
  | None => failwith("unknown tool " ++ name)
  };

let call = (fake, name, args) =>
  find(fake, name).execute(
    "call",
    Js.Json.parseExn(args),
    Js.Nullable.null,
    Js.Nullable.null,
    toolContext(fake),
  );

let active = fake => Names.Set.elements(fake.active);

let text = (result: PiTool.result) =>
  result.content
  |> Array.to_list
  |> List.filter_map(PiTool.textOf)
  |> String.concat("\n");

let theme =
  toTheme({
    fg: (color, text) => Printf.sprintf("<%s>%s", color, text),
    bold: text => "*" ++ text ++ "*",
  });

let loadout = (~guidelines, declared) =>
  toLoadout({
    declared: Array.of_list(declared),
    getPromptGuidelines: name =>
      Array.of_list(
        Option.value(List.assoc_opt(name, guidelines), ~default=[]),
      ),
  });
