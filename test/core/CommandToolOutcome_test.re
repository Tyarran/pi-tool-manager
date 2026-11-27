open Jest;
open Expect;

describe("CommandTool.Outcome", () => {
  testAll(
    "formats the streams and how the command ended",
    [
      (
        "out",
        "err",
        CommandTool.Outcome.Exited(1),
        "stdout:\nout\n\nstderr:\nerr\n\nExit code: 1",
      ),
      ("out\n", " \n", Exited(0), "stdout:\nout\n\nExit code: 0"),
      ("", "", Signaled("SIGKILL"), "Terminated by signal SIGKILL"),
      ("", "", TimedOut(5), "Killed after timeout (5s)"),
      ("", "", Aborted, "Aborted by the user"),
      ("", "", Failed("ENOENT"), "Failed to start: ENOENT"),
    ],
    ((stdout, stderr, outcome, expected)) =>
    expect(CommandTool.Outcome.format(~stdout, ~stderr, outcome))
    |> toEqual(expected)
  );

  testAll(
    "fails unless the command exits with 0",
    [
      (CommandTool.Outcome.Exited(0), false),
      (Exited(2), true),
      (TimedOut(1), true),
      (Aborted, true),
    ],
    ((outcome, expected)) =>
    expect(CommandTool.Outcome.isError(outcome)) |> toBe(expected)
  );
});
