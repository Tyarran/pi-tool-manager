module Set = Set.Make(String);

module Map = Map.Make(String);

let keys = map =>
  Map.fold((key, _, keys) => Set.add(key, keys), map, Set.empty);

let join = names => String.concat(", ", Set.elements(names));
