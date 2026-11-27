module Set: Stdlib.Set.S with type elt = string;

module Map: Stdlib.Map.S with type key = string;

let keys: Map.t('a) => Set.t;

let join: Set.t => string;
