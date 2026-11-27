open Jest;
open Expect;

let decode = (decoder, text) => decoder("root", Helpers.json(text));

describe("Decode", () => {
  testAll(
    "reports the path of invalid values",
    [
      (
        decode(Decode.string, "1") |> Helpers.message,
        "root must be a string",
      ),
      (
        decode(Decode.bool, "1") |> Helpers.message,
        "root must be a boolean",
      ),
      (
        decode(Decode.int, "1.5") |> Helpers.message,
        "root must be an integer",
      ),
      (
        decode(Decode.positiveInt, "0") |> Helpers.message,
        "root must be a positive integer",
      ),
      (
        decode(Decode.list(Decode.string), {|["a", 1]|}) |> Helpers.message,
        "root[1] must be a string",
      ),
      (
        decode(Decode.object_, "[]") |> Helpers.message,
        "root must be an object",
      ),
      (
        decode(Decode.dict(_ => Decode.int), {|{"a": "x"}|}) |> Helpers.message,
        "root.a must be an integer",
      ),
    ],
    ((actual, expected)) =>
    expect(actual) |> toEqual(expected)
  );

  test("decodes a dictionary with its keys", () =>
    expect(
      decode(
        Decode.dict((key, _path, _json) => Ok(key ++ "!")),
        {|{"a": 1}|},
      )
      |> Result.map(Names.Map.bindings),
    )
    |> toEqual(Ok([("a", "a!")]))
  );

  testAll(
    "reads fields",
    [
      (
        Decode.field(Decode.int, "n", "root", Helpers.fields("{}"))
        |> Helpers.message,
        "root.n is required",
      ),
      (
        Decode.optionalField(Decode.int, "n", "root", Helpers.fields("{}"))
        |> Helpers.message,
        "ok",
      ),
      (
        Decode.fieldOr(
          Decode.int,
          "n",
          ~default=3,
          "root",
          Helpers.fields({|{"n": true}|}),
        )
        |> Helpers.message,
        "root.n must be an integer",
      ),
    ],
    ((actual, expected)) =>
    expect(actual) |> toEqual(expected)
  );

  test("uses the default of a missing field", () =>
    expect(
      Decode.fieldOr(
        Decode.int,
        "n",
        ~default=3,
        "root",
        Helpers.fields("{}"),
      ),
    )
    |> toEqual(Ok(3))
  );
});
