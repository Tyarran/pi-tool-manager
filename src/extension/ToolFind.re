type params = {
  query: string,
  limit: option(int),
};

let namespaceParts = (namespace: option(Pi.namespace)) =>
  switch (namespace) {
  | Some(namespace) => [
      namespace.name,
      ...List.filter_map(
           Fun.id,
           [namespace.description, namespace.instructions],
         ),
    ]
  | None => []
  };

let documentOf = (tool: Pi.toolInfo) =>
  Search.document(
    ~name=tool.name,
    ~description=tool.description,
    ~parameters=tool.parameters,
    ~namespace=namespaceParts(tool.namespace),
  );

let foundOf = (tool: Pi.toolInfo) => {
  Catalog.name: tool.name,
  description: tool.description,
  schema: tool.parameters,
};

let find = (pi, ~selection, params) => {
  let selection = selection();
  let candidates =
    Pi.allTools(pi)
    |> Array.to_list
    |> List.filter((tool: Pi.toolInfo) =>
         Names.Set.mem(tool.name, selection)
       );
  Search.rank(
    params.query,
    List.map(documentOf, candidates),
    ~limit=Option.value(params.limit, ~default=Proxy.defaultFindLimit),
  )
  |> List.filter_map(name =>
       List.find_opt((tool: Pi.toolInfo) => tool.name == name, candidates)
     )
  |> List.map(foundOf)
  |> Catalog.foundTools;
};

let execute = (pi, ~selection, params) =>
  Js.Promise.resolve(
    String.trim(params.query) == ""
      ? PiTool.failure("query must not be empty")
      : PiTool.success(find(pi, ~selection, params)),
  );

let register = (pi, ~selection) =>
  Pi.registerTool(
    pi,
    PiTool.define(
      ~name=Proxy.toolFind,
      ~label="Tool Find",
      ~description=
        "Search the tools available through tool_call by keywords. Returns the description and the parameter schema of each match; call them with tool_call.",
      ~promptSnippet=
        "Find the tools available through tool_call and their parameters",
      ~promptGuidelines=[|
        "Use tool_find to get the parameters of a tool before calling it with tool_call when you do not know them.",
      |],
      ~parameters=Proxy.findParameters,
      ~exposure=PiTool.modelOnly,
      ~defaultActive=false,
      ~reachesTools=_input => PiTool.allTools,
      (_id, params, _signal, _onUpdate, _context) =>
        execute(pi, ~selection, params),
    ),
  );
