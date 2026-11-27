type params = {
  query: string,
  limit: option(int),
};

let defaultLimit = 8;

let parameters =
  TypeBox.object_(
    Js.Dict.fromList([
      (
        "query",
        TypeBox.string_({
          "description": "Keywords describing the tool you need",
        }),
      ),
      (
        "limit",
        TypeBox.optional(
          TypeBox.integer({
            "description": "Maximum number of tools to return. Defaults to 8.",
            "minimum": 1,
          }),
        ),
      ),
    ]),
    { "additionalProperties": false },
  );

let namespaceText = (tool: Pi.toolInfo) =>
  switch (tool.namespace) {
  | None => ""
  | Some(namespace) =>
    [
      namespace.name,
      Option.value(namespace.description, ~default=""),
      Option.value(namespace.instructions, ~default=""),
    ]
    |> String.concat(" ")
  };

let find = (pi, ~available, params) => {
  let available = available();
  let candidates =
    Pi.getAllTools(pi)
    |> Array.to_list
    |> List.filter((tool: Pi.toolInfo) => List.mem(tool.name, available));
  let documents =
    List.map(
      (tool: Pi.toolInfo) =>
        Search.document(
          ~name=tool.name,
          ~description=tool.description,
          ~parameters=tool.parameters,
          ~namespaceText=namespaceText(tool),
        ),
      candidates,
    );
  Search.rank(
    params.query,
    documents,
    ~limit=Option.value(params.limit, ~default=defaultLimit),
  )
  |> List.filter_map(((name, _score)) =>
       List.find_opt((tool: Pi.toolInfo) => tool.name == name, candidates)
     )
  |> List.map((tool: Pi.toolInfo) =>
       (
         tool.name,
         tool.description,
         Js.Json.stringifyWithSpace(tool.parameters, 2),
       )
     )
  |> Proxy.describeFound;
};

let execute = (pi, ~available, params) =>
  Js.Promise.resolve(
    String.trim(params.query) == ""
      ? ToolResult.failure("query must not be empty")
      : ToolResult.success(find(pi, ~available, params)),
  );

let register = (pi, ~available) =>
  Pi.registerTool(
    pi,
    {
      "name": Proxy.findTool,
      "label": "Tool Find",
      "description": "Search the tools available through tool_call by keywords. Returns the description and the parameter schema of each match; call them with tool_call.",
      "promptSnippet": "Find the tools available through tool_call and their parameters",
      "promptGuidelines": [|
        "Use tool_find to get the parameters of a tool before calling it with tool_call when you do not know them.",
      |],
      "parameters": parameters,
      "exposure": "model-only",
      "defaultActive": false,
      "reachesTools": _input => "all",
      "execute": (_toolCallId, params, _signal, _onUpdate, _ctx) =>
        execute(pi, ~available, params),
    },
  );
