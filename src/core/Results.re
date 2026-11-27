let map = (f, items) => {
  let rec loop = (mapped, items) =>
    switch (items) {
    | [] => Ok(List.rev(mapped))
    | [item, ...rest] =>
      switch (f(item)) {
      | Ok(value) => loop([value, ...mapped], rest)
      | Error(error) => Error(error)
      }
    };
  loop([], items);
};

let iter = (f, items) => map(f, items) |> Result.map(_ => ());
