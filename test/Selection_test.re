open Jest;
open Expect;

let base = ["read", "edit", "bash"];

describe("Selection.resolve", () => {
  test("keeps the base without entries", () =>
    expect(Selection.resolve(~base, [])) |> toEqual(base)
  );

  test("removes and adds tools on top of the base", () =>
    expect(Selection.resolve(~base, ["-bash", "+grep"]))
    |> toEqual(["read", "edit", "grep"])
  );

  test("replaces the base with plain names, then applies modifiers", () =>
    expect(Selection.resolve(~base, ["ls", "+read", "-ls"]))
    |> toEqual(["read"])
  );

  test("applies modifiers in order", () =>
    expect(Selection.resolve(~base, ["-read", "+read"]))
    |> toEqual(["edit", "bash", "read"])
  );

  test("does not duplicate an added tool", () =>
    expect(Selection.resolve(~base, ["+read"]))
    |> toEqual(["edit", "bash", "read"])
  );
});

describe("Selection.unknownTools", () =>
  test("lists the unknown names of every entry once", () =>
    expect(
      Selection.unknownTools(
        ~known=["read", "edit"],
        ["read", "+grep", "-grep", "-bash"],
      ),
    )
    |> toEqual(["bash", "grep"])
  )
);

describe("Selection.parseAdjustments", () => {
  test("parses modifiers separated by whitespace", () =>
    expect(Selection.parseAdjustments("  +bash\t-edit "))
    |> toEqual(Ok(["+bash", "-edit"]))
  );

  test("requires at least one modifier", () =>
    expect(Selection.parseAdjustments("  "))
    |> toEqual(Error("Usage: /tool +name -name ..."))
  );

  testAll(
    "rejects entries that are not modifiers",
    ["bash", "+bash edit", "+", "-"],
    text =>
    expect(Result.is_error(Selection.parseAdjustments(text))) |> toBe(true)
  );
});
