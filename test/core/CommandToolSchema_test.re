open Jest;
open Expect;

describe("CommandTool.Schema.make", () =>
  test("describes parameters with their constraints and hints", () =>
    expect(Js.Json.stringify(CommandTool.Schema.make(Specs.jj)))
    |> toEqual(
         {|{"type":"object","properties":{"args":{"type":"array","items":{"type":"string","pattern":"^[^;]*$"},"description":"Not allowed: --config, -R."},"limit":{"type":"integer"},"message":{"type":"string","description":"Message Required when subcommand is describe."},"subcommand":{"type":"string","enum":["status","describe"]},"verbose":{"type":"boolean"}},"required":["subcommand"],"additionalProperties":false}|},
       )
  )
);
