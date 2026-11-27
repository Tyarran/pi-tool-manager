type t =
  | Exited(int)
  | Signaled(string)
  | TimedOut(int)
  | Aborted
  | Failed(string);

let isError = outcome =>
  switch (outcome) {
  | Exited(0) => false
  | _ => true
  };

let status = outcome =>
  switch (outcome) {
  | Exited(code) => Printf.sprintf("Exit code: %d", code)
  | Signaled(signal) => "Terminated by signal " ++ signal
  | TimedOut(seconds) =>
    Printf.sprintf("Killed after timeout (%ds)", seconds)
  | Aborted => "Aborted by the user"
  | Failed(message) => "Failed to start: " ++ message
  };

let rec trimEnd = text => {
  let length = String.length(text);
  length > 0 && (text.[length - 1] == '\n' || text.[length - 1] == '\r')
    ? trimEnd(String.sub(text, 0, length - 1)) : text;
};

let format = (~stdout, ~stderr, outcome) =>
  [("stdout", stdout), ("stderr", stderr)]
  |> List.filter(((_, text)) => String.trim(text) != "")
  |> List.map(((name, text)) => name ++ ":\n" ++ trimEnd(text))
  |> (sections => sections @ [status(outcome)])
  |> String.concat("\n\n");
