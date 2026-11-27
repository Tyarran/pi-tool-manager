type document = {
  name: string,
  text: string,
  namespaceText: string,
};

let stopWords = [
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
];

let endsWith = (~suffix, text) => {
  let length = String.length(text);
  let suffixLength = String.length(suffix);
  length >= suffixLength
  && String.sub(text, length - suffixLength, suffixLength) == suffix;
};

let dropEnd = (count, text) =>
  String.sub(text, 0, String.length(text) - count);

let stem = term => {
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
  |> List.filter(word => word != "" && !List.mem(word, stopWords))
  |> List.map(word => isAsciiWord(word) ? stem(word) : word);

let rec schemaText = json =>
  switch (Js.Json.classify(json)) {
  | JSONObject(schema) =>
    let field = key => Js.Dict.get(schema, key);
    let description =
      switch (Option.map(Js.Json.classify, field("description"))) {
      | Some(JSONString(text)) => [text]
      | _ => []
      };
    let properties =
      switch (Option.map(Js.Json.classify, field("properties"))) {
      | Some(JSONObject(properties)) =>
        properties
        |> Js.Dict.entries
        |> Array.to_list
        |> List.concat_map(((name, property)) =>
             [name, ...schemaText(property)]
           )
      | _ => []
      };
    let items = Option.fold(~none=[], ~some=schemaText, field("items"));
    let variants =
      ["anyOf", "oneOf", "allOf"]
      |> List.concat_map(key =>
           switch (Option.map(Js.Json.classify, field(key))) {
           | Some(JSONArray(variants)) =>
             List.concat_map(schemaText, Array.to_list(variants))
           | _ => []
           }
         );
    description @ properties @ items @ variants;
  | _ => []
  };

let document = (~name, ~description, ~parameters, ~namespaceText) => {
  name,
  text:
    [description, ...schemaText(parameters)]
    |> List.filter(part => String.trim(part) != "")
    |> String.concat(" "),
  namespaceText,
};

let normalizeName = name =>
  name |> String.trim |> Js.String.toLowerCase |> replace("[^a-z0-9_$]", "_");

let isExactNameQuery = (query, name) => {
  let normalized = normalizeName(name);
  normalized == query || endsWith(~suffix="__" ++ query, normalized);
};

let nameWeight = 3.0;
let namespaceWeight = 0.5;
let k1 = 1.2;
let b = 0.75;

type indexed = {
  name: string,
  nameTerms: list(string),
  localNameTerms: list(string),
  textCounts: list((string, int)),
  textLength: int,
  namespaceTerms: list(string),
};

let uniq = terms => List.sort_uniq(compare, terms);

let count = terms =>
  List.fold_left(
    (counts, term) =>
      switch (List.assoc_opt(term, counts)) {
      | Some(n) => [(term, n + 1), ...List.remove_assoc(term, counts)]
      | None => [(term, 1), ...counts]
      },
    [],
    terms,
  );

let lastSegment = name =>
  switch (List.rev(Js.String.split(~sep="__", name) |> Array.to_list)) {
  | [last, ..._] => last
  | [] => name
  };

let index = (document: document) => {
  let textTerms = tokenize(document.text);
  {
    name: document.name,
    nameTerms: uniq(tokenize(document.name)),
    localNameTerms: uniq(tokenize(lastSegment(document.name))),
    textCounts: count(textTerms),
    textLength: List.length(textTerms),
    namespaceTerms: uniq(tokenize(document.namespaceText)),
  };
};

let rank = (query, documents, ~limit) => {
  let queryTerms = uniq(tokenize(query));
  if (queryTerms == [] || documents == [] || limit <= 0) {
    [];
  } else {
    let indexed = List.map(index, documents);
    let total = float_of_int(List.length(indexed));
    let averageLength = {
      let sum =
        List.fold_left((sum, doc) => sum + doc.textLength, 0, indexed);
      sum == 0 ? 1.0 : float_of_int(sum) /. total;
    };
    let idf =
      List.map(
        term => {
          let frequency =
            indexed
            |> List.filter(doc =>
                 List.mem(term, doc.nameTerms)
                 || List.mem_assoc(term, doc.textCounts)
                 || List.mem(term, doc.namespaceTerms)
               )
            |> List.length
            |> float_of_int;
          (
            term,
            log(1.0 +. (total -. frequency +. 0.5) /. (frequency +. 0.5)),
          );
        },
        queryTerms,
      );
    let fullNameBonus = List.fold_left((sum, (_, v)) => sum +. v, 0.0, idf);
    let exactQuery = normalizeName(query);
    indexed
    |> List.filter_map(doc => {
         let norm =
           k1
           *. (1.0 -. b +. b *. float_of_int(doc.textLength) /. averageLength);
         let (score, covered) =
           List.fold_left(
             ((score, covered), (term, termIdf)) => {
               let n =
                 float_of_int(
                   Option.value(
                     List.assoc_opt(term, doc.textCounts),
                     ~default=0,
                   ),
                 );
               let weight =
                 (List.mem(term, doc.nameTerms) ? nameWeight : 0.0)
                 +. (
                   List.mem(term, doc.namespaceTerms) ? namespaceWeight : 0.0
                 )
                 +. n
                 *. (k1 +. 1.0)
                 /. (n +. norm);
               (
                 score +. termIdf *. weight,
                 weight > 0.0 ? covered + 1 : covered,
               );
             },
             (0.0, 0),
             idf,
           );
         if (score == 0.0) {
           None;
         } else {
           let bonus =
             doc.localNameTerms != []
             && List.for_all(
                  term => List.mem(term, queryTerms),
                  doc.localNameTerms,
                )
               ? fullNameBonus : 0.0;
           Some((
             doc.name,
             score +. bonus,
             covered,
             isExactNameQuery(exactQuery, doc.name),
           ));
         };
       })
    |> List.stable_sort(((_, s1, c1, e1), (_, s2, c2, e2)) =>
         if (e1 != e2) {
           e2 ? 1 : (-1);
         } else if (c1 != c2) {
           compare(c2, c1);
         } else {
           compare(s2, s1);
         }
       )
    |> List.filteri((i, _) => i < limit)
    |> List.map(((name, score, _, _)) => (name, score));
  };
};
