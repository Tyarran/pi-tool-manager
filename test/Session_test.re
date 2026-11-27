open Jest;
open Expect;

let config: Config.t = {
  baseTools: ["read", "edit", "bash"],
  defaultPreset: Some("no-bash"),
  presets: [("no-bash", ["-bash"]), ("read-only", ["-edit", "-bash"])],
  tools: [],
};

let known = ["read", "edit", "bash", "grep"];

let session = (preset, adjustments): Session.t => {
  preset,
  adjustments,
};

describe("Session.start", () =>
  test("starts on the default preset without adjustments", () =>
    expect(Session.start(config)) |> toEqual(session(Some("no-bash"), []))
  )
);

describe("Session.tools", () => {
  test("uses the base without preset", () =>
    expect(Session.tools(config, session(None, [])))
    |> toEqual(["read", "edit", "bash"])
  );

  test("applies the preset then the adjustments", () =>
    expect(Session.tools(config, session(Some("read-only"), ["+grep"])))
    |> toEqual(["read", "grep"])
  );
});

describe("Session.allows", () => {
  test("allows tools of the selection", () =>
    expect(Session.allows(config, session(Some("no-bash"), []), "edit"))
    |> toBe(true)
  );

  test("rejects tools outside the selection", () =>
    expect(Session.allows(config, session(Some("no-bash"), []), "bash"))
    |> toBe(false)
  );

  test("always allows the proxy", () =>
    expect(
      Session.allows(config, session(Some("no-bash"), []), "tool_call"),
    )
    |> toBe(true)
  );
});

describe("Session.active", () =>
  test("adds the proxy to the selection", () =>
    expect(Session.active(config, session(Some("read-only"), [])))
    |> toEqual(["read", "tool_call", "tool_find"])
  )
);

describe("Session.needsSync", () => {
  let noBash = session(Some("no-bash"), []);

  test("is in sync when the active tools match the known selection", () =>
    expect(
      Session.needsSync(
        config,
        noBash,
        ~known=["read", "bash", "tool_call"],
        ~active=["tool_call", "read"],
      ),
    )
    |> toBe(false)
  );

  test("needs a sync when a selected tool appears", () =>
    expect(
      Session.needsSync(
        config,
        noBash,
        ~known=["read", "edit", "bash"],
        ~active=["read"],
      ),
    )
    |> toBe(true)
  );

  test("needs a sync when a tool outside the selection is active", () =>
    expect(
      Session.needsSync(
        config,
        noBash,
        ~known=["read", "edit", "bash"],
        ~active=["edit", "read", "bash"],
      ),
    )
    |> toBe(true)
  );
});

describe("Session.switchPreset", () => {
  test("switches preset and resets adjustments", () =>
    expect(Session.switchPreset(config, ~known, " read-only "))
    |> toEqual(Ok(session(Some("read-only"), [])))
  );

  test("requires a preset name", () =>
    expect(Session.switchPreset(config, ~known, ""))
    |> toEqual(
         Error("Usage: /tool-preset <name> (available: no-bash, read-only)"),
       )
  );

  test("rejects an unknown preset", () =>
    expect(Session.switchPreset(config, ~known, "code"))
    |> toEqual(
         Error("Unknown preset \"code\" (available: no-bash, read-only)"),
       )
  );

  test("rejects a preset with unknown tools", () =>
    expect(Session.switchPreset(config, ~known=["read", "edit"], "no-bash"))
    |> toEqual(Error("Unknown tools: bash"))
  );
});

describe("Session.adjust", () => {
  test("appends adjustments", () =>
    expect(
      Session.adjust(
        ~known,
        session(Some("no-bash"), ["+bash"]),
        "-edit +grep",
      ),
    )
    |> toEqual(Ok(session(Some("no-bash"), ["+bash", "-edit", "+grep"])))
  );

  test("rejects unknown tools", () =>
    expect(Session.adjust(~known, session(None, []), "+ls"))
    |> toEqual(Error("Unknown tools: ls"))
  );

  test("rejects invalid entries", () =>
    expect(
      Result.is_error(Session.adjust(~known, session(None, []), "ls")),
    )
    |> toBe(true)
  );
});
