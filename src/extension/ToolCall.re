type params = {
  tool: string,
  intent: string,
  arguments: option(Js.Json.t),
};

type renderArgs = {
  tool: option(string),
  intent: option(string),
  arguments: option(Js.Json.t),
};

let parameters =
  TypeBox.object_(
    Js.Dict.fromList([
      (
        "tool",
        TypeBox.string_({
          "description": "Name of the tool to call, from the list of available tools",
        }),
      ),
      (
        "intent",
        TypeBox.string_({
          "description": "Why you call this tool, in one short sentence",
          "minLength": 1,
        }),
      ),
      (
        "arguments",
        TypeBox.optional(
          TypeBox.object_(
            Js.Dict.empty(),
            {
              "description": "Parameters of the called tool",
              "additionalProperties": true,
            },
          ),
        ),
      ),
    ]),
    { "additionalProperties": false },
  );

let schemaOf = (pi, name) =>
  Pi.getAllTools(pi)
  |> Js.Array.find(~f=(tool: Pi.toolInfo) => tool.name == name)
  |> Option.map((tool: Pi.toolInfo) =>
       Js.Json.stringifyWithSpace(tool.parameters, 2)
     );

let withSchema = (pi, name, result) =>
  switch (schemaOf(pi, name)) {
  | None => ()
  | Some(schema) =>
    Pi.setResultContent(
      result,
      Js.Array.concat(
        ~other=[|
          ToolResult.text(
            "Expected parameters of " ++ name ++ ":\n" ++ schema,
          ),
        |],
        Pi.resultContent(result),
      ),
    )
  };

let execute = (pi, ~available, params: params, signal, onUpdate, ctx) =>
  switch (
    Proxy.validate(
      ~available=available(),
      ~tool=params.tool,
      ~intent=params.intent,
    )
  ) {
  | Error(message) => Js.Promise.resolve(ToolResult.failure(message))
  | Ok () =>
    Pi.executeTool(
      ctx,
      params.tool,
      Option.value(
        params.arguments,
        ~default=Js.Json.object_(Js.Dict.empty()),
      ),
      {
        signal,
        onUpdate,
      },
    )
    |> Js.Promise.then_((outcome: Pi.toolOutcome) => {
         let result = Pi.copyResult(Js.Obj.empty(), outcome.result);
         Pi.setResultIsError(result, outcome.isError);
         if (outcome.isError && Option.is_none(outcome.durationMs)) {
           withSchema(pi, params.tool, result);
         };
         Js.Promise.resolve(result);
       })
  };

let maxArgumentsLength = 300;

let summarize = json => {
  let value = Js.Json.stringify(json);
  String.length(value) > maxArgumentsLength
    ? String.sub(value, 0, maxArgumentsLength) ++ "…" : value;
};

let renderCall = (args: renderArgs, theme) => {
  let header =
    Pi.fg(theme, "toolTitle", Pi.bold(theme, Proxy.callTool))
    ++ " "
    ++ Pi.fg(theme, "accent", Option.value(args.tool, ~default="…"));
  let lines =
    [
      Some(header),
      Option.map(intent => Pi.fg(theme, "muted", intent), args.intent),
      Option.map(
        json => Pi.fg(theme, "dim", summarize(json)),
        args.arguments,
      ),
    ]
    |> List.filter_map(line => line);
  Pi.text(String.concat("\n", lines), 0, 0);
};

let prepareLoadout = (loadout: Pi.loadout) => {
  let declared = Array.to_list(loadout.declared);
  let hidden =
    Proxy.hiddenDeclarations(
      List.map((tool: Pi.agentTool) => tool.name, declared),
    );
  let tools =
    declared
    |> List.filter((tool: Pi.agentTool) => List.mem(tool.name, hidden))
    |> List.map((tool: Pi.agentTool) => (tool.name, tool.description));
  let guidelines =
    List.concat_map(
      name => Array.to_list(Pi.getPromptGuidelines(loadout, name)),
      hidden,
    );
  {
    Pi.descriptions:
      Js.Dict.fromList([
        (Proxy.callTool, Proxy.describe(~guidelines, tools)),
      ]),
    hiddenDeclarations: Array.of_list(hidden),
  };
};

let reachesTools = (input: Js.Dict.t(Js.Json.t)) =>
  switch (Option.map(Js.Json.classify, Js.Dict.get(input, "tool"))) {
  | Some(JSONString(tool)) => [|tool|]
  | _ => [||]
  };

let register = (pi, ~available) =>
  Pi.registerTool(
    pi,
    {
      "name": Proxy.callTool,
      "label": "Tool Call",
      "description": Proxy.describe([]),
      "promptSnippet": "Call any available tool, with the intent behind the call",
      "promptGuidelines": [|
        "Use tool_call to call every tool: pass its name in tool, its parameters in arguments, and a short intent explaining why you call it.",
      |],
      "parameters": parameters,
      "exposure": "model-only",
      "defaultActive": false,
      "reachesTools": reachesTools,
      "prepareLoadout": prepareLoadout,
      "renderCall": (args, theme, _context) => renderCall(args, theme),
      "execute": (_toolCallId, params, signal, onUpdate, ctx) =>
        execute(pi, ~available, params, signal, onUpdate, ctx),
    },
  );
