open Jest;
open Expect;

let config =
  switch (
    Config.decode(
      Some(
        Helpers.json(
          {|{
            "baseTools": ["read", "edit", "bash"],
            "defaultPreset": "no-bash",
            "presets": {"no-bash": ["-bash"], "read-only": ["-edit", "-bash"], "ghost": ["+ghost"]}
          }|},
        ),
      ),
    )
  ) {
  | Ok(Some(config)) => config
  | Ok(None)
  | Error(_) => failwith("invalid test configuration")
  };

let known = Helpers.names(["read", "edit", "bash", "grep"]);

let session = (preset, adjustments): Session.t => {
  preset,
  adjustments,
};

let selection = session =>
  Names.Set.elements(Session.selection(config, session));

describe("Session", () => {
  test("starts on the default preset", () =>
    expect(Session.start(config)) |> toEqual(session(Some("no-bash"), []))
  );

  testAll(
    "selects tools from the preset and the adjustments",
    [
      (session(None, []), ["bash", "edit", "read"]),
      (session(Some("read-only"), [Add("grep")]), ["grep", "read"]),
      (
        session(Some("no-bash"), [Add("bash"), Remove("read")]),
        ["bash", "edit"],
      ),
    ],
    ((session, expected)) =>
    expect(selection(session)) |> toEqual(expected)
  );

  test("activates the proxy tools with the selection", () =>
    expect(
      Names.Set.elements(
        Session.active(config, session(Some("read-only"), [])),
      ),
    )
    |> toEqual(["read", "tool_call", "tool_find"])
  );

  testAll(
    "allows only active tools",
    [("edit", true), ("tool_call", true), ("bash", false)],
    ((name, expected)) =>
    expect(Session.allows(config, session(Some("no-bash"), []), name))
    |> toBe(expected)
  );

  testAll(
    "detects when the active tools drift",
    [
      (["read", "tool_call"], ["read", "tool_call"], false),
      (["read", "edit", "tool_call"], ["read", "tool_call"], true),
      (["read", "bash"], ["read", "bash"], true),
    ],
    ((known, active, expected)) =>
    expect(
      Session.needsSync(
        config,
        session(Some("no-bash"), []),
        ~known=Helpers.names(known),
        ~active=Helpers.names(active),
      ),
    )
    |> toBe(expected)
  );

  test("switches preset and resets adjustments", () =>
    expect(Session.switchPreset(config, ~known, "read-only"))
    |> toEqual(Ok(session(Some("read-only"), [])))
  );

  testAll(
    "rejects invalid preset switches",
    [
      (
        "code",
        "Unknown preset \"code\" (available: ghost, no-bash, read-only)",
      ),
      ("ghost", "Unknown tools: ghost"),
    ],
    ((name, expected)) =>
    expect(Session.switchPreset(config, ~known, name) |> Helpers.message)
    |> toEqual(expected)
  );

  test("appends adjustments", () =>
    expect(
      Session.adjust(
        ~known,
        session(None, [Add("grep")]),
        [Remove("edit")],
      ),
    )
    |> toEqual(Ok(session(None, [Add("grep"), Remove("edit")])))
  );

  test("rejects adjustments of unknown tools", () =>
    expect(
      Session.adjust(~known, session(None, []), [Add("ls")])
      |> Helpers.message,
    )
    |> toEqual("Unknown tools: ls")
  );
});
