open Jest;
open Expect;

let config: Config.t = {
  baseTools: ["read", "edit", "bash"],
  defaultPreset: Some("no-bash"),
  presets: [("no-bash", ["-bash"]), ("read-only", ["-edit", "-bash"])],
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
