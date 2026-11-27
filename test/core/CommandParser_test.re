open Jest;
open Expect;

describe("CommandParser", () => {
  testAll(
    "reads a preset name",
    [
      (" code ", Ok("code")),
      ("", Error("Usage: /tool-preset <name> (available: code, docs)")),
      ("code docs", Error("Usage: /tool-preset <name>")),
    ],
    ((text, expected)) =>
    expect(CommandParser.presetName(~available=["code", "docs"], text))
    |> toEqual(expected)
  );

  test("reads adjustments separated by whitespace", () =>
    expect(CommandParser.adjustments(" +bash\t-edit "))
    |> toEqual(Ok([Selection.Add("bash"), Selection.Remove("edit")]))
  );

  test("requires at least one adjustment", () =>
    expect(CommandParser.adjustments(" "))
    |> toEqual(Error("Usage: /tool +name -name ..."))
  );
});
