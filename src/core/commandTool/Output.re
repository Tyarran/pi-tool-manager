type t = {
  text: string,
  truncated: bool,
  keptLines: int,
  totalLines: int,
};

let utf8Size = code =>
  if (code < 0x80) {
    1;
  } else if (code < 0x800 || code >= 0xD800 && code <= 0xDFFF) {
    2;
  } else {
    3;
  };

let byteLength = text => {
  let length = Js.String.length(text);
  let rec count = (index, total) =>
    index >= length
      ? total
      : count(
          index + 1,
          total + utf8Size(int_of_float(Js.String.charCodeAt(~index, text))),
        );
  count(0, 0);
};

let tail = (~maxLines, ~maxBytes, text) => {
  let lines = String.split_on_char('\n', text);
  let rec keep = (kept, count, bytes, remaining) =>
    switch (remaining) {
    | [] => kept
    | [line, ...rest] =>
      let size = byteLength(line) + (count == 0 ? 0 : 1);
      count >= maxLines || bytes + size > maxBytes
        ? kept : keep([line, ...kept], count + 1, bytes + size, rest);
    };
  let kept = keep([], 0, 0, List.rev(lines));
  let keptLines = List.length(kept);
  let totalLines = List.length(lines);
  {
    text: String.concat("\n", kept),
    truncated: keptLines < totalLines,
    keptLines,
    totalLines,
  };
};

let withNotice = (output, ~file) =>
  Printf.sprintf(
    "[Output truncated: last %d of %d lines. Full output saved to: %s]\n%s",
    output.keptLines,
    output.totalLines,
    file,
    output.text,
  );
