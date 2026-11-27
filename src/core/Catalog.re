type tool = {
  name: string,
  description: string,
};

type found = {
  name: string,
  description: string,
  schema: Js.Json.t,
};

let callIntroduction = "Call one of the available tools. Every tool is reached through tool_call: pass its name in `tool`, its parameters in `arguments`, and explain in `intent` why you call it. Use tool_find to get the parameters of a tool. Invalid arguments return the expected parameter schema.";

let firstLine = text =>
  switch (String.split_on_char('\n', String.trim(text))) {
  | [line, ..._] => String.trim(line)
  | [] => ""
  };

let toolLine = (tool: tool) =>
  switch (firstLine(tool.description)) {
  | "" => "- " ++ tool.name
  | line => Printf.sprintf("- %s: %s", tool.name, line)
  };

let toolList = tools =>
  switch (tools) {
  | [] => "No tool is available in the current selection."
  | [_, ..._] => String.concat("\n", List.map(toolLine, tools))
  };

let guidelineSection = guidelines =>
  switch (guidelines) {
  | [] => []
  | [_, ..._] => [
      "Guidelines:\n"
      ++ String.concat("\n", List.map(line => "- " ++ line, guidelines)),
    ]
  };

let callDescription = (~guidelines, tools) =>
  [callIntroduction, "Available tools:\n" ++ toolList(tools)]
  @ guidelineSection(guidelines)
  |> String.concat("\n\n");

let schemaBlock = schema =>
  "```json\n" ++ Js.Json.stringifyWithSpace(schema, 2) ++ "\n```";

let foundTool = (tool: found) =>
  Printf.sprintf(
    "## %s\n\n%s\n\nParameters:\n%s",
    tool.name,
    String.trim(tool.description),
    schemaBlock(tool.schema),
  );

let foundTools = tools =>
  switch (tools) {
  | [] => "No matching tools found."
  | [_, ..._] => String.concat("\n\n", List.map(foundTool, tools))
  };

let expectedParameters = (~name, schema) =>
  Printf.sprintf(
    "Expected parameters of %s:\n%s",
    name,
    schemaBlock(schema),
  );

let blocked = name =>
  Printf.sprintf(
    "%s is not allowed by the active tool selection (%s)",
    name,
    Config.key,
  );
