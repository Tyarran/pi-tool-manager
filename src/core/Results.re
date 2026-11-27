let all = results =>
  List.fold_right(
    (result, acc) =>
      switch (result, acc) {
      | (Ok(value), Ok(values)) => Ok([value, ...values])
      | (Error(message), _)
      | (_, Error(message)) => Error(message)
      },
    results,
    Ok([]),
  );
