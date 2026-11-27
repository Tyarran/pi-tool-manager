type t = {
  text: string,
  truncated: bool,
  keptLines: int,
  totalLines: int,
};

let tail: (~maxLines: int, ~maxBytes: int, string) => t;

let withNotice: (t, ~file: string) => string;
