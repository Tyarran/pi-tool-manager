open Jest;
open Expect;

describe("Names", () => {
  test("lists the keys of a map", () =>
    expect(
      Names.Set.elements(
        Names.keys(Names.Map.of_seq(List.to_seq([("b", 1), ("a", 2)]))),
      ),
    )
    |> toEqual(["a", "b"])
  );

  test("joins names in order", () =>
    expect(Names.join(Helpers.names(["read", "edit"])))
    |> toEqual("edit, read")
  );
});
