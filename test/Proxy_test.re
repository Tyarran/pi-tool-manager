open Jest;
open Expect;

describe("Proxy.hiddenDeclarations", () =>
  test("hides every declared tool except the proxy", () =>
    expect(Proxy.hiddenDeclarations(["read", "tool_call", "edit"]))
    |> toEqual(["read", "edit"])
  )
);

describe("Proxy.describe", () => {
  test("lists tools with the first line of their description", () =>
    expect(
      Proxy.describe([
        ("read", "  Read a file.\nMore details."),
        ("edit", ""),
      ]),
    )
    |> toContainString("Available tools:\n- read: Read a file.\n- edit")
  );

  test("reports an empty selection", () =>
    expect(Proxy.describe([]))
    |> toContainString("No tool is available in the current selection.")
  );

  test("appends the guidelines of the hidden tools", () =>
    expect(
      Proxy.describe(
        ~guidelines=["Use read instead of cat."],
        [("read", "Read a file.")],
      ),
    )
    |> toContainString("\n\nGuidelines:\n- Use read instead of cat.")
  );
});

describe("Proxy.validate", () => {
  let available = ["read", "edit"];

  test("accepts an available tool with an intent", () =>
    expect(Proxy.validate(~available, ~tool="read", ~intent="Inspect it"))
    |> toEqual(Ok())
  );

  test("requires an intent", () =>
    expect(Proxy.validate(~available, ~tool="read", ~intent="  "))
    |> toEqual(Error("intent must explain why the tool is called"))
  );

  test("rejects a tool outside the selection", () =>
    expect(Proxy.validate(~available, ~tool="bash", ~intent="Run it"))
    |> toEqual(
         Error("Tool \"bash\" is not available. Available tools: read, edit"),
       )
  );
});

describe("Proxy.describeFound", () => {
  test("reports no match", () =>
    expect(Proxy.describeFound([])) |> toEqual("No matching tools found.")
  );

  test("returns the description and schema of each match", () =>
    expect(
      Proxy.describeFound([
        ("read", " Read a file. ", "{}"),
        ("edit", "Edit a file.", "{\"type\": \"object\"}"),
      ]),
    )
    |> toEqual(
         "## read\n\nRead a file.\n\nParameters:\n```json\n{}\n```\n\n## edit\n\nEdit a file.\n\nParameters:\n```json\n{\"type\": \"object\"}\n```",
       )
  );
});
