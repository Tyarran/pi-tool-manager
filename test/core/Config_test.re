open Jest;
open Expect;

let decode = text => Config.decode(Some(Helpers.json(text)));

describe("Config.decode", () => {
  test("decodes a full configuration", () => {
    let config =
      decode(
        {|{
          "baseTools": ["read", "edit"],
          "defaultPreset": "read-only",
          "presets": {"read-only": ["-edit"], "only-ls": ["ls"]},
          "tools": {"list": {"description": "List", "command": ["ls"]}}
        }|},
      );
    expect(
      config
      |> Result.map(config =>
           switch (config) {
           | Some(config: Config.t) => (
               Names.Set.elements(config.baseTools),
               config.defaultPreset,
               Config.presetNames(config),
               List.map(
                 (tool: CommandTool.Spec.t) => tool.name,
                 config.tools,
               ),
             )
           | None => ([], None, [], [])
           }
         ),
    )
    |> toEqual(
         Ok((
           ["edit", "read"],
           Some("read-only"),
           ["only-ls", "read-only"],
           ["list"],
         )),
       );
  });

  test("treats missing settings as not configured", () =>
    expect(Config.decode(None)) |> toEqual(Ok(None))
  );

  testAll(
    "reports invalid configurations",
    [
      ({|"read"|}, "pi-tool-manager must be an object"),
      ({|{}|}, "pi-tool-manager.baseTools is required"),
      (
        {|{"baseTools": "read"}|},
        "pi-tool-manager.baseTools must be an array",
      ),
      (
        {|{"baseTools": ["+read"]}|},
        "pi-tool-manager.baseTools must list plain tool names, got \"+read\"",
      ),
      (
        {|{"baseTools": [], "presets": {"code": ["-"]}}|},
        "pi-tool-manager.presets.code contains an empty tool name",
      ),
      (
        {|{"baseTools": [], "defaultPreset": "code"}|},
        "pi-tool-manager.defaultPreset names an unknown preset \"code\"",
      ),
      (
        {|{"baseTools": [], "tools": {"jj": {"description": "jj", "command": ["jj", "{rev}"]}}}|},
        "pi-tool-manager.tools.jj uses an undeclared parameter \"rev\"",
      ),
    ],
    ((text, expected)) =>
    expect(decode(text) |> Helpers.message) |> toEqual(expected)
  );
});
