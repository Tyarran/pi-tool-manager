let quote = value =>
  "'" ++ String.concat("'\\''", String.split_on_char('\'', value)) ++ "'";

let join = args => String.concat(" ", List.map(quote, args));
