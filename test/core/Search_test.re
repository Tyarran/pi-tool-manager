open Jest;
open Expect;

let document = (~namespace=[], ~parameters="{}", name, description) =>
  Search.document(
    ~name,
    ~description,
    ~parameters=Helpers.json(parameters),
    ~namespace,
  );

let documents = [
  document("read", "Read the contents of a file"),
  document("edit", "Edit a file by replacing text"),
  document("jj", "Run a version control command"),
  document(
    ~namespace=["mcp__palace", "Memory palace"],
    "mcp__palace__search",
    "Search stored memories",
  ),
  document(
    ~parameters=
      {|{"properties": {"revision": {"description": "Commit identifier"}}}|},
    "show",
    "Show something",
  ),
];

describe("Search.rank", () =>
  testAll(
    "ranks the tools matching a query",
    [
      ("edit file", 8, ["edit", "read"]),
      ("version command file", 8, ["jj", "read", "edit"]),
      ("search", 1, ["mcp__palace__search"]),
      ("palace", 8, ["mcp__palace__search"]),
      ("commit revision", 8, ["show"]),
      ("files", 1, ["read"]),
      ("the of", 8, []),
    ],
    ((query, limit, expected)) =>
    expect(Search.rank(query, documents, ~limit)) |> toEqual(expected)
  )
);
