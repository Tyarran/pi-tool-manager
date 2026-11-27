open Jest;
open Expect;

let half = number =>
  number mod 2 == 0
    ? Ok(number / 2) : Error(Printf.sprintf("%d is odd", number));

describe("Results", () => {
  test("maps every item", () =>
    expect(Results.map(half, [2, 4, 6])) |> toEqual(Ok([1, 2, 3]))
  );

  test("stops at the first error", () =>
    expect(Results.map(half, [2, 3, 5])) |> toEqual(Error("3 is odd"))
  );

  test("iterates for checks", () =>
    expect(
      Results.iter(number => half(number) |> Result.map(_ => ()), [1]),
    )
    |> toEqual(Error("1 is odd"))
  );
});
