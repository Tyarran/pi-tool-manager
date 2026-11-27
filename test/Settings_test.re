open Jest;
open Expect;

describe("Settings", () =>
  test("reads its configuration under the pi-tool-manager key", () =>
    expect(Settings.key) |> toEqual("pi-tool-manager")
  )
);
