open Jest;
open Expect;

let empty = Js.Json.object_(Js.Dict.empty());

let doc = (~namespaceText="", ~parameters=empty, name, description) =>
  Search.document(~name, ~description, ~parameters, ~namespaceText);

let names = results => List.map(fst, results);

describe("Search.tokenize", () => {
  test("splits identifiers, lowercases and drops stop words", () =>
    expect(Search.tokenize("readFile of the HTTPServer, mcp__app"))
    |> toEqual(["read", "file", "http", "server", "mcp", "app"])
  );

  test("reduces plural forms", () =>
    expect(Search.tokenize("issues searches files class"))
    |> toEqual(["issue", "search", "file", "class"])
  );
});

describe("Search.document", () =>
  test("indexes the description, schema descriptions and property names", () => {
    let parameters =
      Js.Json.parseExn(
        {|{"type": "object", "properties": {"path": {"type": "string", "description": "File path"}, "lines": {"type": "array", "items": {"anyOf": [{"description": "Line number"}]}}}}|},
      );
    expect(doc(~parameters, "read", "Read a file").text)
    |> toEqual("Read a file path File path lines Line number");
  })
);

describe("Search.rank", () => {
  let documents = [
    doc("read", "Read the contents of a file"),
    doc("edit", "Edit a file by replacing text"),
    doc("jj", "Run a version control command"),
    doc(
      ~namespaceText="mcp__palace Memory palace",
      "mcp__palace__search",
      "Search stored memories",
    ),
  ];

  test("ranks a name match first", () =>
    expect(names(Search.rank("edit file", documents, ~limit=8)))
    |> toEqual(["edit", "read"])
  );

  test("prefers tools covering more query terms", () =>
    expect(names(Search.rank("version command file", documents, ~limit=8)))
    |> toEqual(["jj", "read", "edit"])
  );

  test("ranks an exact name query first", () =>
    expect(names(Search.rank("search", documents, ~limit=1)))
    |> toEqual(["mcp__palace__search"])
  );

  test("finds tools through their namespace", () =>
    expect(names(Search.rank("palace", documents, ~limit=8)))
    |> toEqual(["mcp__palace__search"])
  );

  test("respects the limit", () =>
    expect(List.length(Search.rank("file", documents, ~limit=1)))
    |> toBe(1)
  );

  test("returns nothing for a query without terms", () =>
    expect(Search.rank("the of", documents, ~limit=8)) |> toEqual([])
  );
});
