let toolCall: string;

let toolFind: string;

let tools: Names.Set.t;

let callParameters: Js.Json.t;

let findParameters: Js.Json.t;

let defaultFindLimit: int;

let checkCall:
  (~selection: Names.Set.t, ~tool: string, ~intent: string) =>
  result(unit, string);
