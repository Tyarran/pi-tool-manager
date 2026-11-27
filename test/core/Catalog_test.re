open Jest;
open Expect;

describe("Catalog", () => {
  test("lists tools with the first line of their description", () =>
    expect(
      Catalog.callDescription(
        ~guidelines=[],
        [
          {
            name: "read",
            description: " Read a file.\nMore.",
          },
          {
            name: "edit",
            description: "",
          },
        ],
      ),
    )
    |> toContainString("Available tools:\n- read: Read a file.\n- edit")
  );

  test("reports an empty selection", () =>
    expect(Catalog.callDescription(~guidelines=[], []))
    |> toContainString("No tool is available in the current selection.")
  );

  test("appends the guidelines of the hidden tools", () =>
    expect(Catalog.callDescription(~guidelines=["Use read."], []))
    |> toContainString("\n\nGuidelines:\n- Use read.")
  );

  test("describes found tools with their schema", () =>
    expect(
      Catalog.foundTools([
        {
          name: "read",
          description: "Read a file. ",
          schema: Helpers.json("{}"),
        },
      ]),
    )
    |> toEqual("## read\n\nRead a file.\n\nParameters:\n```json\n{}\n```")
  );

  test("reports when nothing is found", () =>
    expect(Catalog.foundTools([])) |> toEqual("No matching tools found.")
  );
});
