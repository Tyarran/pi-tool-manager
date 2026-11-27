let callTool = "tool_call";

let findTool = "tool_find";

let names = [callTool, findTool];

let firstLine = text =>
  switch (String.split_on_char('\n', String.trim(text))) {
  | [line, ..._] => String.trim(line)
  | [] => ""
  };

let hiddenDeclarations = declared =>
  List.filter(name => !List.mem(name, names), declared);

let describe = (~guidelines=[], tools) => {
  let catalog =
    switch (tools) {
    | [] => "No tool is available in the current selection."
    | tools =>
      tools
      |> List.map(((name, description)) =>
           switch (firstLine(description)) {
           | "" => "- " ++ name
           | line => Printf.sprintf("- %s: %s", name, line)
           }
         )
      |> String.concat("\n")
    };
  "Call one of the available tools. Every tool is reached through tool_call: pass its name in `tool`, its parameters in `arguments`, and explain in `intent` why you call it. Use tool_find to get the parameters of a tool. Invalid arguments return the expected parameter schema.\n\nAvailable tools:\n"
  ++ catalog
  ++ (
    switch (guidelines) {
    | [] => ""
    | guidelines =>
      "\n\nGuidelines:\n"
      ++ String.concat("\n", List.map(line => "- " ++ line, guidelines))
    }
  );
};

let unavailable = (~available, tool) =>
  Printf.sprintf(
    "Tool \"%s\" is not available. Available tools: %s",
    tool,
    String.concat(", ", available),
  );

let validate = (~available, ~tool, ~intent) =>
  if (String.trim(intent) == "") {
    Error("intent must explain why the tool is called");
  } else if (!List.mem(tool, available)) {
    Error(unavailable(~available, tool));
  } else {
    Ok();
  };

let describeFound = found =>
  switch (found) {
  | [] => "No matching tools found."
  | found =>
    found
    |> List.map(((name, description, schema)) =>
         Printf.sprintf(
           "## %s\n\n%s\n\nParameters:\n```json\n%s\n```",
           name,
           String.trim(description),
           schema,
         )
       )
    |> String.concat("\n\n")
  };
