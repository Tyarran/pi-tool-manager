open Jest;
open Expect;

let decode = text => Config.decode(Some(Js.Json.parseExn(text)));

let message = result =>
  switch (result) {
  | Ok(_) => "ok"
  | Error(message) => message
  };

describe("Config.decode", () => {
  test("decodes a full configuration", () =>
    expect(
      decode(
        {|{"baseTools": ["read", "edit"], "defaultPreset": "read-only", "presets": {"read-only": ["-edit"], "only-ls": ["ls", "+read"]}}|},
      ),
    )
    |> toEqual(
         Ok(
           Some(
             {
               baseTools: ["read", "edit"],
               defaultPreset: Some("read-only"),
               presets: [
                 ("read-only", ["-edit"]),
                 ("only-ls", ["ls", "+read"]),
               ],
               tools: [],
             }: Config.t,
           ),
         ),
       )
  );

  test("treats a missing configuration as not configured", () =>
    expect(Config.decode(None)) |> toEqual(Ok(None))
  );

  test("defaults to no preset", () =>
    expect(decode({|{"baseTools": []}|}))
    |> toEqual(
         Ok(
           Some(
             {
               baseTools: [],
               defaultPreset: None,
               presets: [],
               tools: [],
             }: Config.t,
           ),
         ),
       )
  );

  test("reports an invalid tool definition", () =>
    expect(
      message(
        decode(
          {|{"baseTools": [], "tools": {"jj": {"description": "Run jj", "command": ["jj", "{rev}"]}}}|},
        ),
      ),
    )
    |> toEqual(
         "pi-tool-manager.tools.jj uses an undeclared parameter \"rev\"",
       )
  );

  test("reports a missing base tool list", () =>
    expect(message(decode({|{"presets": {}}|})))
    |> toEqual("pi-tool-manager.baseTools is required")
  );

  test("reports an unknown default preset", () =>
    expect(message(decode({|{"baseTools": [], "defaultPreset": "code"}|})))
    |> toEqual(
         "pi-tool-manager.defaultPreset names an unknown preset \"code\"",
       )
  );

  test("reports modifiers in the base tool list", () =>
    expect(message(decode({|{"baseTools": ["+read"]}|})))
    |> toEqual(
         "pi-tool-manager.baseTools must list plain tool names, got \"+read\"",
       )
  );

  test("reports the faulty preset", () =>
    expect(
      message(decode({|{"baseTools": [], "presets": {"code": ["-"]}}|})),
    )
    |> toEqual("pi-tool-manager.presets.code contains an empty tool name")
  );

  testAll(
    "rejects invalid configurations",
    [
      {|"read"|},
      {|{"baseTools": "read"}|},
      {|{"baseTools": [1]}|},
      {|{"baseTools": [""]}|},
      {|{"baseTools": [], "presets": []}|},
      {|{"baseTools": [], "presets": {"code": "read"}}|},
      {|{"baseTools": [], "presets": {"code": [true]}}|},
      {|{"baseTools": [], "presets": {}, "defaultPreset": 1}|},
    ],
    text =>
    expect(Result.is_error(decode(text))) |> toBe(true)
  );
});
