open Jest;
open Expect;

let base = Helpers.names(["read", "edit", "bash"]);

let resolve = texts =>
  Selection.parsePreset(texts)
  |> Result.map(preset =>
       Names.Set.elements(Selection.resolve(~base, preset))
     );

describe("Selection.resolve", () =>
  testAll(
    "applies presets to the base",
    [
      ([], ["bash", "edit", "read"]),
      (["-bash", "+grep"], ["edit", "grep", "read"]),
      (["ls", "+read", "-ls"], ["read"]),
      (["-read", "+read"], ["bash", "edit", "read"]),
    ],
    ((texts, expected)) =>
    expect(resolve(texts)) |> toEqual(Ok(expected))
  )
);

describe("Selection.parsePreset", () =>
  test("rejects empty names", () =>
    expect(Selection.parsePreset(["read", "-"]) |> Helpers.message)
    |> toEqual("contains an empty tool name")
  )
);

describe("Selection.parseNames", () => {
  test("accepts plain names", () =>
    expect(
      Selection.parseNames(["read", "edit"])
      |> Result.map(Names.Set.elements),
    )
    |> toEqual(Ok(["edit", "read"]))
  );

  test("rejects modifiers", () =>
    expect(Selection.parseNames(["+read"]) |> Helpers.message)
    |> toEqual("must list plain tool names, got \"+read\"")
  );
});

describe("Selection.parseModifiers", () => {
  test("parses additions and removals", () =>
    expect(Selection.parseModifiers(["+bash", "-edit"]))
    |> toEqual(Ok([Selection.Add("bash"), Selection.Remove("edit")]))
  );

  testAll("rejects other entries", ["bash", "+", "-"], text =>
    expect(Selection.parseModifiers([text]) |> Helpers.message)
    |> toEqual(
         Printf.sprintf(
           "Invalid entry \"%s\": use +name to add a tool and -name to remove one",
           text,
         ),
       )
  );
});

describe("Selection.referencedNames", () =>
  test("lists replacements and modified names", () =>
    expect(
      Selection.parsePreset(["ls", "+read", "-bash"])
      |> Result.map(preset =>
           Names.Set.elements(Selection.referencedNames(preset))
         ),
    )
    |> toEqual(Ok(["bash", "ls", "read"]))
  )
);
