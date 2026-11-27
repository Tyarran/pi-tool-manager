open Jest;
open Expect;

let render = (spec, text) =>
  Specs.arguments(spec, text)
  |> Result.map(values =>
       CommandTool.Render.argv(spec, values, ~home="/home/me")
     );

describe("CommandTool.Render.argv", () => {
  testAll(
    "renders program arguments",
    [
      ({|{"subcommand": "status"}|}, ["/home/me/bin/jj", "status"]),
      (
        {|{"subcommand": "describe", "message": "~ fix", "verbose": true, "limit": 3, "args": ["-r", "@"]}|},
        [
          "/home/me/bin/jj",
          "describe",
          "-m",
          "~ fix",
          "--verbose",
          "-n",
          "3",
          "-r",
          "@",
        ],
      ),
      (
        {|{"subcommand": "status", "verbose": false}|},
        ["/home/me/bin/jj", "status"],
      ),
    ],
    ((text, expected)) =>
    expect(render(Specs.jj, text)) |> toEqual(Ok(expected))
  );

  testAll(
    "quotes the values of a shell command",
    [
      (
        {|{"unit": "a'; rm -rf / #", "since": "1 hour ago", "lines": 5}|},
        "journalctl -u 'a'\\''; rm -rf / #' '--since' '1 hour ago' | tail -n '5' | awk '{print $1}'",
      ),
      (
        {|{"unit": "ssh"}|},
        "journalctl -u 'ssh'  | tail -n  | awk '{print $1}'",
      ),
    ],
    ((text, expected)) =>
    expect(render(Specs.logs, text))
    |> toEqual(Ok(["/bin/sh", "-c", expected]))
  );
});

describe("CommandTool.Render.env", () =>
  test("sets literals and provided parameters", () =>
    expect(
      Specs.arguments(
        Specs.jj,
        {|{"subcommand": "describe", "message": "fix"}|},
      )
      |> Result.map(values =>
           CommandTool.Render.env(Specs.jj, values, ~home="/home/me")
         ),
    )
    |> toEqual(
         Ok([
           ("HOME_DIR", "/home/me"),
           ("JJ_EDITOR", "false"),
           ("JJ_MESSAGE", "fix"),
         ]),
       )
  )
);

describe("CommandTool.Render.workingDirectory", () =>
  test("expands the home directory", () =>
    expect(CommandTool.Render.workingDirectory(Specs.jj, ~home="/home/me"))
    |> toEqual(Some("/home/me/repo"))
  )
);
