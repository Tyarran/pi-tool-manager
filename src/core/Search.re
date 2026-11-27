type document = {
  name: string,
  text: string,
  namespace: string,
};

type indexed = {
  name: string,
  nameTerms: Names.Set.t,
  localNameTerms: Names.Set.t,
  termCounts: Names.Map.t(int),
  length: int,
  namespaceTerms: Names.Set.t,
};

type scored = {
  name: string,
  score: float,
  covered: int,
  exact: bool,
};

let nameWeight = 3.0;

let namespaceWeight = 0.5;

let k1 = 1.2;

let b = 0.75;

let stopWords =
  Names.Set.of_list([
    "a",
    "an",
    "and",
    "are",
    "as",
    "at",
    "be",
    "by",
    "for",
    "from",
    "in",
    "is",
    "it",
    "of",
    "on",
    "or",
    "that",
    "the",
    "this",
    "to",
    "with",
  ]);

let endsWith = (~suffix, text) => {
  let length = String.length(text);
  let suffixLength = String.length(suffix);
  length >= suffixLength
  && String.sub(text, length - suffixLength, suffixLength) == suffix;
};

let dropEnd = (count, text) =>
  String.sub(text, 0, String.length(text) - count);

let singular = term => {
  let length = String.length(term);
  if (length > 4 && endsWith(~suffix="ies", term)) {
    dropEnd(3, term) ++ "y";
  } else if (length > 4
             && List.exists(
                  suffix => endsWith(~suffix, term),
                  ["ches", "shes", "sses", "xes", "zes"],
                )) {
    dropEnd(2, term);
  } else if (length > 3
             && endsWith(~suffix="s", term)
             && !endsWith(~suffix="ss", term)) {
    dropEnd(1, term);
  } else {
    term;
  };
};

let replace = (pattern, replacement, text) =>
  Js.String.replaceByRe(
    ~regexp=Js.Re.fromStringWithFlags(pattern, ~flags="gu"),
    ~replacement,
    text,
  );

let isAsciiWord = word =>
  Js.Re.test(~str=word, Js.Re.fromString("^[a-z0-9]+$"));

let tokenize = text =>
  text
  |> replace("([a-z0-9])([A-Z])", "$1 $2")
  |> replace("([A-Z]+)([A-Z][a-z])", "$1 $2")
  |> Js.String.toLowerCase
  |> replace("[^\\p{L}\\p{M}\\p{N}]+", " ")
  |> String.split_on_char(' ')
  |> List.filter(word => word != "" && !Names.Set.mem(word, stopWords))
  |> List.map(word => isAsciiWord(word) ? singular(word) : word);

let terms = text => Names.Set.of_list(tokenize(text));

let stringField = (fields, key) =>
  Option.bind(Js.Dict.get(fields, key), Js.Json.decodeString);

let rec schemaText = json =>
  switch (Js.Json.decodeObject(json)) {
  | None => []
  | Some(schema) =>
    let nested = key =>
      Option.fold(~none=[], ~some=schemaText, Js.Dict.get(schema, key));
    let properties =
      switch (
        Option.bind(Js.Dict.get(schema, "properties"), Js.Json.decodeObject)
      ) {
      | Some(properties) =>
        properties
        |> Js.Dict.entries
        |> Array.to_list
        |> List.concat_map(((name, property)) =>
             [name, ...schemaText(property)]
           )
      | None => []
      };
    let variants =
      ["anyOf", "oneOf", "allOf"]
      |> List.concat_map(key =>
           switch (
             Option.bind(Js.Dict.get(schema, key), Js.Json.decodeArray)
           ) {
           | Some(variants) =>
             List.concat_map(schemaText, Array.to_list(variants))
           | None => []
           }
         );
    List.concat([
      Option.to_list(stringField(schema, "description")),
      properties,
      nested("items"),
      variants,
    ]);
  };

let document = (~name, ~description, ~parameters, ~namespace) => {
  name,
  text:
    [description, ...schemaText(parameters)]
    |> List.filter(part => String.trim(part) != "")
    |> String.concat(" "),
  namespace: String.concat(" ", namespace),
};

let countTerms = words =>
  List.fold_left(
    (counts, word) =>
      Names.Map.update(
        word,
        count => Some(Option.value(count, ~default=0) + 1),
        counts,
      ),
    Names.Map.empty,
    words,
  );

let lastSegment = name =>
  switch (List.rev(Array.to_list(Js.String.split(~sep="__", name)))) {
  | [last, ..._] => last
  | [] => name
  };

let index = (document: document) => {
  let words = tokenize(document.text);
  {
    name: document.name,
    nameTerms: terms(document.name),
    localNameTerms: terms(lastSegment(document.name)),
    termCounts: countTerms(words),
    length: List.length(words),
    namespaceTerms: terms(document.namespace),
  };
};

let normalizeName = name =>
  name |> String.trim |> Js.String.toLowerCase |> replace("[^a-z0-9_$]", "_");

let isExactName = (~query, name) => {
  let normalized = normalizeName(name);
  normalized == query || endsWith(~suffix="__" ++ query, normalized);
};

let contains = (term, indexed) =>
  Names.Set.mem(term, indexed.nameTerms)
  || Names.Map.mem(term, indexed.termCounts)
  || Names.Set.mem(term, indexed.namespaceTerms);

let inverseFrequency = (documents, term) => {
  let total = float_of_int(List.length(documents));
  let frequency =
    float_of_int(List.length(List.filter(contains(term), documents)));
  log(1.0 +. (total -. frequency +. 0.5) /. (frequency +. 0.5));
};

let averageLength = documents => {
  let total = List.fold_left((sum, doc) => sum + doc.length, 0, documents);
  total == 0
    ? 1.0 : float_of_int(total) /. float_of_int(List.length(documents));
};

let termWeight = (indexed, ~norm, term) => {
  let count =
    float_of_int(
      Option.value(Names.Map.find_opt(term, indexed.termCounts), ~default=0),
    );
  (Names.Set.mem(term, indexed.nameTerms) ? nameWeight : 0.0)
  +. (Names.Set.mem(term, indexed.namespaceTerms) ? namespaceWeight : 0.0)
  +. count
  *. (k1 +. 1.0)
  /. (count +. norm);
};

let score = (~queryTerms, ~weights, ~average, ~exactQuery, indexed) => {
  let norm = k1 *. (1.0 -. b +. b *. float_of_int(indexed.length) /. average);
  let (score, covered) =
    Names.Map.fold(
      (term, idf, (score, covered)) => {
        let weight = termWeight(indexed, ~norm, term);
        (score +. idf *. weight, weight > 0.0 ? covered + 1 : covered);
      },
      weights,
      (0.0, 0),
    );
  let fullNameBonus =
    !Names.Set.is_empty(indexed.localNameTerms)
    && Names.Set.subset(indexed.localNameTerms, queryTerms)
      ? Names.Map.fold((_, idf, sum) => sum +. idf, weights, 0.0) : 0.0;
  score == 0.0
    ? None
    : Some({
        name: indexed.name,
        score: score +. fullNameBonus,
        covered,
        exact: isExactName(~query=exactQuery, indexed.name),
      });
};

let compareScored = (a, b) =>
  if (a.exact != b.exact) {
    compare(b.exact, a.exact);
  } else if (a.covered != b.covered) {
    compare(b.covered, a.covered);
  } else {
    compare(b.score, a.score);
  };

let rank = (query, documents, ~limit) => {
  let queryTerms = terms(query);
  if (Names.Set.is_empty(queryTerms) || documents == [] || limit <= 0) {
    [];
  } else {
    let indexed = List.map(index, documents);
    let weights =
      Names.Set.fold(
        (term, weights) =>
          Names.Map.add(term, inverseFrequency(indexed, term), weights),
        queryTerms,
        Names.Map.empty,
      );
    indexed
    |> List.filter_map(
         score(
           ~queryTerms,
           ~weights,
           ~average=averageLength(indexed),
           ~exactQuery=normalizeName(query),
         ),
       )
    |> List.stable_sort(compareScored)
    |> List.filteri((position, _) => position < limit)
    |> List.map(scored => scored.name);
  };
};
