open Jest;
open Expect;

let settings = (~baseTools={|["read", "edit", "bash", "echo", "count"]|}, ()) =>
  Printf.sprintf(
    {|{
  "baseTools": %s,
  "defaultPreset": "no-bash",
  "presets": {"no-bash": ["-bash"], "read-only": ["read"]},
  "tools": {
    "echo": {
      "description": "Print words",
      "command": ["printf", "%%s|", "{words}"],
      "parameters": {"words": {"type": "array", "required": true, "deny": ["--evil"]}}
    },
    "count": {
      "description": "Count words with a shell pipeline",
      "shell": true,
      "command": "printf '%%s\\n' {words} | wc -l",
      "parameters": {"words": {"type": "array", "required": true}}
    }
  }
}|},
    baseTools,
  );

let start = (~settings=Some(settings()), ()) => {
  let fake = FakePi.create(settings);
  Extension.activate(FakePi.api(fake));
  FakePi.startSession(fake);
  fake;
};

let callThroughProxy =
    (fake, ~tool, ~intent="Check the behavior", ~arguments="{}", ()) =>
  FakePi.call(
    fake,
    "tool_call",
    Printf.sprintf(
      {|{"tool": %s, "intent": %s, "arguments": %s}|},
      Js.Json.stringify(Js.Json.string(tool)),
      Js.Json.stringify(Js.Json.string(intent)),
      arguments,
    ),
  )
  |> Js.Promise.then_(result => Js.Promise.resolve(FakePi.text(result)));

describe("session start", () => {
  test("activates the default preset and the proxy tools", () =>
    expect(FakePi.active(start()))
    |> toEqual(["count", "echo", "edit", "read", "tool_call", "tool_find"])
  );

  test("keeps the tools of pi when the extension is not configured", () => {
    let fake = start(~settings=None, ());
    expect((FakePi.active(fake), fake.notifications))
    |> toEqual((["bash", "edit", "grep", "read"], []));
  });

  test("reports an invalid configuration and keeps the tools of pi", () => {
    let fake = start(~settings=Some({|{"baseTools": "read"}|}), ());
    expect((FakePi.active(fake), fake.notifications))
    |> toEqual((
         ["bash", "edit", "grep", "read"],
         [
           (
             "error",
             "pi-tool-manager is disabled, fix its configuration: pi-tool-manager.baseTools must be an array",
           ),
         ],
       ));
  });
});

describe("commands", () => {
  test("/tool-preset switches the preset", () => {
    let fake = start();
    FakePi.runCommand(fake, "tool-preset", "read-only");
    expect((FakePi.active(fake), fake.notifications))
    |> toEqual((
         ["read", "tool_call", "tool_find"],
         [("info", "Active tools: read")],
       ));
  });

  test("/tool adjusts the session until the next preset switch", () => {
    let fake = start();
    FakePi.runCommand(fake, "tool", "+bash -edit");
    let adjusted = FakePi.active(fake);
    FakePi.runCommand(fake, "tool-preset", "no-bash");
    expect((adjusted, FakePi.active(fake)))
    |> toEqual((
         ["bash", "count", "echo", "read", "tool_call", "tool_find"],
         ["count", "echo", "edit", "read", "tool_call", "tool_find"],
       ));
  });

  testAll(
    "report errors without changing the tools",
    [
      (
        "tool-preset",
        "",
        "Usage: /tool-preset <name> (available: no-bash, read-only)",
      ),
      (
        "tool-preset",
        "code",
        "Unknown preset \"code\" (available: no-bash, read-only)",
      ),
      ("tool", "+ghost", "Unknown tools: ghost"),
      (
        "tool",
        "bash",
        "Invalid entry \"bash\": use +name to add a tool and -name to remove one",
      ),
    ],
    ((command, args, message)) => {
      let fake = start();
      let before = FakePi.active(fake);
      FakePi.runCommand(fake, command, args);
      expect((FakePi.active(fake), fake.notifications))
      |> toEqual((before, [("error", message)]));
    },
  );
});

describe("guard", () => {
  test("re-applies the selection when tools appear", () => {
    let fake =
      start(
        ~settings=
          Some(
            settings(
              ~baseTools={|["read", "edit", "bash", "echo", "count", "late"]|},
              (),
            ),
          ),
        (),
      );
    FakePi.register(fake, FakePi.builtin("late", "Registered late"));
    FakePi.register(fake, FakePi.builtin("intruder", "Not selected"));
    FakePi.startTurn(fake);
    expect(FakePi.active(fake))
    |> toEqual([
         "count",
         "echo",
         "edit",
         "late",
         "read",
         "tool_call",
         "tool_find",
       ]);
  });

  testAll(
    "blocks calls outside the selection",
    [
      (
        "bash",
        Some(
          "bash is not allowed by the active tool selection (pi-tool-manager)",
        ),
      ),
      ("read", None),
      ("tool_find", None),
    ],
    ((tool, expected)) =>
    expect(FakePi.blockReason(start(), tool)) |> toEqual(expected)
  );
});

describe("tool_call", () => {
  testPromise("runs a selected tool", () =>
    callThroughProxy(start(), ~tool="read", ())
    |> Js.Promise.then_(text =>
         Js.Promise.resolve(expect(text) |> toEqual("ran read"))
       )
  );

  testPromise("refuses a tool outside the selection", () =>
    callThroughProxy(start(), ~tool="bash", ())
    |> Js.Promise.then_(text =>
         Js.Promise.resolve(
           expect(text)
           |> toEqual(
                "Tool \"bash\" is not available. Available tools: count, echo, edit, read",
              ),
         )
       )
  );

  testPromise("requires an intent", () =>
    callThroughProxy(start(), ~tool="read", ~intent=" ", ())
    |> Js.Promise.then_(text =>
         Js.Promise.resolve(
           expect(text)
           |> toEqual("intent must explain why the tool is called"),
         )
       )
  );

  testPromise("returns the expected parameters of a rejected call", () =>
    callThroughProxy(
      start(),
      ~tool="read",
      ~arguments={|{"invalid": true}|},
      (),
    )
    |> Js.Promise.then_(text =>
         Js.Promise.resolve(
           expect(text)
           |> toContainString(
                "Validation failed\nExpected parameters of read:",
              ),
         )
       )
  );

  testPromise("runs a command tool without a shell", () =>
    callThroughProxy(
      start(),
      ~tool="echo",
      ~arguments={|{"words": ["a b", "$(whoami)"]}|},
      (),
    )
    |> Js.Promise.then_(text =>
         Js.Promise.resolve(
           expect(text) |> toEqual("stdout:\na b|$(whoami)|\n\nExit code: 0"),
         )
       )
  );

  testPromise("rejects invalid arguments of a command tool", () =>
    callThroughProxy(
      start(),
      ~tool="echo",
      ~arguments={|{"words": ["--evil"]}|},
      (),
    )
    |> Js.Promise.then_(text =>
         Js.Promise.resolve(
           expect(text) |> toEqual("words does not allow \"--evil\""),
         )
       )
  );

  testPromise("quotes the values of a shell command", () =>
    callThroughProxy(
      start(),
      ~tool="count",
      ~arguments={|{"words": ["a", "b; echo hacked"]}|},
      (),
    )
    |> Js.Promise.then_(text =>
         Js.Promise.resolve(
           expect(text) |> toEqual("stdout:\n2\n\nExit code: 0"),
         )
       )
  );

  test("hides the selected tools and describes them", () => {
    let fake = start();
    let changes =
      switch (FakePi.find(fake, "tool_call").prepareLoadout) {
      | Some(prepare) =>
        prepare(
          FakePi.loadout(
            ~guidelines=[("read", ["Use read to inspect files."])],
            [
              {
                name: "read",
                description: "Read a file",
              },
              {
                name: "tool_call",
                description: "Proxy",
              },
            ],
          ),
        )
      | None => failwith("tool_call must prepare the loadout")
      };
    let description =
      Option.value(
        Js.Dict.get(changes.descriptions, "tool_call"),
        ~default="",
      );
    expect((
      changes.hiddenDeclarations,
      Js.String.includes(~search="- read: Read a file", description),
      Js.String.includes(~search="- Use read to inspect files.", description),
    ))
    |> toEqual(([|"read"|], true, true));
  });

  test("renders the tool, the intent and the arguments", () => {
    let rendered =
      switch (FakePi.find(start(), "tool_call").renderCall) {
      | Some(render) =>
        render(
          Js.Json.parseExn(
            {|{"tool": "edit", "intent": "Fix a typo", "arguments": {"path": "a"}}|},
          ),
          FakePi.theme,
          Js.Json.null,
        )
      | None => failwith("tool_call must render its calls")
      };
    expect(FakePi.renderedText(rendered).text)
    |> toEqual(
         "<toolTitle>*tool_call* <accent>edit\n<muted>Fix a typo\n<dim>{\"path\":\"a\"}",
       );
  });
});

describe("tool_find", () =>
  testPromise("searches the selected tools only", () =>
    FakePi.call(start(), "tool_find", {|{"query": "shell"}|})
    |> Js.Promise.then_(result => {
         let text = FakePi.text(result);
         Js.Promise.resolve(
           expect((
             Js.String.includes(~search="## count", text),
             Js.String.includes(~search="## bash", text),
           ))
           |> toEqual((true, false)),
         );
       })
  )
);
