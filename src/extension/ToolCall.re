type params = {
  tool: string,
  intent: string,
  arguments: option(Js.Json.t),
};

let maxSummaryLength = 300;

let emptyArguments = Js.Json.object_(Js.Dict.empty());

let parametersOf = (pi, name) =>
  Pi.allTools(pi)
  |> Array.to_list
  |> List.find_opt((tool: Pi.toolInfo) => tool.name == name)
  |> Option.map((tool: Pi.toolInfo) => tool.parameters);

let withExpectedParameters = (pi, name, result: PiTool.result) =>
  switch (parametersOf(pi, name)) {
  | Some(schema) =>
    PiTool.withContent(
      result,
      ~content=
        Array.append(
          result.content,
          [|PiTool.text(Catalog.expectedParameters(~name, schema))|],
        ),
      ~isError=true,
    )
  | None => result
  };

let resultOf = (pi, name, outcome: PiTool.outcome) => {
  let result =
    PiTool.withContent(
      outcome.result,
      ~content=outcome.result.content,
      ~isError=outcome.isError,
    );
  let rejectedBeforeRunning =
    outcome.isError && Option.is_none(outcome.durationMs);
  rejectedBeforeRunning ? withExpectedParameters(pi, name, result) : result;
};

let execute = (pi, ~selection, params, signal, onUpdate, context) =>
  switch (
    Proxy.checkCall(
      ~selection=selection(),
      ~tool=params.tool,
      ~intent=params.intent,
    )
  ) {
  | Error(message) => Js.Promise.resolve(PiTool.failure(message))
  | Ok () =>
    PiTool.executeTool(
      context,
      params.tool,
      Option.value(params.arguments, ~default=emptyArguments),
      {
        signal,
        onUpdate,
      },
    )
    |> Js.Promise.then_(outcome =>
         Js.Promise.resolve(resultOf(pi, params.tool, outcome))
       )
  };

let summarize = json => {
  let text = Js.Json.stringify(json);
  String.length(text) > maxSummaryLength
    ? String.sub(text, 0, maxSummaryLength) ++ "…" : text;
};

let renderCall = (args, theme, _context) => {
  let field = key =>
    Option.bind(Js.Json.decodeObject(args), fields =>
      Js.Dict.get(fields, key)
    );
  let text = key => Option.bind(field(key), Js.Json.decodeString);
  let header =
    PiTui.fg(theme, `toolTitle, PiTui.bold(theme, Proxy.toolCall))
    ++ " "
    ++ PiTui.fg(theme, `accent, Option.value(text("tool"), ~default="…"));
  [
    Some(header),
    Option.map(PiTui.fg(theme, `muted), text("intent")),
    Option.map(
      json => PiTui.fg(theme, `dim, summarize(json)),
      field("arguments"),
    ),
  ]
  |> List.filter_map(Fun.id)
  |> String.concat("\n")
  |> PiTui.text;
};

let prepareLoadout = loadout => {
  let hidden =
    PiTool.declared(loadout)
    |> Array.to_list
    |> List.filter((tool: PiTool.declared) =>
         !Names.Set.mem(tool.name, Proxy.tools)
       );
  let guidelines =
    List.concat_map(
      (tool: PiTool.declared) =>
        Array.to_list(PiTool.promptGuidelines(loadout, tool.name)),
      hidden,
    );
  let tools =
    List.map(
      (tool: PiTool.declared) =>
        {
          Catalog.name: tool.name,
          description: tool.description,
        },
      hidden,
    );
  {
    PiTool.descriptions:
      Js.Dict.fromList([
        (Proxy.toolCall, Catalog.callDescription(~guidelines, tools)),
      ]),
    hiddenDeclarations:
      Array.of_list(List.map((tool: PiTool.declared) => tool.name, hidden)),
  };
};

let reachesTools = input =>
  switch (Option.bind(Js.Dict.get(input, "tool"), Js.Json.decodeString)) {
  | Some(tool) => PiTool.someTools([|tool|])
  | None => PiTool.someTools([||])
  };

let register = (pi, ~selection) =>
  Pi.registerTool(
    pi,
    PiTool.define(
      ~name=Proxy.toolCall,
      ~label="Tool Call",
      ~description=Catalog.callDescription(~guidelines=[], []),
      ~promptSnippet=
        "Call any available tool, with the intent behind the call",
      ~promptGuidelines=[|
        "Use tool_call to call every tool: pass its name in tool, its parameters in arguments, and a short intent explaining why you call it.",
      |],
      ~parameters=Proxy.callParameters,
      ~exposure=PiTool.modelOnly,
      ~defaultActive=false,
      ~reachesTools,
      ~prepareLoadout,
      ~renderCall,
      (_id, params, signal, onUpdate, context) =>
      execute(pi, ~selection, params, signal, onUpdate, context)
    ),
  );
