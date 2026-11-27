open Jest;
open Expect;

let tail = (~maxLines=10, ~maxBytes=100, text) =>
  CommandTool.Output.tail(~maxLines, ~maxBytes, text);

describe("CommandTool.Output", () => {
  test("keeps a short output", () =>
    expect(tail("a\nb"))
    |> toEqual({
         CommandTool.Output.text: "a\nb",
         truncated: false,
         keptLines: 2,
         totalLines: 2,
       })
  );

  test("keeps the last lines", () =>
    expect(tail(~maxLines=2, "a\nb\nc"))
    |> toEqual({
         CommandTool.Output.text: "b\nc",
         truncated: true,
         keptLines: 2,
         totalLines: 3,
       })
  );

  test("counts UTF-8 bytes", () =>
    expect(tail(~maxBytes=5, {js|abc
éé|js}).text) |> toEqual({js|éé|js})
  );

  test("announces where the full output is", () =>
    expect(
      CommandTool.Output.withNotice(
        tail(~maxLines=1, "a\nb"),
        ~file="/tmp/x",
      ),
    )
    |> toEqual(
         "[Output truncated: last 1 of 2 lines. Full output saved to: /tmp/x]\nb",
       )
  );
});
