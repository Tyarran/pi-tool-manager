open Jest;
open Expect;

describe("Outcome.format", () => {
  test("shows both streams and the exit code", () =>
    expect(Outcome.format(~stdout="out", ~stderr="err", Exited(1)))
    |> toEqual("stdout:\nout\n\nstderr:\nerr\n\nExit code: 1")
  );

  test("omits empty streams", () =>
    expect(Outcome.format(~stdout="out", ~stderr=" \n", Exited(0)))
    |> toEqual("stdout:\nout\n\nExit code: 0")
  );

  test("trims trailing newlines of the streams", () =>
    expect(Outcome.format(~stdout="out\n", ~stderr="err\r\n\n", Exited(1)))
    |> toEqual("stdout:\nout\n\nstderr:\nerr\n\nExit code: 1")
  );

  testAll(
    "describes how the command ended",
    [
      (Outcome.Signaled("SIGKILL"), "Terminated by signal SIGKILL"),
      (Outcome.TimedOut(5), "Killed after timeout (5s)"),
      (Outcome.Aborted, "Aborted by the user"),
      (Outcome.Failed("ENOENT"), "Failed to start: ENOENT"),
    ],
    ((outcome, expected)) =>
    expect(Outcome.format(~stdout="", ~stderr="", outcome))
    |> toEqual(expected)
  );
});

describe("Outcome.isError", () =>
  testAll(
    "only a zero exit code succeeds",
    [
      (Outcome.Exited(0), false),
      (Outcome.Exited(2), true),
      (Outcome.TimedOut(1), true),
      (Outcome.Aborted, true),
    ],
    ((outcome, expected)) =>
    expect(Outcome.isError(outcome)) |> toBe(expected)
  )
);
