let toolCall = "tool_call";

let toolFind = "tool_find";

let tools = Names.Set.of_list([toolCall, toolFind]);

let defaultFindLimit = 8;

let callParameters =
  Js.Json.parseExn(
    {|{
  "type": "object",
  "properties": {
    "tool": {
      "type": "string",
      "description": "Name of the tool to call, from the list of available tools"
    },
    "intent": {
      "type": "string",
      "minLength": 1,
      "description": "Why you call this tool, in one short sentence"
    },
    "arguments": {
      "type": "object",
      "additionalProperties": true,
      "description": "Parameters of the called tool"
    }
  },
  "required": ["tool", "intent"],
  "additionalProperties": false
}|},
  );

let findParameters =
  Js.Json.parseExn(
    {|{
  "type": "object",
  "properties": {
    "query": {
      "type": "string",
      "description": "Keywords describing the tool you need"
    },
    "limit": {
      "type": "integer",
      "minimum": 1,
      "description": "Maximum number of tools to return. Defaults to 8."
    }
  },
  "required": ["query"],
  "additionalProperties": false
}|},
  );

let checkCall = (~selection, ~tool, ~intent) =>
  if (String.trim(intent) == "") {
    Error("intent must explain why the tool is called");
  } else if (!Names.Set.mem(tool, selection)) {
    Error(
      Printf.sprintf(
        "Tool \"%s\" is not available. Available tools: %s",
        tool,
        Names.join(selection),
      ),
    );
  } else {
    Ok();
  };
